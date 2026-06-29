import AppKit
import SwiftUI

struct PetRootView: View {
    @ObservedObject var model: PetModel
    @ObservedObject var settings: SettingsStore
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    private var isExpanded: Bool {
        model.phase == .expanded
    }

    var body: some View {
        VStack(spacing: 0) {
            PetStage(model: model, settings: settings, isExpanded: isExpanded, message: model.bubbleMessage)

            NotificationBubbleSlot(notification: model.notificationBubble)

            if isExpanded {
                CharacterSwitcher(model: model)
                    .transition(.opacity.combined(with: .move(edge: .bottom)))
                    .padding(.top, 8)
            }
        }
        .padding(.horizontal, isExpanded ? 14 : 8)
        .padding(.vertical, isExpanded ? 10 : 4)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.clear)
        .contentShape(Rectangle())
        .animation(.spring(response: 0.34, dampingFraction: 0.86), value: model.phase)
    }
}

private struct NotificationBubbleSlot: View {
    let notification: PetNotificationBubble?

    var body: some View {
        ZStack(alignment: .top) {
            Color.clear
            if let notification {
                NotificationBubble(notification: notification)
                    .id("\(notification.appName)-\(notification.message ?? "")")
                    .padding(.top, 4)
                    .transition(.bubblePop)
            }
        }
        .frame(minHeight: 68, maxHeight: 96)
        .animation(.spring(response: 0.22, dampingFraction: 0.68), value: notification)
    }
}

private struct PetStage: View {
    @ObservedObject var model: PetModel
    @ObservedObject var settings: SettingsStore
    let isExpanded: Bool
    let message: String?

    private var bubbleTopOffset: CGFloat {
        let scaleDelta = CGFloat(settings.petScale) - 1
        return min(6, max(-46, -22 - scaleDelta * 70))
    }

