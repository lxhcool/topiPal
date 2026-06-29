import AppKit
import Live2DCore
import Metal
import MetalKit
import SwiftUI

struct Live2DMetalView: NSViewRepresentable {
    let character: PetCharacter
    let action: String?

    func makeCoordinator() -> Coordinator {
        Coordinator()
    }

    func makeNSView(context: Context) -> MTKView {
        let view = TransparentLive2DMTKView()
        view.device = MTLCreateSystemDefaultDevice()
        view.clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 0)
        view.colorPixelFormat = .bgra8Unorm
        view.depthStencilPixelFormat = .stencil8
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
        private var model: OpaquePointer?
        private var manifest: Live2DModel3Manifest?
        private var modelDirectory: URL?
        private var textures: [MTLTexture] = []
        private var commandQueue: MTLCommandQueue?
        private var pipelineState: MTLRenderPipelineState?
        private var maskPipelineState: MTLRenderPipelineState?
        private var maskWriteDepthStencilState: MTLDepthStencilState?
        private var maskedDepthStencilState: MTLDepthStencilState?
        private var invertedMaskedDepthStencilState: MTLDepthStencilState?
        private var currentAction: String?
        private var activeExpression: Live2DExpression?
        private var activeMotion: Live2DMotion?
        private var motionStartTime: TimeInterval = 0
        private var referenceBounds: CGRect = .zero

        deinit {
            if let model {
                topi_live2d_dispose(model)
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
                print("Live2D Metal shader compile failed: \(error)")
                library = nil
            }

            let descriptor = MTLRenderPipelineDescriptor()
            descriptor.vertexFunction = library?.makeFunction(name: "live2d_vertex")
            descriptor.fragmentFunction = library?.makeFunction(name: "live2d_fragment")
            descriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat
            descriptor.stencilAttachmentPixelFormat = view.depthStencilPixelFormat
            descriptor.colorAttachments[0].isBlendingEnabled = true
            descriptor.colorAttachments[0].rgbBlendOperation = .add
            descriptor.colorAttachments[0].alphaBlendOperation = .add
            descriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            descriptor.colorAttachments[0].sourceAlphaBlendFactor = .one
            descriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            descriptor.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha

            do {
                pipelineState = try device.makeRenderPipelineState(descriptor: descriptor)
            } catch {
                print("Live2D Metal pipeline failed: \(error)")
            }

            let maskDescriptor = MTLRenderPipelineDescriptor()
            maskDescriptor.vertexFunction = library?.makeFunction(name: "live2d_vertex")
            maskDescriptor.fragmentFunction = library?.makeFunction(name: "live2d_mask_fragment")
            maskDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat
            maskDescriptor.colorAttachments[0].writeMask = []
            maskDescriptor.stencilAttachmentPixelFormat = view.depthStencilPixelFormat
            do {
                maskPipelineState = try device.makeRenderPipelineState(descriptor: maskDescriptor)
            } catch {
                print("Live2D mask pipeline failed: \(error)")
            }

            maskWriteDepthStencilState = device.makeDepthStencilState(descriptor: Self.makeMaskWriteDepthStencilDescriptor())
            maskedDepthStencilState = device.makeDepthStencilState(descriptor: Self.makeMaskedDepthStencilDescriptor(inverted: false))
            invertedMaskedDepthStencilState = device.makeDepthStencilState(descriptor: Self.makeMaskedDepthStencilDescriptor(inverted: true))
            view.delegate = self
        }

