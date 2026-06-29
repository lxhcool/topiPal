import AppKit
import SwiftUI

struct SequenceFrameView: View {
    let character: PetCharacter
    let action: String?

    @State private var frames: [NSImage] = []
    @State private var frameIndex = 0

    private let frameDuration = 1.0 / 12.0

    var body: some View {
        TimelineView(.animation(minimumInterval: frameDuration, paused: frames.isEmpty)) { timeline in
            ZStack {
                if let image = currentImage(at: timeline.date) {
                    Image(nsImage: image)
                        .resizable()
                        .interpolation(.high)
                        .scaledToFit()
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .task(id: "\(character.id)-\(action ?? "")") {
            loadFrames()
        }
    }

    private func currentImage(at date: Date) -> NSImage? {
        guard !frames.isEmpty else { return nil }
        let tick = Int(date.timeIntervalSinceReferenceDate / frameDuration)
        return frames[tick % frames.count]
    }

    @MainActor
    private func loadFrames() {
        let urls = PetAssetLibrary.sequenceFrameURLs(for: character, action: action)
        frames = urls.compactMap { NSImage(contentsOf: $0) }
        frameIndex = 0
    }
}
