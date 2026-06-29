import CoreGraphics
import Foundation

@MainActor
final class SettingsStore: ObservableObject {
    @Published var compactSize = CGSize(width: 448, height: 468)
    @Published var expandedSize = CGSize(width: 468, height: 548)
    @Published var topInset: CGFloat = 10
    @Published var animationIntensity: Double = 1
    @Published var petScale: Double = 0.8 {
        didSet {
            UserDefaults.standard.set(petScale, forKey: Self.petScaleKey)
        }
    }
    @Published var flowingAnimationEnabled = true
    @Published var savedWindowAnchor: CGPoint? {
        didSet {
            if let savedWindowAnchor {
                UserDefaults.standard.set(Double(savedWindowAnchor.x), forKey: Self.anchorXKey)
                UserDefaults.standard.set(Double(savedWindowAnchor.y), forKey: Self.anchorYKey)
            }
        }
    }

    private static let anchorXKey = "Pet.WindowAnchorX"
    private static let anchorYKey = "Pet.WindowAnchorY"
    private static let petScaleKey = "Pet.Scale"

    init() {
        let savedPetScale = UserDefaults.standard.double(forKey: Self.petScaleKey)
        if savedPetScale > 0 {
            petScale = min(max(savedPetScale, 0.72), 1.28)
        }

        if UserDefaults.standard.object(forKey: Self.anchorXKey) != nil,
           UserDefaults.standard.object(forKey: Self.anchorYKey) != nil {
            savedWindowAnchor = CGPoint(
                x: UserDefaults.standard.double(forKey: Self.anchorXKey),
                y: UserDefaults.standard.double(forKey: Self.anchorYKey)
            )
        }
    }
}
