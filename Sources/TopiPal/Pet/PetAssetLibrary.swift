import Foundation
import SpineC
import SpineC40

enum PetRendererKind: String, Codable {
    case sequence
    case spine
    case spine40
    case live2D

    var displayName: String {
        switch self {
        case .sequence: "序列帧"
        case .spine: "Spine"
        case .spine40: "Spine 4.0"
        case .live2D: "Live2D"
        }
    }
}

enum PetHitArea: String, CaseIterable, Codable, Sendable {
    case head
    case body
    case left
    case right
    case feet

    var displayName: String {
        switch self {
        case .head: "头部"
        case .body: "身体"
        case .left: "左侧"
        case .right: "右侧"
        case .feet: "下方"
        }
    }
}

struct PetCharacter: Identifiable, Equatable {
    let id: String
    let name: String
    let rendererKind: PetRendererKind
    let resourceBaseName: String
    let resourceSubdirectory: String
    let resourceDirectoryPath: String?
    let previewColumn: Int
    let previewRow: Int
    let actions: [String]

    var isCustom: Bool {
        resourceDirectoryPath != nil
    }

    var defaultAction: String? {
        actions.first { $0.localizedCaseInsensitiveContains("idle") }
    }

    func action(for area: PetHitArea) -> String? {
        guard !actions.isEmpty else { return nil }
        if rendererKind == .live2D {
            return live2DAction(for: area)
        }
        switch area {
        case .head:
            return findAnim(containing: ["WaveHands", "Thinking", "Cheer", "WaveClaws", "Clap"])
        case .body:
            return findAnim(containing: ["Cheer", "Clap", "CrossArms", "Transporting", "Aerobics", "Meditation", "WaveClaws"])
        case .left:
            return findAnim(containing: ["Clap", "Cheer", "Transporting", "Aerobics", "walking_left", "left"])
        case .right:
            return findAnim(containing: ["WaveHands", "Thinking", "CrossArms", "Meditation", "right"])
        case .feet:
            return findAnim(containing: ["walking_default", "Aerobics", "Meditation", "walking_left"])
        }
    }

    private func findAnim(containing keywords: [String]) -> String? {
        for keyword in keywords {
            if let match = actions.first(where: { $0.localizedCaseInsensitiveContains(keyword) }) {
                return match
            }
        }
        return nil
    }

    private func live2DAction(for area: PetHitArea) -> String? {
        let preferred: [String]
        switch area {
        case .head:
            preferred = ["4OAO", "5QAQ", "3clever", "Tap", "FlickUp", "Flick3"]
        case .body:
            preferred = ["1desk", "2mic", "7keyboard", "Tap", "Flick", "Shake"]
        case .left:
            preferred = ["6i gi a ri", "3clever", "FlickLeft", "Flick", "Tap"]
        case .right:
            preferred = ["8punch", "9", "FlickRight", "Flick", "Tap"]
        case .feet:
            preferred = ["7keyboard", "1desk", "FlickDown", "Shake", "Tap"]
        }

        for name in preferred {
            if actions.contains(name) {
                return name
            }
            if let match = actions.first(where: { $0.localizedCaseInsensitiveContains("\(name)-") }) {
                return match
            }
        }
        return nil
    }
}

enum PetAssetLibrary {
    static var characters: [PetCharacter] {
        builtInCharacters + customCharacters()
    }