        @MainActor
        func load(character: PetCharacter, in view: MTKView) {
            guard characterID != character.id else { return }
            guard let device = view.device else { return }

            if let model {
                topi_live2d_dispose(model)
                self.model = nil
            }

            let manifestURL: URL?
            if let directory = PetAssetLibrary.resourceDirectory(for: character) {
                manifestURL = directory.appendingPathComponent("\(character.resourceBaseName).json")
            } else {
                manifestURL = AppResourceBundle.module.url(
                    forResource: character.resourceBaseName,
                    withExtension: "json",
                    subdirectory: character.resourceSubdirectory
                )
            }

            guard let manifestURL,
                  let data = try? Data(contentsOf: manifestURL),
                  let manifest = try? JSONDecoder().decode(Live2DModel3Manifest.self, from: data) else {
                resetLoadedState()
                return
            }

            let directory = manifestURL.deletingLastPathComponent()
            let mocURL = directory.appendingPathComponent(manifest.fileReferences.moc)
            guard let nextModel = topi_live2d_create(mocURL.path) else {
                resetLoadedState()
                return
            }

            model = nextModel
            self.manifest = manifest
            modelDirectory = directory
            characterID = character.id
            currentAction = nil
            activeExpression = nil
            activeMotion = nil
            textures = loadTextures(paths: manifest.fileReferences.textures, baseURL: directory, device: device)
            topi_live2d_update(nextModel)
            referenceBounds = currentVertexBounds(model: nextModel)
        }

