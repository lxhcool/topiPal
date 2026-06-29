import AppKit
import SwiftUI

struct SettingsPanelView: View {
    @ObservedObject var model: PetModel
    @ObservedObject var settings: SettingsStore
    let onClose: () -> Void

    @State private var selectedSection: SettingsSection = .characters

    var body: some View {
        HStack(spacing: 0) {
            sidebar

            Divider()
                .overlay(Color.white.opacity(0.06))

            content
        }
        .frame(width: 810, height: 610)
        .background(
            RoundedRectangle(cornerRadius: 20, style: .continuous)
                .fill(Color(red: 0.105, green: 0.115, blue: 0.12))
        )
        .clipShape(RoundedRectangle(cornerRadius: 20, style: .continuous))
        .colorScheme(.dark)
        .overlay(alignment: .topTrailing) {
            Button(action: onClose) {
                Image(systemName: "xmark")
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundStyle(.secondary)
                    .frame(width: 26, height: 26)
                    .background(
                        Circle()
                            .fill(Color.white.opacity(0.08))
                            .overlay(
                                Circle()
                                    .stroke(Color.white.opacity(0.12), lineWidth: 1)
                            )
                    )
            }
            .buttonStyle(.plain)
            .pointingHandCursor()
            .padding(.top, 14)
            .padding(.trailing, 14)
        }
    }

    private var sidebar: some View {
        VStack(alignment: .leading, spacing: 6) {
            Spacer()
                .frame(height: 24)

            ForEach(SettingsSection.allCases) { section in
                Button {
                    selectedSection = section
                } label: {
                    HStack(spacing: 10) {
                        Image(systemName: section.icon)
                            .font(.system(size: 14, weight: .medium))
                            .frame(width: 20)
                        Text(section.title)
                            .font(.system(size: 13, weight: .medium))
                        Spacer()
                    }
                    .foregroundStyle(selectedSection == section ? .primary : .secondary)
                    .padding(.horizontal, 12)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .frame(height: 34)
                    .background(
                        RoundedRectangle(cornerRadius: 9, style: .continuous)
                            .fill(selectedSection == section ? Color.white.opacity(0.12) : Color.clear)
                    )
                    .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .pointingHandCursor()
            }

            Spacer()
        }
        .padding(.horizontal, 16)
        .frame(width: 190)
        .background(Color.white.opacity(0.04))
    }

    private var content: some View {
        VStack(spacing: 0) {
            HStack {
                Text(selectedSection.title)
                    .font(.system(size: 18, weight: .bold))
                    .foregroundStyle(.primary)

                Spacer()
            }
            .padding(.horizontal, 26)
            .padding(.top, 28)
            .padding(.bottom, 18)
            .overlay(WindowDragRegion())

            ScrollView(.vertical, showsIndicators: false) {
                VStack(spacing: 16) {
                    switch selectedSection {
                    case .characters:
                        characterSection
                    case .appearance:
                        appearanceSection
                    case .resources:
                        customResourceSection
                    case .smart:
                        SmartSettingsSection(model: model, settings: model.smartSettings)
                    }
                }
                .padding(.horizontal, 26)
                .padding(.bottom, 26)
            }
        }
    }

    private var characterSection: some View {
        SettingsCard(title: "内置角色", icon: "person.crop.square") {
            LazyVGrid(
                columns: [
                    GridItem(.flexible(), spacing: 12),
                    GridItem(.flexible(), spacing: 12)
                ],
                spacing: 12
            ) {
                ForEach(model.characters) { character in
                    CharacterSelectionCard(
                        character: character,
                        isSelected: character.id == model.selectedCharacterID,
                        onDelete: character.isCustom ? {
                            model.deleteCharacter(character)
                        } : nil
                    ) {
                        model.selectCharacter(character)
                    }
                }
            }
        }
    }

    private var appearanceSection: some View {
        SettingsCard(title: "显示", icon: "slider.horizontal.3") {
            VStack(spacing: 20) {
                HStack {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("角色大小")
                            .font(.system(size: 13, weight: .medium))
                            .foregroundStyle(.primary)
                        Text("\(Int(settings.petScale * 100))%")
                            .font(.system(size: 11))
                            .foregroundStyle(.secondary)
                    }

                    Spacer()

                    Button("重置") {
                        settings.petScale = 0.8
                    }
                    .buttonStyle(SettingsButtonStyle())
                }

                Slider(value: $settings.petScale, in: 0.72...1.28, step: 0.02)
                    .tint(.accentColor)
            }
        }
    }