    static var builtInCharacters: [PetCharacter] {
        var characters = [
        PetCharacter(
            id: "seele-spine-01",
            name: "Seele 01",
            rendererKind: .spine,
            resourceBaseName: "SeeleSpine01",
            resourceSubdirectory: "Resources/Pets/Seele",
            resourceDirectoryPath: nil,
            previewColumn: 0,
            previewRow: 0,
            actions: discoveredSpineActions(resourceBaseName: "SeeleSpine01", subdirectory: "Resources/Pets/Seele")
        ),
        PetCharacter(
            id: "seele-spine-02",
            name: "Seele 02",
            rendererKind: .spine,
            resourceBaseName: "SeeleSpine02",
            resourceSubdirectory: "Resources/Pets/Seele",
            resourceDirectoryPath: nil,
            previewColumn: 1,
            previewRow: 0,
            actions: discoveredSpineActions(resourceBaseName: "SeeleSpine02", subdirectory: "Resources/Pets/Seele")
        ),
        PetCharacter(
            id: "seele-spine-03",
            name: "Seele 03",
            rendererKind: .spine,
            resourceBaseName: "SeeleSpine03",
            resourceSubdirectory: "Resources/Pets/Seele",
            resourceDirectoryPath: nil,
            previewColumn: 0,
            previewRow: 1,
            actions: discoveredSpineActions(resourceBaseName: "SeeleSpine03", subdirectory: "Resources/Pets/Seele")
        ),
        PetCharacter(
            id: "seele-spine-04",
            name: "Seele 04",
            rendererKind: .spine,
            resourceBaseName: "SeeleSpine04",
            resourceSubdirectory: "Resources/Pets/Seele",
            resourceDirectoryPath: nil,
            previewColumn: 1,
            previewRow: 1,
            actions: discoveredSpineActions(resourceBaseName: "SeeleSpine04", subdirectory: "Resources/Pets/Seele")
        ),
        PetCharacter(
            id: "wanko-live2d",
            name: "Wanko",
            rendererKind: .live2D,
            resourceBaseName: "wanko_touch.model3",
            resourceSubdirectory: "Resources/Pets/Wanko",
            resourceDirectoryPath: nil,
            previewColumn: 0,
            previewRow: 0,
            actions: discoveredLive2DActions(modelFileName: "wanko_touch.model3", subdirectory: "Resources/Pets/Wanko")
        )
        ]

        let monthlyCardActions = discoveredSpine40Actions(
            resourceBaseName: "Eff_UI_MonthlyCardPageV2_Spine",
            subdirectory: "Resources/Pets/MonthlyCardGirl"
        )
        if !monthlyCardActions.isEmpty {
            characters.append(
                PetCharacter(
                    id: "monthly-card-girl-spine",
                    name: "少女月卡",
                    rendererKind: .spine40,
                    resourceBaseName: "Eff_UI_MonthlyCardPageV2_Spine",
                    resourceSubdirectory: "Resources/Pets/MonthlyCardGirl",
                    resourceDirectoryPath: nil,
                    previewColumn: 0,
                    previewRow: 0,
                    actions: monthlyCardActions
                )
            )
        }

        for index in 1...6 {
            let baseName = "elvisa_\(index)"
            let actions = discoveredSpineActions(
                resourceBaseName: baseName,
                subdirectory: "Resources/Pets/Elysia"
            )
            if !actions.isEmpty {
                characters.append(
                    PetCharacter(
                        id: "elysia-spine-\(index)",
                        name: "Elysia \(index)",
                        rendererKind: .spine,
                        resourceBaseName: baseName,
                        resourceSubdirectory: "Resources/Pets/Elysia",
                        resourceDirectoryPath: nil,
                        previewColumn: 0,
                        previewRow: 0,
                        actions: actions
                    )
                )
            }
        }

        return characters
    }

    static func importCharacter(kind: PetRendererKind, from directory: URL) throws -> PetCharacter {
        let character: PetCharacter
        switch kind {
        case .sequence:
            character = try makeExternalSequenceCharacter(from: directory)
        case .spine:
            character = try makeExternalSpineCharacter(from: directory)
        case .spine40:
            throw PetImportError.unsupported("Spine 4.0 角色当前先作为内置兼容资源接入。")
        case .live2D:
            character = try makeExternalLive2DCharacter(from: directory)
        }

        var records = customRecords().filter { $0.id != character.id }
        records.append(CustomPetRecord(character: character))
        saveCustomRecords(records)
        return character
    }

    static func deleteCustomCharacter(id: String) {
        let records = customRecords().filter { $0.id != id }
        saveCustomRecords(records)
    }

    static func resourceDirectory(for character: PetCharacter) -> URL? {
        if let resourceDirectoryPath = character.resourceDirectoryPath {
            return URL(fileURLWithPath: resourceDirectoryPath, isDirectory: true)
        }
        return AppResourceBundle.module.url(forResource: character.resourceSubdirectory, withExtension: nil)
    }

