import AppKit
import ApplicationServices

@MainActor
final class AppNotificationMonitor {
    private weak var model: PetModel?
    private let settings: SmartCompanionSettings
    private var task: Task<Void, Never>?
    private var lastSeenByApp: [String: Date] = [:]
    private var lastDockStateByApp: [String: String] = [:]
    private var lastFloatingWindowIDsByApp: [String: Set<Int>] = [:]
    private var lastGenericNotificationState: String?
    private var lastNotificationTextStateByApp: [String: String] = [:]
    private var lastRealtimeNotificationStateByApp: [String: String] = [:]
    private var lastWindowStateByApp: [String: String] = [:]
    private var lastMenuBarStateByApp: [String: String] = [:]
    private var lastDatabaseRecordID: Int64?
    private var lastDatabaseBadgeByBundleID: [String: Int] = [:]
    private var hasDatabaseBadgeBaseline = false
    private var lastGenericNotificationSeen: Date?
    private let cooldown: TimeInterval = 6

    init(model: PetModel, settings: SmartCompanionSettings) {
        self.model = model
        self.settings = settings
    }

    func start() {
        task?.cancel()
        task = Task { [weak self] in
            var tick = 0
            while !Task.isCancelled {
                let watchedApps = await MainActor.run {
                    self?.parsedWatchedApps() ?? []
                }
                let databaseSnapshot = await Task.detached(priority: .utility) {
                    SystemNotificationDatabaseMonitor.snapshot(watchedApps: watchedApps)
                }.value
                await MainActor.run {
                    self?.scanOnce(
                        watchedApps: watchedApps,
                        databaseSnapshot: databaseSnapshot,
                        includeFallbacks: tick % 4 == 0
                    )
                }
                tick += 1
                try? await Task.sleep(nanoseconds: 500_000_000)
            }
        }
    }

    func stop() {
        task?.cancel()
        task = nil
    }