    private var customResourceSection: some View {
        VStack(spacing: 16) {
            SettingsCard(title: "导入角色", icon: "square.and.arrow.down") {
                VStack(spacing: 12) {
                    ResourceImportRow(
                        icon: "photo.stack",
                        title: "序列帧角色",
                        detail: "选择一个角色文件夹，动作由子文件夹决定，例如 idle、tap、walk。每个动作目录放连续 PNG。"
                    ) {
                        pickResourceFolder(title: "选择序列帧角色文件夹", kind: .sequence)
                    }

                    Divider().overlay(Color.white.opacity(0.06))

                    ResourceImportRow(
                        icon: "figure.wave",
                        title: "Spine 角色",
                        detail: "文件夹内需要包含 .skel 或 .json、.atlas、以及 atlas 引用的 PNG。动作从 Spine 文件读取。"
                    ) {
                        pickResourceFolder(title: "选择 Spine 角色文件夹", kind: .spine)
                    }

                    Divider().overlay(Color.white.opacity(0.06))

                    ResourceImportRow(
                        icon: "person.crop.rectangle.stack",
                        title: "Live2D 角色",
                        detail: "文件夹内需要包含 model3.json、moc3、textures、motions、expressions。"
                    ) {
                        pickResourceFolder(title: "选择 Live2D 角色文件夹", kind: .live2D)
                    }
                }
            }

            if let importErrorMessage = model.importErrorMessage {
                SettingsCard(title: "导入结果", icon: "exclamationmark.triangle") {
                    Text(importErrorMessage)
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(.white.opacity(0.68))
                }
            }

            SettingsCard(title: "资源规范", icon: "doc.text") {
                VStack(alignment: .leading, spacing: 10) {
                    SpecLine(title: "序列帧", value: "角色目录 / 动作目录 / PNG 帧。动作名等于文件夹名。")
                    SpecLine(title: "Spine", value: "同一目录放 .skel/.json、.atlas、贴图 PNG。")
                    SpecLine(title: "Live2D", value: "以 model3.json 为入口读取模型、动作和表情。")
                    SpecLine(title: "命中区域", value: "后续会在这里配置头、身体、左右、脚分别触发哪个已有动作。")
                }
            }
        }
    }

    private func pickResourceFolder(title: String, kind: PetRendererKind) {
        let panel = NSOpenPanel()
        panel.title = title
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            model.importCharacter(kind: kind, from: url)
        }
    }
}

private enum SettingsSection: String, CaseIterable, Identifiable {
    case characters
    case appearance
    case resources
    case smart

    var id: String { rawValue }

    var title: String {
        switch self {
        case .characters: "角色"
        case .appearance: "外观"
        case .resources: "自定义资源"
        case .smart: "智能陪伴"
        }
    }

    var subtitle: String {
        switch self {
        case .characters: "选择当前桌面宠物。"
        case .appearance: "调整角色大小和显示效果。"
        case .resources: "导入序列帧、Spine 或 Live2D 角色资源。"
        case .smart: "配置 AI、状态感知和通知提醒。"
        }
    }

    var icon: String {
        switch self {
        case .characters: "person.crop.square"
        case .appearance: "slider.horizontal.3"
        case .resources: "folder.badge.plus"
        case .smart: "brain.head.profile"
        }
    }
}