    static func previewImageURL(for character: PetCharacter) -> URL? {
        guard let directory = resourceDirectory(for: character) else { return nil }

        // 1. 优先使用专用的预览图
        for name in ["preview.png", "icon.png"] {
            let url = directory.appendingPathComponent(name)
            if FileManager.default.fileExists(atPath: url.path) {
                return url
            }
        }

        // 2. 序列帧：第一帧就是角色渲染图，可以直接用
        switch character.rendererKind {
        case .sequence:
            let idleDir = directory.appendingPathComponent("idle", isDirectory: true)
            if let urls = try? FileManager.default.contentsOfDirectory(at: idleDir, includingPropertiesForKeys: nil),
               let first = urls.first(where: { $0.pathExtension.lowercased() == "png" }) {
                return first
            }
            if let actionDirs = try? FileManager.default.contentsOfDirectory(at: directory, includingPropertiesForKeys: [.isDirectoryKey]),
               let firstDir = actionDirs.first(where: { (try? $0.resourceValues(forKeys: [.isDirectoryKey]).isDirectory) == true }),
               let urls = try? FileManager.default.contentsOfDirectory(at: firstDir, includingPropertiesForKeys: nil),
               let first = urls.first(where: { $0.pathExtension.lowercased() == "png" }) {
                return first
            }
            return nil

        case .spine, .spine40, .live2D:
            // Spine/Live2D 的贴图是雪碧图，不适合做预览
            // 没有专用预览图时返回 nil，UI 显示占位符
            return nil
        }
    }

    private static func discoveredSpineActions(resourceBaseName: String, subdirectory: String) -> [String] {
        guard let atlasURL = AppResourceBundle.module.url(
            forResource: resourceBaseName,
            withExtension: "atlas",
            subdirectory: subdirectory
        ),
              let skeletonURL = AppResourceBundle.module.url(
            forResource: resourceBaseName,
            withExtension: "skel",
            subdirectory: subdirectory
        ),
              let drawable = topi_spine_create(atlasURL.path, skeletonURL.path, 1.0) else {
            return []
        }

        defer { topi_spine_dispose(drawable) }

        let count = Int(topi_spine_animation_count(drawable))
        guard count > 0 else { return [] }

        return (0..<count).compactMap { index in
            guard let name = topi_spine_animation_name(drawable, Int32(index)) else {
                return nil
            }
            return String(cString: name)
        }
    }

    private static func discoveredSpineActions(atlasURL: URL, skeletonURL: URL) -> [String] {
        guard let drawable = topi_spine_create(atlasURL.path, skeletonURL.path, 1.0) else {
            return []
        }
        defer { topi_spine_dispose(drawable) }
        let count = Int(topi_spine_animation_count(drawable))
        return (0..<count).compactMap { index in
            guard let name = topi_spine_animation_name(drawable, Int32(index)) else { return nil }
            return String(cString: name)
        }
    }

    private static func discoveredSpine40Actions(resourceBaseName: String, subdirectory: String) -> [String] {
        guard let atlasURL = AppResourceBundle.module.url(
            forResource: resourceBaseName,
            withExtension: "atlas",
            subdirectory: subdirectory
        ),
              let skeletonURL = AppResourceBundle.module.url(
                forResource: resourceBaseName,
                withExtension: "skel",
                subdirectory: subdirectory
              ),
              let drawable = topi_spine40_create(atlasURL.path, skeletonURL.path, 1.0) else {
            return []
        }
        defer { topi_spine40_dispose(drawable) }
        let count = Int(topi_spine40_animation_count(drawable))
        return (0..<count).compactMap { index in
            guard let name = topi_spine40_animation_name(drawable, Int32(index)) else { return nil }
            return String(cString: name)
        }
    }

    private static func discoveredLive2DActions(modelFileName: String, subdirectory: String) -> [String] {
        let url = AppResourceBundle.module.url(forResource: modelFileName, withExtension: "json", subdirectory: subdirectory)
        return discoveredLive2DActions(manifestURL: url)
    }

    private static func discoveredLive2DActions(manifestURL: URL?) -> [String] {
        guard let url = manifestURL,
              let data = try? Data(contentsOf: url),
              let model = try? JSONDecoder().decode(Live2DModel3Manifest.self, from: data) else {
            return []
        }

        let expressionNames: [String] = model.fileReferences.expressions?.map(\.name) ?? []
        let motionNames = model.fileReferences.motions?.flatMap { group, motions in
            motions.enumerated().map { index, _ in
                group.isEmpty ? "motion-\(index + 1)" : "\(group)-\(index + 1)"
            }
        } ?? []
        return Array(NSOrderedSet(array: expressionNames + motionNames)) as? [String] ?? expressionNames + motionNames
    }