        func setAction(_ action: String?) {
            guard currentAction != action else { return }
            currentAction = action
            guard let model else { return }

            activeExpression = nil
            activeMotion = nil
            if let action {
                activeExpression = loadExpression(named: action)
                activeMotion = loadMotion(named: action)
                motionStartTime = Date.timeIntervalSinceReferenceDate
            }
            topi_live2d_reset_parameters(model)
            applyActiveExpression(to: model)
            topi_live2d_update(model)
        }

        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}

        func draw(in view: MTKView) {
            guard let model,
                  let pipelineState,
                  let maskPipelineState,
                  let maskWriteDepthStencilState,
                  let maskedDepthStencilState,
                  let invertedMaskedDepthStencilState,
                  let commandQueue,
                  let currentDrawable = view.currentDrawable,
                  let renderPass = view.currentRenderPassDescriptor else {
                return
            }

            topi_live2d_reset_parameters(model)
            applyActiveMotion(to: model)
            applyActiveExpression(to: model)
            applyPointerTracking(to: model, in: view)
            topi_live2d_update(model)

            let vertexCount = Int(topi_live2d_vertex_count(model))
            let indexCount = Int(topi_live2d_index_count(model))
            let commandCount = Int(topi_live2d_command_count(model))
            guard vertexCount > 0, indexCount > 0, commandCount > 0,
                  let vertexPointer = topi_live2d_vertices(model),
                  let indexPointer = topi_live2d_indices(model),
                  let commandPointer = topi_live2d_commands(model),
                  let device = view.device,
                  let vertexBuffer = device.makeBuffer(
                    bytes: vertexPointer,
                    length: MemoryLayout<TopiLive2DVertex>.stride * vertexCount
                  ),
                  let indexBuffer = device.makeBuffer(
                    bytes: indexPointer,
                    length: MemoryLayout<UInt16>.stride * indexCount
                  ) else {
                return
            }

            let commands = UnsafeBufferPointer(start: commandPointer, count: commandCount)
            let commandByDrawable = Dictionary(uniqueKeysWithValues: commands.map { ($0.drawableIndex, $0) })
            let uniforms = makeUniforms(bounds: referenceBounds, drawableSize: view.drawableSize)

            guard let commandBuffer = commandQueue.makeCommandBuffer(),
                  let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPass) else {
                return
            }

            var mutableUniforms = uniforms
            encoder.setVertexBytes(&mutableUniforms, length: MemoryLayout<Live2DUniforms>.stride, index: 1)

            var stencilReference: UInt32 = 1
            for command in commands {
                guard command.textureIndex >= 0, command.textureIndex < textures.count else { continue }
                let masks = maskIndices(from: command)

                if masks.isEmpty {
                    encoder.setRenderPipelineState(pipelineState)
                    encoder.setDepthStencilState(nil)
                } else {
                    encoder.setRenderPipelineState(maskPipelineState)
                    encoder.setDepthStencilState(maskWriteDepthStencilState)
                    encoder.setStencilReferenceValue(stencilReference)

                    for maskDrawableIndex in masks {
                        guard let maskCommand = commandByDrawable[maskDrawableIndex],
                              maskCommand.textureIndex >= 0,
                              maskCommand.textureIndex < textures.count else {
                            continue
                        }
                        encoder.setVertexBuffer(
                            vertexBuffer,
                            offset: Int(maskCommand.vertexOffset) * MemoryLayout<TopiLive2DVertex>.stride,
                            index: 0
                        )
                        encoder.setFragmentTexture(textures[Int(maskCommand.textureIndex)], index: 0)
                        encoder.drawIndexedPrimitives(
                            type: .triangle,
                            indexCount: Int(maskCommand.indexCount),
                            indexType: .uint16,
                            indexBuffer: indexBuffer,
                            indexBufferOffset: Int(maskCommand.indexOffset) * MemoryLayout<UInt16>.stride,
                            instanceCount: 1,
                            baseVertex: 0,
                            baseInstance: 0
                        )
                    }

                    encoder.setRenderPipelineState(pipelineState)
                    encoder.setDepthStencilState(command.invertedMask == 0 ? maskedDepthStencilState : invertedMaskedDepthStencilState)
                    encoder.setStencilReferenceValue(stencilReference)
                    stencilReference = stencilReference == 254 ? 1 : stencilReference + 1
                }

                encoder.setVertexBuffer(
                    vertexBuffer,
                    offset: Int(command.vertexOffset) * MemoryLayout<TopiLive2DVertex>.stride,
                    index: 0
                )
                encoder.setFragmentTexture(textures[Int(command.textureIndex)], index: 0)
                encoder.drawIndexedPrimitives(
                    type: .triangle,
                    indexCount: Int(command.indexCount),
                    indexType: .uint16,
                    indexBuffer: indexBuffer,
                    indexBufferOffset: Int(command.indexOffset) * MemoryLayout<UInt16>.stride,
                    instanceCount: 1,
                    baseVertex: 0,
                    baseInstance: 0
                )
            }

            encoder.endEncoding()
            commandBuffer.present(currentDrawable)
            commandBuffer.commit()
        }

        private func maskIndices(from command: TopiLive2DCommand) -> [Int32] {
            guard command.maskCount > 0 else { return [] }
            return withUnsafeBytes(of: command.maskIndices) { bytes in
                let values = bytes.bindMemory(to: Int32.self)
                return (0..<min(Int(command.maskCount), values.count)).compactMap { index in
                    let value = values[index]
                    return value >= 0 ? value : nil
                }
            }
        }

        private func resetLoadedState() {
            characterID = nil
            manifest = nil
            modelDirectory = nil
            textures = []
            currentAction = nil
            activeExpression = nil
            activeMotion = nil
            referenceBounds = .zero
        }

        @MainActor
        private func applyPointerTracking(to model: OpaquePointer, in view: MTKView) {
            guard let window = view.window else { return }

            let mouse = NSEvent.mouseLocation
            let frame = window.frame
            let normalizedX = max(-1, min(1, (mouse.x - frame.midX) / max(frame.width * 0.5, 1)))
            let normalizedY = max(-1, min(1, (mouse.y - frame.midY) / max(frame.height * 0.5, 1)))

            setParameter(model, aliases: ["ParamEyeBallX", "PARAM_EYE_BALL_X"], value: Float(normalizedX))
            setParameter(model, aliases: ["ParamEyeBallY", "PARAM_EYE_BALL_Y"], value: Float(normalizedY))
            setParameter(model, aliases: ["ParamAngleX", "PARAM_ANGLE_X"], value: Float(normalizedX * 22))
            setParameter(model, aliases: ["ParamAngleY", "PARAM_ANGLE_Y"], value: Float(normalizedY * 16))
            setParameter(model, aliases: ["ParamBodyAngleX", "PARAM_BODY_ANGLE_X"], value: Float(normalizedX * 8))
            setParameter(model, aliases: ["ParamBodyAngleY", "PARAM_BODY_ANGLE_Y"], value: Float(normalizedY * 6))
        }

        private func loadTextures(paths: [String], baseURL: URL, device: MTLDevice) -> [MTLTexture] {
            let loader = MTKTextureLoader(device: device)
            return paths.compactMap { path in
                try? loader.newTexture(URL: baseURL.appendingPathComponent(path), options: [
                    .SRGB: false,
                    .textureUsage: NSNumber(value: MTLTextureUsage.shaderRead.rawValue)
                ])
            }
        }

        private func setParameter(_ model: OpaquePointer, aliases: [String], value: Float) {
            for alias in aliases {
                topi_live2d_set_parameter(model, alias, value)
            }
        }

        private func loadExpression(named name: String) -> Live2DExpression? {
            guard let expression = manifest?.fileReferences.expressions?.first(where: { $0.name == name }),
                  let modelDirectory,
                  let data = try? Data(contentsOf: modelDirectory.appendingPathComponent(expression.file)),
                  let payload = try? JSONDecoder().decode(Live2DExpression.self, from: data) else {
                return nil
            }
            return payload
        }

        private func applyActiveExpression(to model: OpaquePointer) {
            guard let activeExpression else { return }
            for parameter in activeExpression.parameters {
                topi_live2d_set_parameter(model, parameter.id, Float(parameter.value))
            }
        }

        private func loadMotion(named name: String) -> Live2DMotion? {
            guard let motionReference = motionReference(named: name),
                  let modelDirectory,
                  let data = try? Data(contentsOf: modelDirectory.appendingPathComponent(motionReference.file)) else {
                return nil
            }
            return try? JSONDecoder().decode(Live2DMotion.self, from: data)
        }

        private func motionReference(named name: String) -> Live2DModel3Manifest.Motion? {
            guard let motions = manifest?.fileReferences.motions else { return nil }
            for (group, groupMotions) in motions {
                for (index, motion) in groupMotions.enumerated() {
                    let actionName = group.isEmpty ? "motion-\(index + 1)" : "\(group)-\(index + 1)"
                    if actionName == name {
                        return motion
                    }
                }
            }
            if let groupMotions = motions[name], let first = groupMotions.first {
                return first
            }
            return nil
        }

        private func applyActiveMotion(to model: OpaquePointer) {
            guard let activeMotion else { return }
            let duration = max(activeMotion.meta.duration, 0.001)
            let elapsed = Date.timeIntervalSinceReferenceDate - motionStartTime
            let time: Double
            if activeMotion.meta.loop {
                time = elapsed.truncatingRemainder(dividingBy: duration)
            } else {
                time = min(elapsed, duration)
            }

            for curve in activeMotion.curves where curve.target == "Parameter" {
                guard let value = curve.value(at: time) else { continue }
                topi_live2d_set_parameter(model, curve.id, Float(value))
            }
        }

        private func currentVertexBounds(model: OpaquePointer) -> CGRect {
            let count = Int(topi_live2d_vertex_count(model))
            guard count > 0, let vertices = topi_live2d_vertices(model) else {
                return CGRect(x: -1, y: -1, width: 2, height: 2)
            }

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

            return CGRect(x: minX, y: minY, width: max(0.01, maxX - minX), height: max(0.01, maxY - minY))
        }

        private func makeUniforms(bounds: CGRect, drawableSize: CGSize) -> Live2DUniforms {
            let padding: CGFloat = 4
            let availableWidth = max(1, drawableSize.width - padding * 2)
            let availableHeight = max(1, drawableSize.height - padding * 2)
            let scale = min(availableWidth / bounds.width, availableHeight / bounds.height)
            let fittedWidth = bounds.width * scale
            let fittedHeight = bounds.height * scale
            let offset = SIMD2<Float>(
                Float((drawableSize.width - fittedWidth) / 2),
                Float((drawableSize.height - fittedHeight) / 2)
            )

            return Live2DUniforms(
                viewportWidth: Float(drawableSize.width),
                viewportHeight: Float(drawableSize.height),
                contentMinX: Float(bounds.minX),
                contentMinY: Float(bounds.minY),
                contentMaxX: Float(bounds.maxX),
                contentMaxY: Float(bounds.maxY),
                contentScale: Float(scale),
                contentOffsetX: offset.x,
                contentOffsetY: offset.y
            )
        }

        private static let shaderSource = """
        #include <metal_stdlib>
        using namespace metal;

        struct Live2DVertexIn {
            float2 position;
            float2 uv;
            float4 color;
        };

        struct Live2DUniforms {
            float viewportWidth;
            float viewportHeight;
            float contentMinX;
            float contentMinY;
            float contentMaxX;
            float contentMaxY;
            float contentScale;
            float contentOffsetX;
            float contentOffsetY;
        };

        struct Live2DVertexOut {
            float4 position [[position]];
            float2 uv;
            float4 color;
        };

        vertex Live2DVertexOut live2d_vertex(
            const device Live2DVertexIn *vertices [[buffer(0)]],
            constant Live2DUniforms &uniforms [[buffer(1)]],
            uint vertexID [[vertex_id]]
        ) {
            Live2DVertexIn input = vertices[vertexID];
            float2 contentMin = float2(uniforms.contentMinX, uniforms.contentMinY);
            float2 contentOffset = float2(uniforms.contentOffsetX, uniforms.contentOffsetY);
            float2 rotated = float2(
                uniforms.contentMinX + uniforms.contentMaxX - input.position.x,
                uniforms.contentMinY + uniforms.contentMaxY - input.position.y
            );
            float2 fitted = (rotated - contentMin) * uniforms.contentScale + contentOffset;
            float2 clip;
            clip.x = fitted.x / uniforms.viewportWidth * 2.0 - 1.0;
            clip.y = 1.0 - fitted.y / uniforms.viewportHeight * 2.0;

            Live2DVertexOut output;
            output.position = float4(clip, 0.0, 1.0);
            output.uv = input.uv;
            output.color = input.color;
            return output;
        }

        fragment float4 live2d_fragment(
            Live2DVertexOut input [[stage_in]],
            texture2d<float> texture [[texture(0)]]
        ) {
            constexpr sampler textureSampler(mag_filter::linear, min_filter::linear, address::clamp_to_edge);
            return texture.sample(textureSampler, input.uv) * input.color;
        }

        fragment void live2d_mask_fragment(
            Live2DVertexOut input [[stage_in]],
            texture2d<float> texture [[texture(0)]]
        ) {
            constexpr sampler textureSampler(mag_filter::linear, min_filter::linear, address::clamp_to_edge);
            float alpha = texture.sample(textureSampler, input.uv).a * input.color.a;
            if (alpha < 0.05) {
                discard_fragment();
            }
        }
        """

        private static func makeMaskWriteDepthStencilDescriptor() -> MTLDepthStencilDescriptor {
            let descriptor = MTLDepthStencilDescriptor()
            let stencil = MTLStencilDescriptor()
            stencil.stencilCompareFunction = .always
            stencil.stencilFailureOperation = .keep
            stencil.depthFailureOperation = .keep
            stencil.depthStencilPassOperation = .replace
            stencil.readMask = 0xff
            stencil.writeMask = 0xff
            descriptor.frontFaceStencil = stencil
            descriptor.backFaceStencil = stencil
            return descriptor
        }

        private static func makeMaskedDepthStencilDescriptor(inverted: Bool) -> MTLDepthStencilDescriptor {
            let descriptor = MTLDepthStencilDescriptor()
            let stencil = MTLStencilDescriptor()
            stencil.stencilCompareFunction = inverted ? .notEqual : .equal
            stencil.stencilFailureOperation = .keep
            stencil.depthFailureOperation = .keep
            stencil.depthStencilPassOperation = .keep
            stencil.readMask = 0xff
            stencil.writeMask = 0x00
            descriptor.frontFaceStencil = stencil
            descriptor.backFaceStencil = stencil
            return descriptor
        }
    }
}