private struct WindowDragRegion: NSViewRepresentable {
    func makeNSView(context: Context) -> NSView {
        DraggableRegionView()
    }

    func updateNSView(_ nsView: NSView, context: Context) {}
}

private final class DraggableRegionView: NSView {
    override func mouseDown(with event: NSEvent) {
        window?.performDrag(with: event)
    }
}

private struct SmartSettingsSection: View {
    @ObservedObject var model: PetModel
    @ObservedObject var settings: SmartCompanionSettings
    @State private var selectedPane: SmartSettingsPane = .model

    var body: some View {
        VStack(spacing: 16) {
            SmartSettingsSegmentedControl(selection: $selectedPane)

            switch selectedPane {
            case .model:
                modelSection
            case .messages:
                messageSection
            case .awareness:
                awarenessSection
            case .notifications:
                notificationSection
            }
        }
    }

    private var modelSection: some View {
            SettingsCard(title: "模型配置", icon: "brain.head.profile") {
                VStack(spacing: 20) {
                    VStack(alignment: .leading, spacing: 4) {
                        HStack {
                            Text("启用智能气泡")
                                .font(.system(size: 13, weight: .medium))
                                .foregroundStyle(.primary)
                            Spacer()
                            Toggle("", isOn: $settings.isEnabled)
                                .toggleStyle(SwitchToggleStyle(tint: .accentColor))
                                .labelsHidden()
                        }
                        Text("先即时显示本地反应，再用模型结合当前状态润色同一条气泡。")
                            .font(.system(size: 11))
                            .foregroundStyle(.secondary)
                    }

                    Divider().overlay(Color.white.opacity(0.06))

                    HStack(spacing: 12) {
                        Text("服务商")
                            .font(.system(size: 13, weight: .medium))
                            .foregroundStyle(.primary)
                            .frame(width: 76, alignment: .leading)

                        Picker("", selection: $settings.provider) {
                            ForEach(SmartProvider.allCases) { provider in
                                Text(provider.displayName).tag(provider)
                            }
                        }
                        .labelsHidden()
                        .onChange(of: settings.provider) {
                            settings.applyProviderDefaults()
                        }

                        Spacer()
                    }

                    SettingsTextField(title: "Base URL", text: $settings.baseURL, placeholder: "https://api.deepseek.com")
                    SettingsTextField(title: "模型", text: $settings.modelName, placeholder: "deepseek-v4-flash")
                    SettingsSecureField(title: "API Key", text: $settings.apiKey, placeholder: "sk-...")

                    HStack {
                        Spacer()
                        Button("测试气泡") {
                            model.requestSmartBubbleNow()
                        }
                        .buttonStyle(SettingsButtonStyle())
                    }

                    if let message = model.smartTestResultMessage {
                        DiagnosticTextBox(text: message, minHeight: 78)
                    }
                }
            }
    }