    private static func customCharacters() -> [PetCharacter] {
        customRecords().compactMap { record in
            let directory = URL(fileURLWithPath: record.resourceDirectoryPath, isDirectory: true)
            switch record.rendererKind {
            case .sequence:
                let actions = discoveredSequenceActions(in: directory)
                guard !actions.isEmpty else { return nil }
                return record.character(actions: actions)
            case .spine:
                guard let atlasURL = directory.appendingPathComponent("\(record.resourceBaseName).atlas").existingFileURL,
                      let skeletonURL = directory.appendingPathComponent("\(record.resourceBaseName).skel").existingFileURL else {
                    return nil
                }
                let actions = discoveredSpineActions(atlasURL: atlasURL, skeletonURL: skeletonURL)
                guard !actions.isEmpty else { return nil }
                return record.character(actions: actions)
            case .spine40:
                return nil
            case .live2D:
                let manifestURL = directory.appendingPathComponent("\(record.resourceBaseName).json")
                let actions = discoveredLive2DActions(manifestURL: manifestURL)
                guard !actions.isEmpty else { return nil }
                return record.character(actions: actions)
            }
        }
    }

    static func sequenceFrameURLs(for character: PetCharacter, action: String?) -> [URL] {
        guard character.rendererKind == .sequence,
              let directory = resourceDirectory(for: character) else {
            return []
        }

        let resolvedAction = action
            ?? character.defaultAction
            ?? character.actions.first
        guard let resolvedAction else { return [] }

        return pngFrameURLs(in: directory.appendingPathComponent(resolvedAction, isDirectory: true))
    }

    private static func makeExternalSequenceCharacter(from directory: URL) throws -> PetCharacter {
        let actions = discoveredSequenceActions(in: directory)
        guard !actions.isEmpty else {
            throw PetImportError.invalidResource("没有找到有效动作目录。序列帧格式需要：角色目录 / 动作目录 / PNG 帧。")
        }

        return PetCharacter(
            id: "external-sequence-\(stableID(for: directory.path))",
            name: directory.lastPathComponent,
            rendererKind: .sequence,
            resourceBaseName: directory.lastPathComponent,
            resourceSubdirectory: "",
            resourceDirectoryPath: directory.path,
            previewColumn: 0,
            previewRow: 0,
            actions: actions
        )
    }

    private static func discoveredSequenceActions(in directory: URL) -> [String] {
        guard let actionDirectories = try? FileManager.default.contentsOfDirectory(
            at: directory,
            includingPropertiesForKeys: [.isDirectoryKey],
            options: [.skipsHiddenFiles]
        ) else {
            return []
        }

        return actionDirectories
            .filter { url in
                (try? url.resourceValues(forKeys: [.isDirectoryKey]).isDirectory) == true
            }
            .filter { !pngFrameURLs(in: $0).isEmpty }
            .map(\.lastPathComponent)
            .sorted { lhs, rhs in
                if lhs.localizedCaseInsensitiveContains("idle") { return true }
                if rhs.localizedCaseInsensitiveContains("idle") { return false }
                return lhs.localizedStandardCompare(rhs) == .orderedAscending
            }
    }

    private static func pngFrameURLs(in directory: URL) -> [URL] {
        guard let files = try? FileManager.default.contentsOfDirectory(
            at: directory,
            includingPropertiesForKeys: [.isRegularFileKey],
            options: [.skipsHiddenFiles]
        ) else {
            return []
        }

        return files
            .filter { url in
                url.pathExtension.localizedCaseInsensitiveCompare("png") == .orderedSame
            }
            .sorted { $0.lastPathComponent.localizedStandardCompare($1.lastPathComponent) == .orderedAscending }
    }

    private static func makeExternalSpineCharacter(from directory: URL) throws -> PetCharacter {
        let files = try FileManager.default.contentsOfDirectory(at: directory, includingPropertiesForKeys: nil)
        guard let atlasURL = files.first(where: { $0.pathExtension == "atlas" }) else {
            throw PetImportError.invalidResource("没有找到 .atlas 文件。")
        }

        let baseName = atlasURL.deletingPathExtension().lastPathComponent
        let skeletonURL = directory.appendingPathComponent("\(baseName).skel")
        guard FileManager.default.fileExists(atPath: skeletonURL.path) else {
            throw PetImportError.invalidResource("当前 Spine runtime 只支持二进制 .skel，未找到 \(baseName).skel。")
        }

        let actions = discoveredSpineActions(atlasURL: atlasURL, skeletonURL: skeletonURL)
        guard !actions.isEmpty else {
            throw PetImportError.invalidResource("Spine 文件可以找到，但 runtime 没有读到动作。")
        }

        return PetCharacter(
            id: "external-spine-\(stableID(for: directory.path))",
            name: directory.lastPathComponent,
            rendererKind: .spine,
            resourceBaseName: baseName,
            resourceSubdirectory: "",
            resourceDirectoryPath: directory.path,
            previewColumn: 0,
            previewRow: 0,
            actions: actions
        )
    }