    func requestAccessibilityPermission() {
        let options = ["AXTrustedCheckOptionPrompt": true] as CFDictionary
        AXIsProcessTrustedWithOptions(options)
        if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility") {
            NSWorkspace.shared.open(url)
        }
    }

    func requestFullDiskAccessPermission() {
        SystemNotificationDatabaseMonitor.openFullDiskAccessSettings()
    }

    private func scanOnce(watchedApps: [String], databaseSnapshot: SystemNotificationDatabaseSnapshot, includeFallbacks: Bool) {
        guard settings.isNotificationListeningEnabled else { return }

        if !watchedApps.isEmpty {
            if AXIsProcessTrusted(), let match = changedRealtimeNotificationAXApp(watchedApps: watchedApps) {
                guard shouldEmit(appName: match.appName) else { return }
                model?.receiveAppNotification(appName: match.appName, reason: "Realtime Notification AX: \(match.detail)")
                return
            }

            if let badge = changedDatabaseBadge(snapshot: databaseSnapshot) {
                guard shouldEmit(appName: badge.appName) else { return }
                model?.receiveAppNotification(
                    appName: badge.appName,
                    bundleID: badge.bundleID,
                    message: nil,
                    reason: "UserNotifications badge changed: \(badge.badge)"
                )
                return
            }

            if let record = changedDatabaseNotification(watchedApps: watchedApps, snapshot: databaseSnapshot) {
                guard shouldEmit(appName: record.displayName) else { return }
                model?.receiveAppNotification(
                    appName: record.displayName,
                    bundleID: record.bundleID,
                    message: record.displayText.isEmpty ? nil : record.displayText,
                    reason: "UserNotifications database: \(record.source)"
                )
                return
            }

            guard includeFallbacks else { return }

            if let match = changedFloatingWindowApp(watchedApps: watchedApps) {
                guard shouldEmit(appName: match.appName) else { return }
                model?.receiveAppNotification(appName: match.appName, reason: "CG floating window: \(match.detail)")
                return
            }

            if let match = changedWindowNotificationApp(watchedApps: watchedApps) {
                guard shouldEmit(appName: match.appName) else { return }
                model?.receiveAppNotification(appName: match.appName, reason: "CG window changed: \(match.detail)")
                return
            }

            let text = notificationWindowText()
            if !text.isEmpty {
                if let match = changedNotificationTextApp(text: text, watchedApps: watchedApps) {
                    guard shouldEmit(appName: match.appName) else { return }
                    model?.receiveAppNotification(appName: match.appName, reason: "Notification text: \(match.detail)")
                    return
                }
            }

            if AXIsProcessTrusted(), let appName = changedDockNotificationApp(watchedApps: watchedApps) {
                guard shouldEmit(appName: appName) else { return }
                model?.receiveAppNotification(appName: appName, reason: "Dock AX changed")
                return
            }

            if AXIsProcessTrusted(), let appName = changedMenuBarNotificationApp(watchedApps: watchedApps) {
                guard shouldEmit(appName: appName) else { return }
                model?.receiveAppNotification(appName: appName, reason: "Menu bar AX changed")
                return
            }
        }

        guard includeFallbacks else { return }

        if watchedApps.isEmpty, let detail = changedGenericNotificationWindow(), shouldEmitGenericNotification() {
            model?.receiveGenericNotification(reason: "Generic notification changed: \(detail)")
        }
    }

    private func parsedWatchedApps() -> [String] {
        settings.notificationAppNames
            .split { character in
                character == "," || character == "，" || character == "\n" || character == " "
            }
            .map { String($0).trimmingCharacters(in: .whitespacesAndNewlines) }
            .filter { !$0.isEmpty }
    }

    private func shouldEmit(appName: String) -> Bool {
        let now = Date()
        if let lastSeen = lastSeenByApp[appName], now.timeIntervalSince(lastSeen) < cooldown {
            return false
        }
        lastSeenByApp[appName] = now
        return true
    }

    private func shouldEmitGenericNotification() -> Bool {
        let now = Date()
        if let lastSeen = lastGenericNotificationSeen, now.timeIntervalSince(lastSeen) < cooldown {
            return false
        }
        lastGenericNotificationSeen = now
        return true
    }

    private func changedDatabaseBadge(snapshot: SystemNotificationDatabaseSnapshot) -> SystemNotificationAppBadge? {
        let currentBadges = snapshot.badges.reduce(into: [String: Int]()) { output, badge in
            output[badge.bundleID] = badge.badge
        }
        defer {
            lastDatabaseBadgeByBundleID = currentBadges
            hasDatabaseBadgeBaseline = true
        }

        guard hasDatabaseBadgeBaseline else { return nil }

        return snapshot.badges.first { badge in
            let previous = lastDatabaseBadgeByBundleID[badge.bundleID] ?? 0
            return badge.badge > previous
        }
    }

    private func changedDatabaseNotification(watchedApps: [String], snapshot: SystemNotificationDatabaseSnapshot) -> SystemNotificationRecord? {
        let sortedRecords = snapshot.records
            .compactMap { record -> SystemNotificationRecord? in
                guard record.recordID != nil else { return nil }
                return record
            }
            .sorted { ($0.recordID ?? 0) > ($1.recordID ?? 0) }
        guard let latestRecordID = sortedRecords.first?.recordID else {
            return nil
        }

        guard let previousRecordID = lastDatabaseRecordID else {
            lastDatabaseRecordID = latestRecordID
            return nil
        }

        let newRecords = sortedRecords
            .filter { ($0.recordID ?? 0) > previousRecordID }
            .sorted { ($0.recordID ?? 0) < ($1.recordID ?? 0) }

        lastDatabaseRecordID = max(previousRecordID, latestRecordID)
        return newRecords.first
    }

    private func notificationWindowText() -> String {
        var values: [String] = cgNotificationWindowText()

        if AXIsProcessTrusted() {
            values.append(contentsOf: accessibilityNotificationText())
        }

        return values.joined(separator: "\n")
    }

    private func cgNotificationWindowText() -> [String] {
        guard let windows = CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID) as? [[String: Any]] else {
            return []
        }

        return windows.compactMap { window in
            let owner = window[kCGWindowOwnerName as String] as? String ?? ""
            let title = window[kCGWindowName as String] as? String ?? ""
            let layer = window[kCGWindowLayer as String] as? Int ?? 0
            let alpha = window[kCGWindowAlpha as String] as? Double ?? 1
            guard layer >= 0, alpha > 0 else { return nil }
            guard owner.localizedCaseInsensitiveContains("Notification")
                    || owner.localizedCaseInsensitiveContains("通知")
                    || title.localizedCaseInsensitiveContains("Notification")
                    || title.localizedCaseInsensitiveContains("通知") else {
                return nil
            }

            return [owner, title].filter { !$0.isEmpty }.joined(separator: " ")
        }
    }

    private func changedWindowNotificationApp(watchedApps: [String]) -> NotificationWindowMatch? {
        guard let windows = CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID) as? [[String: Any]] else {
            return nil
        }

        let windowTexts = windows.compactMap { window -> String? in
            let owner = window[kCGWindowOwnerName as String] as? String ?? ""
            let title = window[kCGWindowName as String] as? String ?? ""
            let layer = window[kCGWindowLayer as String] as? Int ?? 0
            let alpha = window[kCGWindowAlpha as String] as? Double ?? 1
            guard layer >= 0, alpha > 0 else { return nil }

            var boundsDescription = ""
            if let bounds = window[kCGWindowBounds as String] as? [String: Any] {
                let width = bounds["Width"] as? Double ?? 0
                let height = bounds["Height"] as? Double ?? 0
                boundsDescription = "\(Int(width))x\(Int(height))"
            }

            return [owner, title, "layer:\(layer)", boundsDescription].filter { !$0.isEmpty }.joined(separator: " ")
        }

        for appName in watchedApps {
            let matchingTexts = windowTexts
                .filter { $0.localizedCaseInsensitiveContains(appName) }
                .sorted()
            guard !matchingTexts.isEmpty else { continue }

            let state = matchingTexts.joined(separator: "\n")
            defer { lastWindowStateByApp[appName] = state }

            guard let previousState = lastWindowStateByApp[appName], previousState != state else {
                continue
            }

            let previousLines = Set(previousState.split(separator: "\n").map(String.init))
            let currentLines = Set(matchingTexts)
            let addedLines = currentLines.subtracting(previousLines)
            if let line = addedLines.first(where: { line in
                line.localizedCaseInsensitiveContains(appName)
            }) {
                return NotificationWindowMatch(appName: appName, detail: line)
            }
        }

        return nil
    }

    private func changedFloatingWindowApp(watchedApps: [String]) -> NotificationWindowMatch? {
        let windows = currentCGWindowEntries(watchedApps: watchedApps)

        for appName in watchedApps {
            let matchingWindows = windows
                .filter { $0.line.localizedCaseInsensitiveContains(appName) }
                .filter { isLikelyFloatingNotificationWindow($0.line) }
            let currentIDs = Set(matchingWindows.map(\.id))
            defer { lastFloatingWindowIDsByApp[appName] = currentIDs }

            guard let previousIDs = lastFloatingWindowIDsByApp[appName] else {
                continue
            }

            let addedIDs = currentIDs.subtracting(previousIDs)
            guard let addedID = addedIDs.first,
                  let window = matchingWindows.first(where: { $0.id == addedID }) else {
                continue
            }
            return NotificationWindowMatch(appName: appName, detail: window.line)
        }

        return nil
    }

    private func changedNotificationTextApp(text: String, watchedApps: [String]) -> NotificationWindowMatch? {
        let lines = text
            .split(separator: "\n")
            .map(String.init)
            .filter { !$0.isEmpty }

        for appName in watchedApps {
            let matchingLines = lines
                .filter { $0.localizedCaseInsensitiveContains(appName) }
                .sorted()
            let state = matchingLines.joined(separator: "\n")
            defer { lastNotificationTextStateByApp[appName] = state }

            guard !state.isEmpty else { continue }
            guard let previousState = lastNotificationTextStateByApp[appName] else { continue }
            guard previousState != state else { continue }
            return NotificationWindowMatch(appName: appName, detail: matchingLines.first ?? state)
        }

        return nil
    }

    private func changedRealtimeNotificationAXApp(watchedApps: [String]) -> NotificationWindowMatch? {
        let lines = realtimeNotificationAXText()
            .map { $0.trimmingCharacters(in: .whitespacesAndNewlines) }
            .filter { !$0.isEmpty }
        guard !lines.isEmpty else { return nil }

        for appName in watchedApps {
            let matchingLines = lines
                .filter { $0.localizedCaseInsensitiveContains(appName) }
                .sorted()
            let state = matchingLines.joined(separator: "\n")
            defer { lastRealtimeNotificationStateByApp[appName] = state }

            guard !state.isEmpty else { continue }
            guard let previousState = lastRealtimeNotificationStateByApp[appName] else { continue }
            guard previousState != state else { continue }
            return NotificationWindowMatch(appName: appName, detail: matchingLines.first ?? state)
        }

        return nil
    }

    private func realtimeNotificationAXText() -> [String] {
        notificationCenterApplications().flatMap { app -> [String] in
            guard app.processIdentifier > 0 else { return [] }
            let element = AXUIElementCreateApplication(app.processIdentifier)
            return collectRealtimeNotificationAccessibilityText(from: element, depth: 0)
        }
    }

    private func notificationCenterApplications() -> [NSRunningApplication] {
        NSWorkspace.shared.runningApplications.filter { app in
            let name = app.localizedName ?? ""
            let bundleID = app.bundleIdentifier ?? ""
            return bundleID.localizedCaseInsensitiveContains("notificationcenter")
                || bundleID.localizedCaseInsensitiveContains("usernotification")
                || name.localizedCaseInsensitiveContains("Notification")
                || name.localizedCaseInsensitiveContains("通知")
                || name.localizedCaseInsensitiveContains("UserNotificationCenter")
        }
    }

    private func isLikelyFloatingNotificationWindow(_ text: String) -> Bool {
        guard let layerRange = text.range(of: #"layer:(\d+)"#, options: .regularExpression) else {
            return false
        }
        let layerText = text[layerRange].replacingOccurrences(of: "layer:", with: "")
        return (Int(layerText) ?? 0) >= 8
    }

    private func changedGenericNotificationWindow() -> String? {
        let entries = currentGenericNotificationEntries()
        let state = entries
            .map(\.line)
            .sorted()
            .joined(separator: "\n")
        defer { lastGenericNotificationState = state }

        guard !state.isEmpty else { return nil }
        guard let previousState = lastGenericNotificationState else { return nil }
        guard previousState != state else { return nil }

        let previousLines = Set(previousState.split(separator: "\n").map(String.init))
        let currentLines = Set(state.split(separator: "\n").map(String.init))
        let addedLines = currentLines.subtracting(previousLines)
        return addedLines.first ?? currentLines.first
    }

    private func accessibilityNotificationText() -> [String] {
        NSWorkspace.shared.runningApplications
            .filter { app in
                let name = app.localizedName ?? ""
                return name.localizedCaseInsensitiveContains("Notification")
                    || name.localizedCaseInsensitiveContains("通知")
            }
            .flatMap { app -> [String] in
                guard app.processIdentifier > 0 else { return [] }
                let element = AXUIElementCreateApplication(app.processIdentifier)
                return collectAccessibilityText(from: element, depth: 0)
            }
    }

    private func changedDockNotificationApp(watchedApps: [String]) -> String? {
        guard let dock = NSWorkspace.shared.runningApplications.first(where: { $0.bundleIdentifier == "com.apple.dock" }) else {
            return nil
        }

        let element = AXUIElementCreateApplication(dock.processIdentifier)
        let texts = collectAccessibilityText(from: element, depth: 0)
        guard !texts.isEmpty else { return nil }

        for appName in watchedApps {
            let matchingTexts = texts.filter { $0.localizedCaseInsensitiveContains(appName) }
            guard !matchingTexts.isEmpty else { continue }

            let state = matchingTexts.joined(separator: " | ")
            defer { lastDockStateByApp[appName] = state }

            guard let previousState = lastDockStateByApp[appName],
                  previousState != state,
                  dockTextSuggestsNotification(state, appName: appName) else {
                continue
            }
            return appName
        }

        return nil
    }

    private func changedMenuBarNotificationApp(watchedApps: [String]) -> String? {
        guard let systemUIServer = NSWorkspace.shared.runningApplications.first(where: { $0.bundleIdentifier == "com.apple.systemuiserver" }) else {
            return nil
        }

        let element = AXUIElementCreateApplication(systemUIServer.processIdentifier)
        let texts = collectAccessibilityText(from: element, depth: 0)
        guard !texts.isEmpty else { return nil }

        for appName in watchedApps {
            let matchingTexts = texts.filter { $0.localizedCaseInsensitiveContains(appName) }
            guard !matchingTexts.isEmpty else { continue }

            let state = matchingTexts.joined(separator: " | ")
            defer { lastMenuBarStateByApp[appName] = state }

            guard let previousState = lastMenuBarStateByApp[appName],
                  previousState != state,
                  dockTextSuggestsNotification(state, appName: appName) else {
                continue
            }
            return appName
        }

        return nil
    }

    func diagnosticReport() -> String {
        let watchedApps = parsedWatchedApps()
        var sections: [String] = []
        sections.append("AX trusted optional fallback: \(AXIsProcessTrusted() ? "yes" : "no")")
        sections.append("Notification listening: \(settings.isNotificationListeningEnabled ? "on" : "off")")
        sections.append("Watched: \(watchedApps.joined(separator: ", "))")

        let databaseSnapshot = SystemNotificationDatabaseMonitor.diagnosticSnapshot(watchedApps: watchedApps)
        sections.append("UserNotifications database:\n\(databaseSnapshot.diagnostics.isEmpty ? "(empty)" : databaseSnapshot.diagnostics)")
        let databaseRecords = databaseSnapshot.records.prefix(12).map { record in
            [
                record.displayName,
                record.bundleID ?? "",
                record.title ?? "",
                record.body ?? "",
                record.source
            ]
            .filter { !$0.isEmpty }
            .joined(separator: " | ")
        }
        sections.append("Recent database notifications:\n\(databaseRecords.isEmpty ? "(empty)" : databaseRecords.joined(separator: "\n"))")

        let cgLines = currentCGWindowLines(watchedApps: watchedApps)
        sections.append("CG windows:\n\(cgLines.isEmpty ? "(empty)" : cgLines.joined(separator: "\n"))")
        let floatingEntries = currentCGWindowEntries(watchedApps: watchedApps)
            .filter { isLikelyFloatingNotificationWindow($0.line) }
        if let entry = floatingEntries.first,
           let appName = watchedApps.first(where: { entry.line.localizedCaseInsensitiveContains($0) }) {
            sections.append("Floating baseline:\n\(appName) via \(entry.line)")
        } else {
            sections.append("Floating baseline:\n(empty)")
        }

        let genericNotificationEntries = currentGenericNotificationEntries()
        sections.append("Generic notification windows:\n\(genericNotificationEntries.isEmpty ? "(empty)" : genericNotificationEntries.map(\.line).joined(separator: "\n"))")

        if AXIsProcessTrusted() {
            let dockLines = currentAXLines(bundleID: "com.apple.dock", watchedApps: watchedApps)
            sections.append("Dock AX:\n\(dockLines.isEmpty ? "(empty)" : dockLines.joined(separator: "\n"))")

            let menuLines = currentAXLines(bundleID: "com.apple.systemuiserver", watchedApps: watchedApps)
            sections.append("Menu bar AX:\n\(menuLines.isEmpty ? "(empty)" : menuLines.joined(separator: "\n"))")

            let notificationLines = realtimeNotificationAXText()
                .filter { line in
                    watchedApps.contains { line.localizedCaseInsensitiveContains($0) }
                        || line.localizedCaseInsensitiveContains("Notification")
                        || line.localizedCaseInsensitiveContains("通知")
                }
                .prefix(80)
            sections.append("Notification AX:\n\(notificationLines.isEmpty ? "(empty)" : notificationLines.joined(separator: "\n"))")
        }

        return sections.joined(separator: "\n\n")
    }

    private func currentCGWindowLines(watchedApps: [String]) -> [String] {
        currentCGWindowEntries(watchedApps: watchedApps).map(\.line)
    }

    private func currentCGWindowEntries(watchedApps: [String]) -> [CGWindowEntry] {
        guard let windows = CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID) as? [[String: Any]] else {
            return []
        }

        return windows.compactMap { window -> CGWindowEntry? in
            let id = window[kCGWindowNumber as String] as? Int ?? 0
            let owner = window[kCGWindowOwnerName as String] as? String ?? ""
            let title = window[kCGWindowName as String] as? String ?? ""
            let layer = window[kCGWindowLayer as String] as? Int ?? 0
            let alpha = window[kCGWindowAlpha as String] as? Double ?? 1
            var boundsDescription = ""
            if let bounds = window[kCGWindowBounds as String] as? [String: Any] {
                let width = bounds["Width"] as? Double ?? 0
                let height = bounds["Height"] as? Double ?? 0
                let x = bounds["X"] as? Double ?? 0
                let y = bounds["Y"] as? Double ?? 0
                boundsDescription = "\(Int(width))x\(Int(height))@\(Int(x)),\(Int(y))"
            }
            let text = [owner, title, "id:\(id)", "layer:\(layer)", "alpha:\(String(format: "%.2f", alpha))", boundsDescription]
                .filter { !$0.isEmpty }
                .joined(separator: " ")
            guard watchedApps.contains(where: { text.localizedCaseInsensitiveContains($0) })
                    || text.localizedCaseInsensitiveContains("Notification")
                    || text.localizedCaseInsensitiveContains("通知") else {
                return nil
            }
            return CGWindowEntry(id: id, line: text)
        }
        .prefix(80)
        .map { $0 }
    }

    private func currentGenericNotificationEntries() -> [CGWindowEntry] {
        guard let windows = CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID) as? [[String: Any]] else {
            return []
        }

        return windows.compactMap { window -> CGWindowEntry? in
            guard isLikelySystemNotificationWindow(window) else { return nil }
            return cgWindowEntry(from: window)
        }
        .prefix(40)
        .map { $0 }
    }

    private func isLikelySystemNotificationWindow(_ window: [String: Any]) -> Bool {
        let owner = window[kCGWindowOwnerName as String] as? String ?? ""
        let title = window[kCGWindowName as String] as? String ?? ""
        let layer = window[kCGWindowLayer as String] as? Int ?? 0
        let alpha = window[kCGWindowAlpha as String] as? Double ?? 1
        guard alpha > 0.05, layer >= 8 else { return false }

        guard let bounds = window[kCGWindowBounds as String] as? [String: Any] else { return false }
        let width = bounds["Width"] as? Double ?? 0
        let height = bounds["Height"] as? Double ?? 0
        let x = bounds["X"] as? Double ?? 0
        let y = bounds["Y"] as? Double ?? 0
        let maxScreenX = NSScreen.screens.map(\.frame.maxX).max() ?? 0
        let minScreenY = NSScreen.screens.map(\.frame.minY).min() ?? 0
        let maxScreenWidth = NSScreen.screens.map(\.frame.width).max() ?? 0
        let maxScreenHeight = NSScreen.screens.map(\.frame.height).max() ?? 0
        let isFullScreenLayer = width >= maxScreenWidth * 0.90 && height >= maxScreenHeight * 0.90
        guard !isFullScreenLayer else { return false }

        let isNotificationSized = width >= 180 && width <= 720 && height >= 48 && height <= 360
        let isNearRightTop = x >= maxScreenX - 900 && y <= minScreenY + 280
        guard isNotificationSized && isNearRightTop else { return false }

        if owner.localizedCaseInsensitiveContains("Notification")
            || owner.localizedCaseInsensitiveContains("通知")
            || title.localizedCaseInsensitiveContains("Notification")
            || title.localizedCaseInsensitiveContains("通知") {
            return true
        }

        let ignoredOwners = ["TopiPal", "TopiIsland", "Dock", "Window Server", "Control Center", "Spotlight"]
        let isIgnoredOwner = ignoredOwners.contains { owner.localizedCaseInsensitiveContains($0) }
        return !isIgnoredOwner
    }

    private func cgWindowEntry(from window: [String: Any]) -> CGWindowEntry {
        let id = window[kCGWindowNumber as String] as? Int ?? 0
        let owner = window[kCGWindowOwnerName as String] as? String ?? ""
        let title = window[kCGWindowName as String] as? String ?? ""
        let layer = window[kCGWindowLayer as String] as? Int ?? 0
        let alpha = window[kCGWindowAlpha as String] as? Double ?? 1
        var boundsDescription = ""
        if let bounds = window[kCGWindowBounds as String] as? [String: Any] {
            let width = bounds["Width"] as? Double ?? 0
            let height = bounds["Height"] as? Double ?? 0
            let x = bounds["X"] as? Double ?? 0
            let y = bounds["Y"] as? Double ?? 0
            boundsDescription = "\(Int(width))x\(Int(height))@\(Int(x)),\(Int(y))"
        }
        let line = [owner, title, "id:\(id)", "layer:\(layer)", "alpha:\(String(format: "%.2f", alpha))", boundsDescription]
            .filter { !$0.isEmpty }
            .joined(separator: " ")
        return CGWindowEntry(id: id, line: line)
    }

    private func currentAXLines(bundleID: String, watchedApps: [String]) -> [String] {
        guard let app = NSWorkspace.shared.runningApplications.first(where: { $0.bundleIdentifier == bundleID }) else {
            return []
        }
        let element = AXUIElementCreateApplication(app.processIdentifier)
        return collectAccessibilityText(from: element, depth: 0)
            .filter { line in
                watchedApps.contains { line.localizedCaseInsensitiveContains($0) }
                    || line.localizedCaseInsensitiveContains("Notification")
                    || line.localizedCaseInsensitiveContains("通知")
                    || line.range(of: #"\d+"#, options: .regularExpression) != nil
            }
            .prefix(80)
            .map { $0 }
    }

    private func dockTextSuggestsNotification(_ text: String, appName: String) -> Bool {
        let normalized = text.replacingOccurrences(of: appName, with: "", options: [.caseInsensitive, .diacriticInsensitive])
        return normalized.range(of: #"\d+"#, options: .regularExpression) != nil
            || normalized.localizedCaseInsensitiveContains("通知")
            || normalized.localizedCaseInsensitiveContains("notification")
            || normalized.localizedCaseInsensitiveContains("未读")
            || normalized.localizedCaseInsensitiveContains("new")
    }

    private func collectAccessibilityText(from element: AXUIElement, depth: Int) -> [String] {
        guard depth < 4 else { return [] }

        var output: [String] = []
        for attribute in [kAXTitleAttribute, kAXValueAttribute, kAXDescriptionAttribute, kAXHelpAttribute] {
            var value: CFTypeRef?
            if AXUIElementCopyAttributeValue(element, attribute as CFString, &value) == .success,
               let string = value as? String,
               !string.isEmpty {
                output.append(string)
            }
        }

        var children: CFTypeRef?
        if AXUIElementCopyAttributeValue(element, kAXChildrenAttribute as CFString, &children) == .success,
           let childElements = children as? [AXUIElement] {
            for child in childElements.prefix(24) {
                output.append(contentsOf: collectAccessibilityText(from: child, depth: depth + 1))
            }
        }

        var windows: CFTypeRef?
        if depth == 0,
           AXUIElementCopyAttributeValue(element, kAXWindowsAttribute as CFString, &windows) == .success,
           let windowElements = windows as? [AXUIElement] {
            for window in windowElements.prefix(8) {
                output.append(contentsOf: collectAccessibilityText(from: window, depth: depth + 1))
            }
        }

        return output
    }

    private func collectRealtimeNotificationAccessibilityText(from element: AXUIElement, depth: Int) -> [String] {
        guard depth < 7 else { return [] }

        var output: [String] = []
        for attribute in [
            kAXTitleAttribute,
            kAXValueAttribute,
            kAXDescriptionAttribute,
            kAXHelpAttribute,
            kAXRoleDescriptionAttribute
        ] {
            var value: CFTypeRef?
            if AXUIElementCopyAttributeValue(element, attribute as CFString, &value) == .success,
               let string = value as? String,
               !string.isEmpty {
                output.append(string)
            }
        }

        var children: CFTypeRef?
        if AXUIElementCopyAttributeValue(element, kAXChildrenAttribute as CFString, &children) == .success,
           let childElements = children as? [AXUIElement] {
            for child in childElements.prefix(80) {
                output.append(contentsOf: collectRealtimeNotificationAccessibilityText(from: child, depth: depth + 1))
            }
        }

        var windows: CFTypeRef?
        if depth == 0,
           AXUIElementCopyAttributeValue(element, kAXWindowsAttribute as CFString, &windows) == .success,
           let windowElements = windows as? [AXUIElement] {
            for window in windowElements.prefix(12) {
                output.append(contentsOf: collectRealtimeNotificationAccessibilityText(from: window, depth: depth + 1))
            }
        }

        return output
    }
}

private struct NotificationWindowMatch {
    let appName: String
    let detail: String
}

private struct CGWindowEntry {
    let id: Int
    let line: String
}
