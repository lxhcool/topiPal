import Foundation

enum AppResourceBundle {
    static let module: Bundle = {
        let bundleName = "TopiPal_TopiPal.bundle"
        let executableDirectory = CommandLine.arguments.first
            .map { URL(fileURLWithPath: $0).deletingLastPathComponent() }

        let candidates: [URL?] = [
            Bundle.main.resourceURL?.appendingPathComponent(bundleName),
            Bundle.main.bundleURL.appendingPathComponent(bundleName),
            executableDirectory?.appendingPathComponent(bundleName)
        ]

        for url in candidates.compactMap({ $0 }) {
            if let bundle = Bundle(url: url) {
                return bundle
            }
        }

        return Bundle.main
    }()
}