    private static func makeExternalLive2DCharacter(from directory: URL) throws -> PetCharacter {
        let files = try FileManager.default.contentsOfDirectory(at: directory, includingPropertiesForKeys: nil)
        guard let manifestURL = files.first(where: { $0.lastPathComponent.lowercased().hasSuffix(".model3.json") }) else {
            throw PetImportError.invalidResource("没有找到 .model3.json 文件。")
        }

        let data = try Data(contentsOf: manifestURL)
        let manifest = try JSONDecoder().decode(Live2DModel3Manifest.self, from: data)
        let mocURL = directory.appendingPathComponent(manifest.fileReferences.moc)
        guard FileManager.default.fileExists(atPath: mocURL.path) else {
            throw PetImportError.invalidResource("model3.json 指向的 moc3 不存在：\(manifest.fileReferences.moc)。")
        }
        for texture in manifest.fileReferences.textures {
            guard FileManager.default.fileExists(atPath: directory.appendingPathComponent(texture).path) else {
                throw PetImportError.invalidResource("model3.json 指向的贴图不存在：\(texture)。")
            }
        }

        let actions = discoveredLive2DActions(manifestURL: manifestURL)
        guard !actions.isEmpty else {
            throw PetImportError.invalidResource("Live2D 模型没有声明 expressions 或 motions。")
        }

        return PetCharacter(
            id: "external-live2d-\(stableID(for: directory.path))",
            name: directory.lastPathComponent,
            rendererKind: .live2D,
            resourceBaseName: manifestURL.deletingPathExtension().lastPathComponent,
            resourceSubdirectory: "",
            resourceDirectoryPath: directory.path,
            previewColumn: 0,
            previewRow: 0,
            actions: actions
        )
    }

    private static func customRecords() -> [CustomPetRecord] {
        guard let data = UserDefaults.standard.data(forKey: customRecordsKey),
              let records = try? JSONDecoder().decode([CustomPetRecord].self, from: data) else {
            return []
        }
        return records
    }

    private static func saveCustomRecords(_ records: [CustomPetRecord]) {
        guard let data = try? JSONEncoder().encode(records) else { return }
        UserDefaults.standard.set(data, forKey: customRecordsKey)
    }

    private static func stableID(for value: String) -> String {
        var hash: UInt64 = 1469598103934665603
        for byte in value.utf8 {
            hash ^= UInt64(byte)
            hash &*= 1099511628211
        }
        return String(hash, radix: 16)
    }

    private static let customRecordsKey = "Pet.CustomCharacters"
}

enum PetImportError: LocalizedError {
    case invalidResource(String)
    case unsupported(String)

    var errorDescription: String? {
        switch self {
        case .invalidResource(let message), .unsupported(let message):
            return message
        }
    }
}

private struct CustomPetRecord: Codable {
    let id: String
    let name: String
    let rendererKind: PetRendererKind
    let resourceBaseName: String
    let resourceDirectoryPath: String

    init(character: PetCharacter) {
        id = character.id
        name = character.name
        rendererKind = character.rendererKind
        resourceBaseName = character.resourceBaseName
        resourceDirectoryPath = character.resourceDirectoryPath ?? ""
    }

    func character(actions: [String]) -> PetCharacter {
        PetCharacter(
            id: id,
            name: name,
            rendererKind: rendererKind,
            resourceBaseName: resourceBaseName,
            resourceSubdirectory: "",
            resourceDirectoryPath: resourceDirectoryPath,
            previewColumn: 0,
            previewRow: 0,
            actions: actions
        )
    }
}

private extension URL {
    var existingFileURL: URL? {
        FileManager.default.fileExists(atPath: path) ? self : nil
    }
}

struct Live2DModel3Manifest: Decodable {
    let fileReferences: FileReferences

    enum CodingKeys: String, CodingKey {
        case fileReferences = "FileReferences"
    }

    struct FileReferences: Decodable {
        let moc: String
        let textures: [String]
        let expressions: [Expression]?
        let motions: [String: [Motion]]?

        enum CodingKeys: String, CodingKey {
            case moc = "Moc"
            case textures = "Textures"
            case expressions = "Expressions"
            case motions = "Motions"
        }
    }

    struct Expression: Decodable {
        let name: String
        let file: String

        enum CodingKeys: String, CodingKey {
            case name = "Name"
            case file = "File"
        }
    }

    struct Motion: Decodable {
        let file: String

        enum CodingKeys: String, CodingKey {
            case file = "File"
        }
    }
}