    private var messageSection: some View {
            SettingsCard(title: "话题", icon: "text.bubble") {
                VStack(spacing: 20) {
                    LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 10) {
                        ForEach(SmartMessageTopic.allCases) { topic in
                            SmartTopicToggle(
                                topic: topic,
                                isOn: Binding(
                                    get: { settings.enabledTopics.contains(topic) },
                                    set: { isOn in
                                        if isOn {
                                            settings.enabledTopics.insert(topic)
                                        } else {
                                            settings.enabledTopics.remove(topic)
                                        }
                                    }
                                )
                            )
                        }
                    }

                    Divider().overlay(Color.white.opacity(0.06))

                    VStack(alignment: .leading, spacing: 8) {
                        Text("打扰强度")
                            .font(.system(size: 13, weight: .medium))
                            .foregroundStyle(.primary)

                        Picker("", selection: $settings.talkativeness) {
                            ForEach(CompanionTalkativeness.allCases) { level in
                                Text(level.title).tag(level)
                            }
                        }
                        .pickerStyle(.segmented)
                        .labelsHidden()

                        Text(settings.talkativeness.description)
                            .font(.system(size: 11))
                            .foregroundStyle(.secondary)
                    }

                    Divider().overlay(Color.white.opacity(0.06))

                    SettingsTextField(title: "天气城市", text: $settings.weatherCity, placeholder: "例如：上海")
                }
            }
    }

    private var awarenessSection: some View {
            SettingsCard(title: "活动感知", icon: "eye") {
                VStack(spacing: 20) {
                    VStack(alignment: .leading, spacing: 4) {
                        HStack {
                            Text("根据当前工作状态调整宠物")
                                .font(.system(size: 13, weight: .medium))
                                .foregroundStyle(.primary)
                            Spacer()
                            Toggle("", isOn: $settings.isActivityAwarenessEnabled)
                                .toggleStyle(SwitchToggleStyle(tint: .accentColor))
                                .labelsHidden()
                        }
                        Text("识别写代码、聊天、会议、看视频等状态，影响气泡频率和语气。")
                            .font(.system(size: 11))
                            .foregroundStyle(.secondary)
                    }

                    VStack(alignment: .leading, spacing: 4) {
                        HStack {
                            Text("读取窗口标题")
                                .font(.system(size: 13, weight: .medium))
                                .foregroundStyle(.primary)
                            Spacer()
                            Toggle("", isOn: $settings.canReadWindowTitles)
                                .toggleStyle(SwitchToggleStyle(tint: .accentColor))
                                .labelsHidden()
                        }
                        Text("可更准确识别代码文件、网页和会议；可能包含文件名或网页标题。")
                            .font(.system(size: 11))
                            .foregroundStyle(.secondary)
                    }

                    Divider().overlay(Color.white.opacity(0.06))

                    SettingsInfoRow(
                        icon: "dot.radiowaves.left.and.right",
                        title: "当前状态",
                        value: activityStatusText
                    )
                }
            }
    }

    private var notificationSection: some View {
            SettingsCard(title: "消息监听", icon: "app.badge") {
                VStack(spacing: 20) {
                    VStack(alignment: .leading, spacing: 4) {
                        HStack {
                            Text("监听系统通知")
                                .font(.system(size: 13, weight: .medium))
                                .foregroundStyle(.primary)
                            Spacer()
                            Toggle("", isOn: $settings.isNotificationListeningEnabled)
                                .toggleStyle(SwitchToggleStyle(tint: .accentColor))
                                .labelsHidden()
                        }
                        Text("识别通知来源 App，弹出\u{201c}微信来了消息\u{201d}这类气泡。")
                            .font(.system(size: 11))
                            .foregroundStyle(.secondary)
                    }

                    Divider().overlay(Color.white.opacity(0.06))

                    SettingsTextField(title: "关注 App", text: $settings.notificationAppNames, placeholder: "微信,WeChat,飞书,Lark")

                    HStack {
                        SettingsInfoRow(
                            icon: "lock.shield",
                            title: "权限",
                            value: "消息提醒主链路只需要完全磁盘访问，用于读取系统通知数据库；辅助功能只是 Dock/状态栏兜底，可不开。"
                        )

                        Button("请求权限") {
                            AppNotificationMonitor(model: model, settings: settings).requestAccessibilityPermission()
                            model.showSystemMessage("辅助功能仅用于兜底监听 Dock/状态栏，可不开")
                        }
                        .buttonStyle(SettingsButtonStyle())

                        Button("完全磁盘访问") {
                            AppNotificationMonitor(model: model, settings: settings).requestFullDiskAccessPermission()
                            model.showSystemMessage("请在完全磁盘访问里允许 TopiPal")
                        }
                        .buttonStyle(SettingsButtonStyle())
                    }

                    HStack {
                        Spacer()
                        Button("诊断监听") {
                            model.runNotificationDiagnostics()
                        }
                        .buttonStyle(SettingsButtonStyle())

                        Button("测试消息提醒") {
                            model.receiveAppNotification(appName: "微信")
                        }
                        .buttonStyle(SettingsButtonStyle())
                    }

                    Divider().overlay(Color.white.opacity(0.06))

                    SettingsInfoRow(
                        icon: "bell.badge",
                        title: "微信 / 飞书 / 其他 App",
                        value: "优先读取系统通知数据库识别来源；读不到数据库时，才使用辅助功能和窗口元数据兜底。"
                    )

                    if let event = model.lastNotificationMonitorEvent {
                        Divider().overlay(Color.white.opacity(0.06))
                        SettingsInfoRow(
                            icon: "waveform.path.ecg",
                            title: "最近命中",
                            value: event
                        )
                    }

                    if let report = model.notificationDiagnosticReport {
                        Divider().overlay(Color.white.opacity(0.06))
                        DiagnosticTextBox(text: report, minHeight: 180)
                    }
                }
            }
    }

    private var activityStatusText: String {
        let context = model.activityContext
        let mode = context?.mode.title ?? "未识别"
        let app = context?.appName ?? "无前台 App"
        let confidence = context.map { "\($0.confidence)%" } ?? "-"
        let title = context?.windowTitle?.trimmingCharacters(in: .whitespacesAndNewlines)
        if let title, !title.isEmpty {
            return "\(mode) · \(app) · \(confidence)\n\(title)"
        }
        return "\(mode) · \(app) · \(confidence)"
    }
}