private struct Live2DExpression: Decodable {
    let parameters: [Parameter]

    enum CodingKeys: String, CodingKey {
        case parameters = "Parameters"
    }

    struct Parameter: Decodable {
        let id: String
        let value: Double

        enum CodingKeys: String, CodingKey {
            case id = "Id"
            case value = "Value"
        }
    }
}

private struct Live2DMotion: Decodable {
    let meta: Meta
    let curves: [Curve]

    enum CodingKeys: String, CodingKey {
        case meta = "Meta"
        case curves = "Curves"
    }

    struct Meta: Decodable {
        let duration: Double
        let loop: Bool

        enum CodingKeys: String, CodingKey {
            case duration = "Duration"
            case loop = "Loop"
        }
    }

    struct Curve: Decodable {
        let target: String
        let id: String
        let segments: [Double]

        enum CodingKeys: String, CodingKey {
            case target = "Target"
            case id = "Id"
            case segments = "Segments"
        }

        func value(at time: Double) -> Double? {
            guard segments.count >= 2 else { return nil }
            var previous = MotionPoint(time: segments[0], value: segments[1])
            guard segments.count > 2 else { return previous.value }

            var index = 2
            while index < segments.count {
                let segmentType = Int(segments[index])
                index += 1

                switch segmentType {
                case 0:
                    guard index + 1 < segments.count else { return previous.value }
                    let next = MotionPoint(time: segments[index], value: segments[index + 1])
                    if time <= next.time {
                        return interpolateLinear(from: previous, to: next, at: time)
                    }
                    previous = next
                    index += 2
                case 1:
                    guard index + 5 < segments.count else { return previous.value }
                    let control1 = MotionPoint(time: segments[index], value: segments[index + 1])
                    let control2 = MotionPoint(time: segments[index + 2], value: segments[index + 3])
                    let next = MotionPoint(time: segments[index + 4], value: segments[index + 5])
                    if time <= next.time {
                        return interpolateBezier(from: previous, control1: control1, control2: control2, to: next, at: time)
                    }
                    previous = next
                    index += 6
                case 2:
                    guard index + 1 < segments.count else { return previous.value }
                    let next = MotionPoint(time: segments[index], value: segments[index + 1])
                    if time <= next.time {
                        return previous.value
                    }
                    previous = next
                    index += 2
                case 3:
                    guard index + 1 < segments.count else { return previous.value }
                    let next = MotionPoint(time: segments[index], value: segments[index + 1])
                    if time <= next.time {
                        return next.value
                    }
                    previous = next
                    index += 2
                default:
                    return previous.value
                }
            }

            return previous.value
        }

