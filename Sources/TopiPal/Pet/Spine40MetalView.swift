import AppKit
import Metal
import MetalKit
import SpineC40
import SwiftUI

struct Spine40MetalView: NSViewRepresentable {
    let character: PetCharacter
    let action: String?

    func makeCoordinator() -> Coordinator {
        Coordinator()
    }

    func makeNSView(context: Context) -> MTKView {
        let view = TransparentSpine40MTKView()
        view.device = MTLCreateSystemDefaultDevice()
        view.clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 0)
        view.colorPixelFormat = .bgra8Unorm
        view.framebufferOnly = false
        view.layer?.isOpaque = false
        view.layer?.backgroundColor = NSColor.clear.cgColor
        view.isPaused = false
        view.enableSetNeedsDisplay = false
        view.preferredFramesPerSecond = 60
        context.coordinator.attach(to: view)
        context.coordinator.load(character: character, in: view)
        context.coordinator.setAction(action)
        return view
    }

    func updateNSView(_ view: MTKView, context: Context) {
        context.coordinator.load(character: character, in: view)
        context.coordinator.setAction(action)
    }

    final class Coordinator: NSObject, MTKViewDelegate {
        private var characterID: String?
        private var drawable: OpaquePointer?
        private var textures: [MTLTexture] = []
        private var commandQueue: MTLCommandQueue?
        private var pipelineState: MTLRenderPipelineState?
        private var lastFrameDate = Date()
        private var currentAction: String?
        private var referenceBounds: CGRect = .zero

        deinit {
            if let drawable {
                topi_spine40_dispose(drawable)
            }
        }

        @MainActor
        func attach(to view: MTKView) {
            guard let device = view.device else { return }
            commandQueue = device.makeCommandQueue()

            let library: MTLLibrary?
            do {
                library = try device.makeLibrary(source: Self.shaderSource, options: nil)
            } catch {
                print("Spine Metal shader compile failed: \(error)")
                library = nil
            }
            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = library?.makeFunction(name: "spine40_vertex")
            descriptor.fragmentFunction = library?.makeFunction(name: "spine40_fragment")
            descriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat
            descriptor.colorAttachments[0].isBlendingEnabled = true
            descriptor.colorAttachments[0].rgbBlendOperation = .add
            descriptor.colorAttachments[0].alphaBlendOperation = .add
            descriptor.colorAttachments[0].sourceRGBBlendFactor = .one
            descriptor.colorAttachments[0].sourceAlphaBlendFactor = .one
            descriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            descriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha

            do {
                pipelineState = try device.makeRenderPipelineState(descriptor: descriptor)
            } catch {
                print("Spine Metal pipeline failed: \(error)")
            }
            view.delegate = self
        }

        @MainActor
        func load(character: PetCharacter, in view: MTKView) {
            guard characterID != character.id else { return }
            guard let device = view.device else { return }

            if let drawable {
                topi_spine40_dispose(drawable)
                self.drawable = nil
            }

            let atlasURL: URL?
            let skeletonURL: URL?
            if let directory = PetAssetLibrary.resourceDirectory(for: character) {
                atlasURL = directory.appendingPathComponent("\(character.resourceBaseName).atlas")
                skeletonURL = directory.appendingPathComponent("\(character.resourceBaseName).skel")
            } else {
                atlasURL = AppResourceBundle.module.url(
                    forResource: character.resourceBaseName,
                    withExtension: "atlas",
                    subdirectory: character.resourceSubdirectory
                )
                skeletonURL = AppResourceBundle.module.url(
                    forResource: character.resourceBaseName,
                    withExtension: "skel",
                    subdirectory: character.resourceSubdirectory
                )
            }

            guard let atlasURL,
                  let skeletonURL,
                  let nextDrawable = topi_spine40_create(atlasURL.path, skeletonURL.path, 1.0) else {
                characterID = nil
                textures = []
                return
            }

            drawable = nextDrawable
            characterID = character.id
            textures = loadTextures(for: nextDrawable, atlasDirectory: atlasURL.deletingLastPathComponent(), device: device)

            if let idle = findAnimation(in: nextDrawable, containing: "idle") {
                currentAction = idle
                topi_spine40_set_animation(nextDrawable, idle, 1)
            } else if let first = firstAnimationName(in: nextDrawable) {
                currentAction = first
                topi_spine40_set_animation(nextDrawable, first, 1)
            }

            // Capture reference bounds from the idle pose to keep centering stable across animations
            topi_spine40_update(nextDrawable, 0)
            let vCount = Int(topi_spine40_vertex_count(nextDrawable))
            if vCount > 0, let vPtr = topi_spine40_vertices(nextDrawable) {
                referenceBounds = vertexBounds(vertices: vPtr, count: vCount)
            }
        }

        func setAction(_ action: String?) {
            guard let drawable else { return }
            let resolved = action ?? firstAnimationName(in: drawable)
            guard currentAction != resolved, let resolved else { return }
            currentAction = resolved
            topi_spine40_set_animation(drawable, resolved, 1)
        }

        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}

        func draw(in view: MTKView) {
            guard let drawable,
                  let pipelineState,
                  let commandQueue,
                  let currentDrawable = view.currentDrawable,
                  let renderPass = view.currentRenderPassDescriptor else {
                return
            }

            let now = Date()
            let delta = min(1.0 / 20.0, now.timeIntervalSince(lastFrameDate))
            lastFrameDate = now
            topi_spine40_update(drawable, Float(delta))

            let vertexCount = Int(topi_spine40_vertex_count(drawable))
            let indexCount = Int(topi_spine40_index_count(drawable))
            let commandCount = Int(topi_spine40_command_count(drawable))
            guard vertexCount > 0, indexCount > 0, commandCount > 0,
                  let vertexPointer = topi_spine40_vertices(drawable),
                  let indexPointer = topi_spine40_indices(drawable),
                  let commandPointer = topi_spine40_commands(drawable),
                  let device = view.device,
                  let vertexBuffer = device.makeBuffer(
                    bytes: vertexPointer,
                    length: MemoryLayout<TopiSpine40Vertex>.stride * vertexCount
                  ) else {
                return
            }

            let commands = UnsafeBufferPointer(start: commandPointer, count: commandCount)
            let uniforms = makeUniforms(bounds: referenceBounds, drawableSize: view.drawableSize)

            guard let commandBuffer = commandQueue.makeCommandBuffer(),
                  let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPass) else {
                return
            }

            encoder.setRenderPipelineState(pipelineState)
            encoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
            var mutableUniforms = uniforms
            encoder.setVertexBytes(&mutableUniforms, length: MemoryLayout<Spine40Uniforms>.stride, index: 1)

            for command in commands {
                guard command.textureIndex >= 0, command.textureIndex < textures.count else { continue }
                encoder.setFragmentTexture(textures[Int(command.textureIndex)], index: 0)
                let start = indexPointer.advanced(by: Int(command.indexOffset))
                encoder.drawIndexedPrimitives(
                    type: .triangle,
                    indexCount: Int(command.indexCount),
                    indexType: .uint16,
                    indexBuffer: makeIndexBuffer(device: device, pointer: start, count: Int(command.indexCount)),
                    indexBufferOffset: 0
                )
            }

            encoder.endEncoding()
            commandBuffer.present(currentDrawable)
            commandBuffer.commit()
        }

        private func loadTextures(for drawable: OpaquePointer, atlasDirectory: URL, device: MTLDevice) -> [MTLTexture] {
            let loader = MTKTextureLoader(device: device)
            let count = Int(topi_spine40_texture_count(drawable))

            return (0..<count).compactMap { index in
                guard let namePointer = topi_spine40_texture_name(drawable, Int32(index)) else { return nil }
                let name = String(cString: namePointer)
                let url = atlasDirectory.appendingPathComponent(name)
                return try? loader.newTexture(URL: url, options: [
                    .SRGB: false,
                    .textureUsage: NSNumber(value: MTLTextureUsage.shaderRead.rawValue)
                ])
            }
        }

        private func firstAnimationName(in drawable: OpaquePointer) -> String? {
            let count = Int(topi_spine40_animation_count(drawable))
            guard count > 0, let name = topi_spine40_animation_name(drawable, 0) else { return nil }
            return String(cString: name)
        }

        private func findAnimation(in drawable: OpaquePointer, containing keyword: String) -> String? {
            let count = Int(topi_spine40_animation_count(drawable))
            for i in 0..<count {
                if let name = topi_spine40_animation_name(drawable, Int32(i)) {
                    let str = String(cString: name)
                    if str.localizedCaseInsensitiveContains(keyword) {
                        return str
                    }
                }
            }
            return nil
        }

        private func vertexBounds(vertices: UnsafePointer<TopiSpine40Vertex>, count: Int) -> CGRect {
            var minX = CGFloat.greatestFiniteMagnitude
            var minY = CGFloat.greatestFiniteMagnitude
            var maxX = -CGFloat.greatestFiniteMagnitude
            var maxY = -CGFloat.greatestFiniteMagnitude

            for index in 0..<count {
                let vertex = vertices[index]
                minX = min(minX, CGFloat(vertex.x))
                minY = min(minY, CGFloat(vertex.y))
                maxX = max(maxX, CGFloat(vertex.x))
                maxY = max(maxY, CGFloat(vertex.y))
            }

            return CGRect(x: minX, y: minY, width: max(1, maxX - minX), height: max(1, maxY - minY))
        }

        private func makeUniforms(bounds: CGRect, drawableSize: CGSize) -> Spine40Uniforms {
            let padding: CGFloat = 8
            let availableWidth = max(1, drawableSize.width - padding * 2)
            let availableHeight = max(1, drawableSize.height - padding * 2)
            let scale = min(availableWidth / bounds.width, availableHeight / bounds.height)
            let fittedWidth = bounds.width * scale
            let fittedHeight = bounds.height * scale
            let offset = SIMD2<Float>(
                Float((drawableSize.width - fittedWidth) / 2),
                Float((drawableSize.height - fittedHeight) / 2)
            )

            return Spine40Uniforms(
                viewportWidth: Float(drawableSize.width),
                viewportHeight: Float(drawableSize.height),
                contentMinX: Float(bounds.minX),
                contentMinY: Float(bounds.minY),
                contentScale: Float(scale),
                reserved: 0,
                contentOffsetX: offset.x,
                contentOffsetY: offset.y
            )
        }

        private func makeIndexBuffer(device: MTLDevice?, pointer: UnsafePointer<UInt16>, count: Int) -> MTLBuffer {
            device!.makeBuffer(bytes: pointer, length: MemoryLayout<UInt16>.stride * count)!
        }

        private static let shaderSource = """
        #include <metal_stdlib>
        using namespace metal;

        struct SpineVertexIn {
            float2 position;
            float2 uv;
            float4 color;
        };

        struct Spine40Uniforms {
            float viewportWidth;
            float viewportHeight;
            float contentMinX;
            float contentMinY;
            float contentScale;
            float reserved;
            float contentOffsetX;
            float contentOffsetY;
        };

        struct SpineVertexOut {
            float4 position [[position]];
            float2 uv;
            float4 color;
        };

        vertex SpineVertexOut spine40_vertex(
            const device SpineVertexIn *vertices [[buffer(0)]],
            constant Spine40Uniforms &uniforms [[buffer(1)]],
            uint vertexID [[vertex_id]]
        ) {
            SpineVertexIn input = vertices[vertexID];
            float2 contentMin = float2(uniforms.contentMinX, uniforms.contentMinY);
            float2 contentOffset = float2(uniforms.contentOffsetX, uniforms.contentOffsetY);
            float2 fitted = (input.position - contentMin) * uniforms.contentScale + contentOffset;
            float2 clip;
            clip.x = fitted.x / uniforms.viewportWidth * 2.0 - 1.0;
            clip.y = 1.0 - fitted.y / uniforms.viewportHeight * 2.0;

            SpineVertexOut output;
            output.position = float4(clip, 0.0, 1.0);
            output.uv = input.uv;
            output.color = input.color;
            return output;
        }

        fragment float4 spine40_fragment(
            SpineVertexOut input [[stage_in]],
            texture2d<float> texture [[texture(0)]]
        ) {
            constexpr sampler textureSampler(mag_filter::linear, min_filter::linear, address::clamp_to_edge);
            return texture.sample(textureSampler, input.uv) * input.color;
        }
        """
    }
}

private final class TransparentSpine40MTKView: MTKView {
    override var isOpaque: Bool {
        false
    }
}

private struct Spine40Uniforms {
    var viewportWidth: Float
    var viewportHeight: Float
    var contentMinX: Float
    var contentMinY: Float
    var contentScale: Float
    var reserved: Float
    var contentOffsetX: Float
    var contentOffsetY: Float
}