private enum SmartSettingsPane: String, CaseIterable, Identifiable {
    case model
    case messages
    case awareness
    case notifications

    var id: String { rawValue }

    var title: String {
        switch self {
        case .model: "模型"
        case .messages: "文案"
        case .awareness: "感知"
        case .notifications: "通知"
        }
    }

    var icon: String {
        switch self {
        case .model: "brain.head.profile"
        case .messages: "text.bubble"
        case .awareness: "eye"
        case .notifications: "app.badge"
        }
    }
}

private struct SmartSettingsSegmentedControl: View {
    @Binding var selection: SmartSettingsPane

    var body: some View {
        HStack(spacing: 8) {
            ForEach(SmartSettingsPane.allCases) { pane in
                Button {
                    withAnimation(.spring(response: 0.22, dampingFraction: 0.82)) {
                        selection = pane
                    }
                } label: {
                    Label(pane.title, systemImage: pane.icon)
                        .font(.system(size: 12, weight: .semibold))
                        .foregroundStyle(selection == pane ? .primary : .secondary)
                        .frame(maxWidth: .infinity)
                        .frame(height: 32)
                        .background(
                            RoundedRectangle(cornerRadius: 10, style: .continuous)
                                .fill(selection == pane ? Color.white.opacity(0.12) : Color.white.opacity(0.04))
                        )
                }
                .buttonStyle(.plain)
                .pointingHandCursor()
            }
        }
        .padding(4)
        .background(Color.white.opacity(0.035), in: RoundedRectangle(cornerRadius: 13, style: .continuous))
    }
}

private struct DiagnosticTextBox: View {
    let text: String
    let minHeight: CGFloat

    var body: some View {
        ScrollView(.vertical, showsIndicators: true) {
            Text(text)
                .font(.system(size: 10, weight: .medium, design: .monospaced))
                .foregroundStyle(.secondary)
                .textSelection(.enabled)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(10)
        }
        .frame(minHeight: minHeight, maxHeight: minHeight)
        .background(.white.opacity(0.04), in: RoundedRectangle(cornerRadius: 8, style: .continuous))
        .overlay(
            RoundedRectangle(cornerRadius: 8, style: .continuous)
                .stroke(.white.opacity(0.08), lineWidth: 1)
        )
    }
}

