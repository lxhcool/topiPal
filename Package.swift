// swift-tools-version: 6.0

import PackageDescription

let package = Package(
    name: "TopiPal",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        .executable(name: "TopiPal", targets: ["TopiPal"])
    ],
    targets: [
        .target(
            name: "SpineC",
            path: "Sources/SpineC",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("include"),
                .headerSearchPath("src")
            ]
        ),
        .target(
            name: "SpineC40",
            path: "Sources/SpineC40",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("include"),
                .headerSearchPath("src")
            ]
        ),
        .target(
            name: "Live2DCore",
            path: "Sources/Live2DCore",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("include")
            ],
            linkerSettings: [
                .unsafeFlags(["-LSources/Live2DCore/lib/macos/arm64"]),
                .linkedLibrary("Live2DCubismCore")
            ]
        ),
        .executableTarget(
            name: "TopiPal",
            dependencies: ["SpineC", "SpineC40", "Live2DCore"],
            path: "Sources/TopiPal",
            resources: [
                .copy("Resources")
            ],
            linkerSettings: [
                .linkedLibrary("sqlite3")
            ]
        )
    ]
)
