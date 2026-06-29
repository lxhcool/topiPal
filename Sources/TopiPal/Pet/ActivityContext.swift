import AppKit
import ApplicationServices
import CoreGraphics
import Foundation

enum UserActivityMode: String, CaseIterable, Identifiable, Sendable {
    case coding
    case chatting
    case meeting
    case watchingVideo
    case browsing
    case reading
    case designing
    case terminal
    case idle
    case unknown

    var id: String { rawValue }

    var title: String {
        switch self {
        case .coding: "写代码"
        case .chatting: "聊天"
        case .meeting: "会议"
        case .watchingVideo: "看视频"
        case .browsing: "浏览"
        case .reading: "阅读"
        case .designing: "设计"
        case .terminal: "终端"
        case .idle: "离开"
        case .unknown: "未知"
        }
    }
}

struct ActivitySnapshot: Sendable {
    let timestamp: Date
    let frontAppName: String?
    let bundleID: String?
    let windowTitle: String?
    let isFullScreen: Bool
    let idleSeconds: TimeInterval
}

struct ActivityContext: Sendable, Equatable {
    let mode: UserActivityMode
    let appName: String?
    let bundleID: String?
    let windowTitle: String?
    let idleSeconds: TimeInterval
    let confidence: Int
}

enum ActivityContextMonitor {
    static func snapshot(readWindowTitle: Bool) -> ActivitySnapshot {
        let app = NSWorkspace.shared.frontmostApplication
        let bundleID = app?.bundleIdentifier
        let appName = app?.localizedName
        let title = readWindowTitle ? frontWindowTitle(processID: app?.processIdentifier) : nil

        return ActivitySnapshot(
            timestamp: Date(),
            frontAppName: appName,
            bundleID: bundleID,
            windowTitle: title,
            isFullScreen: isFrontWindowFullScreen(processID: app?.processIdentifier),
            idleSeconds: systemIdleSeconds()
        )
    }

    private static func frontWindowTitle(processID: pid_t?) -> String? {
        guard let processID, processID > 0, AXIsProcessTrusted() else { return nil }
        let appElement = AXUIElementCreateApplication(processID)

        var focusedWindow: CFTypeRef?
        if AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute as CFString, &focusedWindow) == .success,
           let window = focusedWindow {
            return axString(window as! AXUIElement, attribute: kAXTitleAttribute)
        }

        var windows: CFTypeRef?
        if AXUIElementCopyAttributeValue(appElement, kAXWindowsAttribute as CFString, &windows) == .success,
           let windowElements = windows as? [AXUIElement] {
            return windowElements.compactMap { axString($0, attribute: kAXTitleAttribute) }.first
        }
        return nil
    }

    private static func axString(_ element: AXUIElement, attribute: String) -> String? {
        var value: CFTypeRef?
        guard AXUIElementCopyAttributeValue(element, attribute as CFString, &value) == .success,
              let string = value as? String,
              !string.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
            return nil
        }
        return string
    }

    private static func isFrontWindowFullScreen(processID: pid_t?) -> Bool {
        guard let processID,
              let windows = CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID) as? [[String: Any]] else {
            return false
        }

        let screenFrame = NSScreen.main?.frame ?? .zero
        return windows.contains { window in
            let ownerPID = window[kCGWindowOwnerPID as String] as? pid_t
            guard ownerPID == processID,
                  let bounds = window[kCGWindowBounds as String] as? [String: Any] else { return false }
            let width = bounds["Width"] as? Double ?? 0
            let height = bounds["Height"] as? Double ?? 0
            return width >= screenFrame.width * 0.9 && height >= screenFrame.height * 0.9
        }
    }

    private static func systemIdleSeconds() -> TimeInterval {
        CGEventSource.secondsSinceLastEventType(
            .hidSystemState,
            eventType: CGEventType(rawValue: ~0)!
        )
    }
}

enum ActivityClassifier {
    static func classify(_ snapshot: ActivitySnapshot) -> ActivityContext {
        if snapshot.idleSeconds >= 180 {
            return context(.idle, snapshot: snapshot, confidence: 95)
        }

        let app = [snapshot.frontAppName, snapshot.bundleID]
            .compactMap(\.self)
            .joined(separator: " ")
        let title = snapshot.windowTitle ?? ""

        if matches(app: app, title: title, appHints: ["Xcode", "Cursor", "Visual Studio Code", "Code", "IntelliJ", "WebStorm", "PyCharm"], titleHints: [".swift", ".tsx", ".ts", ".js", ".py", ".go", ".rs", "Pull Request", "GitHub"]) {
            return context(.coding, snapshot: snapshot, confidence: title.isEmpty ? 70 : 90)
        }

        if matches(app: app, title: title, appHints: ["Terminal", "iTerm", "Warp"], titleHints: ["zsh", "bash", "ssh", "vim", "nvim"]) {
            return context(.terminal, snapshot: snapshot, confidence: 85)
        }

        if matches(app: app, title: title, appHints: ["Zoom", "腾讯会议", "飞书会议"], titleHints: ["会议", "Meeting", "通话", "Call"]) {
            return context(.meeting, snapshot: snapshot, confidence: 90)
        }

        if matches(app: app, title: title, appHints: ["微信", "WeChat", "飞书", "Lark", "Slack", "Telegram"], titleHints: ["聊天", "chat", "channel"]) {
            return context(.chatting, snapshot: snapshot, confidence: 86)
        }

        if snapshot.isFullScreen && matches(app: app, title: title, appHints: ["IINA", "QuickTime", "TV", "Safari", "Chrome", "Arc"], titleHints: ["YouTube", "Bilibili", "Netflix", "视频", "播放"]) {
            return context(.watchingVideo, snapshot: snapshot, confidence: 88)
        }

        if matches(app: app, title: title, appHints: ["Figma", "Sketch", "Photoshop", "Illustrator"], titleHints: ["Design", "设计"]) {
            return context(.designing, snapshot: snapshot, confidence: 88)
        }

        if matches(app: app, title: title, appHints: ["Preview", "Books", "PDF"], titleHints: [".pdf", "文档", "Document"]) {
            return context(.reading, snapshot: snapshot, confidence: 78)
        }

        if matches(app: app, title: title, appHints: ["Safari", "Chrome", "Arc", "Firefox", "Edge"], titleHints: []) {
            return context(.browsing, snapshot: snapshot, confidence: 68)
        }

        return context(.unknown, snapshot: snapshot, confidence: 35)
    }

    private static func matches(app: String, title: String, appHints: [String], titleHints: [String]) -> Bool {
        appHints.contains { app.localizedCaseInsensitiveContains($0) }
            || titleHints.contains { title.localizedCaseInsensitiveContains($0) }
    }

    private static func context(_ mode: UserActivityMode, snapshot: ActivitySnapshot, confidence: Int) -> ActivityContext {
        ActivityContext(
            mode: mode,
            appName: snapshot.frontAppName,
            bundleID: snapshot.bundleID,
            windowTitle: snapshot.windowTitle,
            idleSeconds: snapshot.idleSeconds,
            confidence: confidence
        )
    }
}