private struct SmartTopicToggle: View {
    let topic: SmartMessageTopic
    @Binding var isOn: Bool

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: topic.icon)
                .font(.system(size: 14, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 20)

            Text(topic.title)
                .font(.system(size: 13, weight: .medium))
                .foregroundStyle(.primary)

            Spacer()

            Toggle("", isOn: $isOn)
                .toggleStyle(SwitchToggleStyle(tint: .accentColor))
                .labelsHidden()
        }
        .padding(.horizontal, 4)
    }
}

private struct CharacterSelectionCard: View {
    let character: PetCharacter
    let isSelected: Bool
    let onDelete: (() -> Void)?
    let action: () -> Void

    @State private var isHovered = false
    var body: some View {
        ZStack(alignment: .topTrailing) {
            Button(action: action) {
                VStack(alignment: .leading, spacing: 0) {
                    CharacterPreview(character: character)

                    VStack(alignment: .leading, spacing: 10) {
                        HStack {
                            Text(character.name)
                                .font(.system(size: 14, weight: .semibold))
                                .foregroundStyle(.primary)
                                .lineLimit(1)

                            Spacer()

                            Image(systemName: isSelected ? "checkmark.circle.fill" : "circle")
                                .font(.system(size: 16, weight: .semibold))
                                .foregroundStyle(isSelected ? .primary : .secondary)
                                .padding(.trailing, onDelete == nil ? 0 : 26)
                        }

                        HStack(spacing: 8) {
                            ResourceTag(text: character.rendererKind.displayName)
                            ResourceTag(text: "\(character.actions.count) 动作")
                            if character.isCustom {
                                ResourceTag(text: "自定义")
                            }
                        }

                    }
                    .padding(12)
                }
                .frame(maxWidth: .infinity, minHeight: 268, alignment: .topLeading)
                .background(
                    RoundedRectangle(cornerRadius: 14, style: .continuous)
                        .fill(Color.white.opacity(isSelected ? 0.08 : (isHovered ? 0.06 : 0.04)))
                        .overlay(
                            RoundedRectangle(cornerRadius: 14, style: .continuous)
                                .stroke(Color.white.opacity(isSelected ? 0.18 : 0.08), lineWidth: 1)
                        )
                )
                .shadow(color: .black.opacity(isHovered ? 0.18 : 0.06), radius: isHovered ? 14 : 4, y: isHovered ? 7 : 2)
                .scaleEffect(isHovered ? 1.015 : 1)
            }
            .buttonStyle(.plain)
            .pointingHandCursor()

            if let onDelete {
                Button(action: onDelete) {
                    Image(systemName: "trash")
                        .font(.system(size: 12, weight: .semibold))
                        .foregroundStyle(.secondary)
                        .frame(width: 26, height: 26)
                        .background(.white.opacity(0.08), in: RoundedRectangle(cornerRadius: 8, style: .continuous))
                        .overlay(
                            RoundedRectangle(cornerRadius: 8, style: .continuous)
                                .stroke(.white.opacity(0.10), lineWidth: 1)
                        )
                }
                .buttonStyle(.plain)
                .pointingHandCursor()
                .help("删除导入记录")
                .padding(9)
            }
        }
        .onHover { hovering in
            withAnimation(.spring(response: 0.24, dampingFraction: 0.78)) {
                isHovered = hovering
            }
        }
    }
}

private struct CharacterPreview: View {
    let character: PetCharacter

    var body: some View {
        ZStack {
            LinearGradient(
                colors: [
                    Color.white.opacity(0.055),
                    Color.white.opacity(0.02)
                ],
                startPoint: .top,
                endPoint: .bottom
            )

            PetRendererView(character: character, action: character.defaultAction)
                .frame(width: 154, height: 142)
                .allowsHitTesting(false)
                .padding(.top, 8)
        }
        .frame(height: 184)
        .clipped()
    }
}

private struct ResourceImportRow: View {
    let icon: String
    let title: String
    let detail: String
    let action: () -> Void