        private func interpolateLinear(from start: MotionPoint, to end: MotionPoint, at time: Double) -> Double {
            let range = end.time - start.time
            guard range > 0 else { return end.value }
            let progress = max(0, min(1, (time - start.time) / range))
            return start.value + (end.value - start.value) * progress
        }

        private func interpolateBezier(
            from start: MotionPoint,
            control1: MotionPoint,
            control2: MotionPoint,
            to end: MotionPoint,
            at time: Double
        ) -> Double {
            let range = end.time - start.time
            guard range > 0 else { return end.value }
            let x = max(0, min(1, (time - start.time) / range))
            let t = solveBezierT(forX: x, start: start, control1: control1, control2: control2, end: end)
            let inv = 1 - t
            return inv * inv * inv * start.value
                + 3 * inv * inv * t * control1.value
                + 3 * inv * t * t * control2.value
                + t * t * t * end.value
        }

        private func solveBezierT(forX x: Double, start: MotionPoint, control1: MotionPoint, control2: MotionPoint, end: MotionPoint) -> Double {
            let range = end.time - start.time
            guard range > 0 else { return x }
            let c1x = (control1.time - start.time) / range
            let c2x = (control2.time - start.time) / range

            var low = 0.0
            var high = 1.0
            var t = x
            for _ in 0..<8 {
                t = (low + high) * 0.5
                let inv = 1 - t
                let sample = 3 * inv * inv * t * c1x + 3 * inv * t * t * c2x + t * t * t
                if sample < x {
                    low = t
                } else {
                    high = t
                }
            }
            return t
        }

        private struct MotionPoint {
            let time: Double
            let value: Double
        }
    }
}

private final class TransparentLive2DMTKView: MTKView {
    override var isOpaque: Bool {
        false
    }
}

private struct Live2DUniforms {
    var viewportWidth: Float
    var viewportHeight: Float
    var contentMinX: Float
    var contentMinY: Float
    var contentMaxX: Float
    var contentMaxY: Float
    var contentScale: Float
    var contentOffsetX: Float
    var contentOffsetY: Float
}