    var body: some View {
        GeometryReader { proxy in
            let scale = CGFloat(settings.petScale)
            let petWidth = proxy.size.width * scale
            let petHeight = max(120, proxy.size.height - 22) * scale

            ZStack(alignment: .top) {
                PetRendererView(character: model.selectedCharacter, action: model.activeAction)
                    .frame(width: petWidth, height: petHeight)
                    .scaleEffect(model.activeAction != nil ? 1.03 : 1.0)
                    .overlay {
                        PointerTrackingView { point, trackingSize in
                            model.handlePointerMove(at: point, in: trackingSize)
                        } onExit: {
                            model.handlePointerExit()
                        }
                    }
                    .gesture(
                        DragGesture(minimumDistance: 0)
                            .onEnded { value in
                                let distance = hypot(value.translation.width, value.translation.height)
                                if distance < 5 {
                                    model.handleTap(in: hitArea(at: value.startLocation, size: proxy.size))
                                }
                            }
                    )
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .animation(.spring(response: 0.22, dampingFraction: 0.62), value: model.activeAction)

                if let message {
                    PetBubble(message: message)
                        .id(message)
                        .frame(maxWidth: min(proxy.size.width * 0.72, 232), alignment: .center)
                        .padding(.top, bubbleTopOffset)
                        .transition(.bubblePop)
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .animation(.spring(response: 0.2, dampingFraction: 0.66), value: message)
        }
        .frame(height: isExpanded ? 292 : 272)
    }

    private func hitArea(at point: CGPoint, size: CGSize) -> PetHitArea {
        let x = point.x / max(size.width, 1)
        let y = point.y / max(size.height, 1)

        if y < 0.36 { return .head }
        if y > 0.78 { return .feet }
        if x < 0.34 { return .left }
        if x > 0.66 { return .right }
        return .body
    }
}

private struct PointerTrackingView: NSViewRepresentable {
    let onMove: (CGPoint, CGSize) -> Void
    let onExit: () -> Void

    func makeNSView(context: Context) -> TrackingNSView {
        let view = TrackingNSView()
        view.onMove = onMove
        view.onExit = onExit
        return view
    }

    func updateNSView(_ view: TrackingNSView, context: Context) {
        view.onMove = onMove
        view.onExit = onExit
    }
}

private final class TrackingNSView: NSView {
    var onMove: ((CGPoint, CGSize) -> Void)?
    var onExit: (() -> Void)?
    private var trackingArea: NSTrackingArea?

    override func hitTest(_ point: NSPoint) -> NSView? {
        nil
    }

    override func updateTrackingAreas() {
        super.updateTrackingAreas()
        if let trackingArea {
            removeTrackingArea(trackingArea)
        }
        let nextArea = NSTrackingArea(
            rect: bounds,
            options: [.activeAlways, .mouseMoved, .mouseEnteredAndExited, .inVisibleRect],
            owner: self
        )
        addTrackingArea(nextArea)
        trackingArea = nextArea
    }

    override func mouseMoved(with event: NSEvent) {
        onMove?(convert(event.locationInWindow, from: nil), bounds.size)
    }

    override func mouseDragged(with event: NSEvent) {
        onMove?(convert(event.locationInWindow, from: nil), bounds.size)
    }

    override func mouseExited(with event: NSEvent) {
        onExit?()
    }
}

struct PetRendererView: View {
    let character: PetCharacter
    let action: String?

    var body: some View {
        switch character.rendererKind {
        case .spine:
            SpineMetalView(character: character, action: action)
        case .spine40:
            Spine40MetalView(character: character, action: action)
        case .live2D:
            Live2DMetalView(character: character, action: action)
        case .sequence:
            SequenceFrameView(character: character, action: action)
        }
    }
}

private struct NotificationBubble: View {
    let notification: PetNotificationBubble

    private var appStyle: NotificationAppStyle {
        NotificationAppStyle(appName: notification.appName)
    }

    private var messageText: String {
        notification.message?.isEmpty == false ? notification.message! : "来消息了"
    }

    var body: some View {
        NotificationSurface {
            HStack(alignment: .top, spacing: 8) {
                NotificationAppIcon(notification: notification, fallbackStyle: appStyle)

                VStack(alignment: .leading, spacing: 4) {
                    Text(notification.appName)
                        .font(.system(size: 9, weight: .semibold))
                        .foregroundStyle(.white.opacity(0.58))
                        .lineLimit(1)

                    Text(messageText)
                        .font(.system(size: 11, weight: .semibold))
                        .foregroundStyle(.white.opacity(0.78))
                        .lineLimit(3)
                        .multilineTextAlignment(.leading)
                        .fixedSize(horizontal: false, vertical: true)
                }
                .frame(maxWidth: 210, alignment: .leading)
            }
        }
        .fixedSize(horizontal: false, vertical: true)
    }
}

private struct NotificationAppIcon: View {
    let notification: PetNotificationBubble
    let fallbackStyle: NotificationAppStyle

    var body: some View {
        if let image = AppIconResolver.icon(bundleID: notification.bundleID, appName: notification.appName) {
            Image(nsImage: image)
                .resizable()
                .scaledToFit()
                .frame(width: 21, height: 21)
                .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
        } else {
            ZStack {
                RoundedRectangle(cornerRadius: 7, style: .continuous)
                    .fill(fallbackStyle.color.opacity(0.72))
                Image(systemName: fallbackStyle.icon)
                    .font(.system(size: 10, weight: .semibold))
                    .foregroundStyle(.white.opacity(0.86))
            }
            .frame(width: 21, height: 21)
        }
    }
}

private enum AppIconResolver {
    static func icon(bundleID: String?, appName: String) -> NSImage? {
        if let bundleID,
           !bundleID.isEmpty,
           let url = NSWorkspace.shared.urlForApplication(withBundleIdentifier: bundleID) {
            return icon(at: url)
        }

        if let runningApp = NSWorkspace.shared.runningApplications.first(where: { app in
            guard let localizedName = app.localizedName else { return false }
            return localizedName.localizedCaseInsensitiveContains(appName)
                || appName.localizedCaseInsensitiveContains(localizedName)
        }), let bundleURL = runningApp.bundleURL {
            return icon(at: bundleURL)
        }

        if let appURL = NSWorkspace.shared.urlForApplication(withBundleIdentifier: fallbackBundleID(for: appName)) {
            return icon(at: appURL)
        }

        return nil
    }

    private static func icon(at url: URL) -> NSImage {
        let image = NSWorkspace.shared.icon(forFile: url.path)
        image.size = NSSize(width: 24, height: 24)
        return image
    }

    private static func fallbackBundleID(for appName: String) -> String {
        if appName.localizedCaseInsensitiveContains("微信")
            || appName.localizedCaseInsensitiveContains("WeChat") {
            return "com.tencent.xinWeChat"
        }
        if appName.localizedCaseInsensitiveContains("飞书")
            || appName.localizedCaseInsensitiveContains("Lark") {
            return "com.larksuite.Lark"
        }
        if appName.localizedCaseInsensitiveContains("邮件")
            || appName.localizedCaseInsensitiveContains("Mail") {
            return "com.apple.mail"
        }
        if appName.localizedCaseInsensitiveContains("信息")
            || appName.localizedCaseInsensitiveContains("Message") {
            return "com.apple.MobileSMS"
        }
        return ""
    }
}

private struct NotificationAppStyle {
    let icon: String
    let color: Color

    init(appName: String) {
        if appName.localizedCaseInsensitiveContains("微信")
            || appName.localizedCaseInsensitiveContains("WeChat") {
            icon = "message.fill"
            color = Color(red: 0.18, green: 0.74, blue: 0.33)
        } else if appName.localizedCaseInsensitiveContains("飞书")
                    || appName.localizedCaseInsensitiveContains("Lark") {
            icon = "paperplane.fill"
            color = Color(red: 0.16, green: 0.48, blue: 0.95)
        } else if appName.localizedCaseInsensitiveContains("邮件")
                    || appName.localizedCaseInsensitiveContains("Mail") {
            icon = "envelope.fill"
            color = Color(red: 0.96, green: 0.48, blue: 0.22)
        } else if appName.localizedCaseInsensitiveContains("信息")
                    || appName.localizedCaseInsensitiveContains("Message") {
            icon = "bubble.left.and.bubble.right.fill"
            color = Color(red: 0.18, green: 0.68, blue: 0.84)
        } else {
            icon = "bell.fill"
            color = Color(red: 0.76, green: 0.58, blue: 0.22)
        }
    }
}

private struct PetBubble: View {
    let message: String

    var body: some View {
        SpeechBubbleSurface {
            Text(message)
                .font(.system(size: 12, weight: .bold))
                .lineLimit(3)
                .multilineTextAlignment(.center)
                .foregroundStyle(.white.opacity(0.94))
                .fixedSize(horizontal: false, vertical: true)
        }
    }
}

private struct SpeechBubbleSurface<Content: View>: View {
    @ViewBuilder let content: Content

    var body: some View {
        content
            .padding(.horizontal, 14)
            .padding(.vertical, 9)
            .background(
                ZStack {
                    RoundedRectangle(cornerRadius: 16, style: .continuous)
                        .fill(Color(red: 0.055, green: 0.058, blue: 0.07).opacity(0.68))
                    RoundedRectangle(cornerRadius: 16, style: .continuous)
                        .stroke(.white.opacity(0.16), lineWidth: 1)
                    RoundedRectangle(cornerRadius: 13, style: .continuous)
                        .stroke(.white.opacity(0.07), lineWidth: 3)
                        .padding(2)
                }
            )
        .fixedSize(horizontal: false, vertical: true)
        .shadow(color: .black.opacity(0.18), radius: 8, y: 3)
    }
}

private struct NotificationSurface<Content: View>: View {
    @ViewBuilder let content: Content

    var body: some View {
        content
            .padding(.horizontal, 10)
            .padding(.vertical, 7)
            .background(
                ZStack {
                    RoundedRectangle(cornerRadius: 13, style: .continuous)
                        .fill(Color(red: 0.055, green: 0.058, blue: 0.07).opacity(0.48))
                    RoundedRectangle(cornerRadius: 13, style: .continuous)
                        .stroke(.white.opacity(0.11), lineWidth: 1)
                }
            )
            .shadow(color: .black.opacity(0.12), radius: 7, y: 3)
    }
}

private extension AnyTransition {
    static var bubblePop: AnyTransition {
        .asymmetric(
            insertion: .scale(scale: 0.72, anchor: .bottom).combined(with: .opacity),
            removal: .opacity.combined(with: .scale(scale: 0.96, anchor: .bottom))
        )
    }
}

private struct CharacterSwitcher: View {
    @ObservedObject var model: PetModel

    var body: some View {
        HStack(spacing: 8) {
            ForEach(model.characters) { character in
                Button {
                    model.selectCharacter(character)
                } label: {
                    Text(character.name.replacingOccurrences(of: "Seele ", with: ""))
                        .font(.system(size: 12, weight: .bold))
                        .foregroundStyle(.white)
                        .frame(width: 34, height: 34)
                        .background(.white.opacity(character.id == model.selectedCharacterID ? 0.22 : 0.08), in: Circle())
                        .overlay(
                            Circle()
                                .stroke(character.id == model.selectedCharacterID ? .white.opacity(0.86) : .white.opacity(0.18), lineWidth: 1.2)
                        )
                }
                .buttonStyle(.plain)
                .help(character.name)
            }
        }
        .padding(.horizontal, 10)
        .padding(.vertical, 7)
        .background(.white.opacity(0.08), in: Capsule())
    }
}