    var body: some View {
        HStack(alignment: .center, spacing: 12) {
            Image(systemName: icon)
                .font(.system(size: 14, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 20)

            VStack(alignment: .leading, spacing: 4) {
                Text(title)
                    .font(.system(size: 13, weight: .medium))
                    .foregroundStyle(.primary)
                Text(detail)
                    .font(.system(size: 11))
                    .foregroundStyle(.secondary)
                    .lineLimit(2)
            }

            Spacer()

            Button("选择") {
                action()
            }
            .buttonStyle(SettingsButtonStyle())
        }
    }
}

private struct SettingsTextField: View {
    let title: String
    @Binding var text: String
    let placeholder: String

    var body: some View {
        HStack(spacing: 12) {
            Text(title)
                .font(.system(size: 13, weight: .medium))
                .foregroundStyle(.primary)
                .frame(width: 76, alignment: .leading)

            TextField(placeholder, text: $text)
                .textFieldStyle(.plain)
                .font(.system(size: 12, weight: .medium))
                .foregroundStyle(.primary)
                .padding(.horizontal, 10)
                .frame(height: 34)
                .background(.white.opacity(0.07), in: RoundedRectangle(cornerRadius: 8, style: .continuous))
                .overlay(
                    RoundedRectangle(cornerRadius: 8, style: .continuous)
                        .stroke(.white.opacity(0.10), lineWidth: 1)
                )
        }
    }
}

private struct SettingsSecureField: View {
    let title: String
    @Binding var text: String
    let placeholder: String

    var body: some View {
        HStack(spacing: 12) {
            Text(title)
                .font(.system(size: 13, weight: .medium))
                .foregroundStyle(.primary)
                .frame(width: 76, alignment: .leading)

            PasteFriendlySecureField(text: $text, placeholder: placeholder)
                .padding(.horizontal, 10)
                .frame(height: 34)
                .background(.white.opacity(0.07), in: RoundedRectangle(cornerRadius: 8, style: .continuous))
                .overlay(
                    RoundedRectangle(cornerRadius: 8, style: .continuous)
                        .stroke(.white.opacity(0.10), lineWidth: 1)
                )
        }
    }
}

private struct PasteFriendlySecureField: NSViewRepresentable {
    @Binding var text: String
    let placeholder: String

    func makeNSView(context: Context) -> NSSecureTextField {
        let field = PasteEnabledSecureTextField()
        field.placeholderString = placeholder
        field.stringValue = text
        field.isBordered = false
        field.isBezeled = false
        field.drawsBackground = false
        field.focusRingType = .none
        field.font = .systemFont(ofSize: 12, weight: .medium)
        field.textColor = .white
        field.delegate = context.coordinator
        field.lineBreakMode = .byTruncatingMiddle
        return field
    }

    func updateNSView(_ field: NSSecureTextField, context: Context) {
        if field.stringValue != text {
            field.stringValue = text
        }
        field.placeholderString = placeholder
    }

    func makeCoordinator() -> Coordinator {
        Coordinator(text: $text)
    }

    final class Coordinator: NSObject, NSTextFieldDelegate {
        @Binding var text: String

        init(text: Binding<String>) {
            _text = text
        }

        func controlTextDidChange(_ notification: Notification) {
            guard let field = notification.object as? NSSecureTextField else { return }
            text = field.stringValue
        }
    }
}

private final class PasteEnabledSecureTextField: NSSecureTextField {
    override func performKeyEquivalent(with event: NSEvent) -> Bool {
        guard event.modifierFlags.contains(.command),
              let characters = event.charactersIgnoringModifiers?.lowercased() else {
            return super.performKeyEquivalent(with: event)
        }

        switch characters {
        case "v":
            currentEditor()?.paste(nil)
            return true
        case "a":
            selectText(nil)
            return true
        case "c":
            currentEditor()?.copy(nil)
            return true
        case "x":
            currentEditor()?.cut(nil)
            return true
        default:
            return super.performKeyEquivalent(with: event)
        }
    }
}

private struct SettingsInfoRow: View {
    let icon: String
    let title: String
    let value: String

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            Image(systemName: icon)
                .font(.system(size: 14, weight: .medium))
                .foregroundStyle(.secondary)
                .frame(width: 20)

            VStack(alignment: .leading, spacing: 4) {
                Text(title)
                    .font(.system(size: 13, weight: .medium))
                    .foregroundStyle(.primary)
                Text(value)
                    .font(.system(size: 11))
                    .foregroundStyle(.secondary)
            }

            Spacer()
        }
    }
}

private struct SpecLine: View {
    let title: String
    let value: String

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            Text(title)
                .font(.system(size: 12, weight: .medium))
                .foregroundStyle(.primary)
                .frame(width: 58, alignment: .leading)
            Text(value)
                .font(.system(size: 12))
                .foregroundStyle(.secondary)
            Spacer()
        }
    }
}

private struct ResourceTag: View {
    let text: String

    var body: some View {
        Text(text)
            .font(.system(size: 10, weight: .medium))
            .foregroundStyle(.secondary)
            .padding(.horizontal, 7)
            .padding(.vertical, 3)
            .background(.white.opacity(0.08), in: Capsule())
    }
}

private struct SettingsCard<Content: View>: View {
    let title: String
    let icon: String
    @ViewBuilder let content: Content

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Label(title, systemImage: icon)
                .font(.system(size: 13, weight: .semibold))
                .foregroundStyle(.secondary)
                .padding(.horizontal, 4)

            VStack(alignment: .leading, spacing: 0) {
                content
            }
            .padding(16)
            .background(
                RoundedRectangle(cornerRadius: 16, style: .continuous)
                    .fill(Color.white.opacity(0.04))
                    .overlay(
                        RoundedRectangle(cornerRadius: 16, style: .continuous)
                            .stroke(Color.white.opacity(0.08), lineWidth: 1)
                    )
            )
        }
    }
}

private struct SettingsButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: 12, weight: .medium))
            .foregroundStyle(.primary)
            .padding(.horizontal, 12)
            .padding(.vertical, 6)
            .background(
                RoundedRectangle(cornerRadius: 8, style: .continuous)
                    .fill(Color.white.opacity(configuration.isPressed ? 0.12 : 0.08))
            )
            .overlay(
                RoundedRectangle(cornerRadius: 8, style: .continuous)
                    .stroke(Color.white.opacity(0.1), lineWidth: 1)
            )
            .scaleEffect(configuration.isPressed ? 0.96 : 1)
            .animation(.easeOut(duration: 0.1), value: configuration.isPressed)
            .pointingHandCursor()
    }
}

private extension View {
    func pointingHandCursor() -> some View {
        overlay(CursorTrackingView(cursor: .pointingHand))
    }
}

private struct CursorTrackingView: NSViewRepresentable {
    let cursor: NSCursor

    func makeNSView(context: Context) -> NSView {
        CursorTrackerView(cursor: cursor)
    }

    func updateNSView(_ nsView: NSView, context: Context) {}
}

private final class CursorTrackerView: NSView {
    let cursor: NSCursor
    private var trackingArea: NSTrackingArea?
    private var isInside = false

    init(cursor: NSCursor) {
        self.cursor = cursor
        super.init(frame: .zero)
        wantsLayer = true
        layer?.backgroundColor = NSColor.clear.cgColor
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func updateTrackingAreas() {
        super.updateTrackingAreas()
        if let trackingArea { removeTrackingArea(trackingArea) }
        let area = NSTrackingArea(rect: bounds, options: [.mouseEnteredAndExited, .activeAlways, .inVisibleRect], owner: self)
        addTrackingArea(area)
        trackingArea = area
    }

    override func mouseEntered(with event: NSEvent) {
        isInside = true
        cursor.push()
    }

    override func mouseExited(with event: NSEvent) {
        if isInside {
            cursor.pop()
            isInside = false
        }
    }

    override func hitTest(_ point: NSPoint) -> NSView? { nil }
}
