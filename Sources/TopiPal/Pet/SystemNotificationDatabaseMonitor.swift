import AppKit
import Foundation
import SQLite3

struct SystemNotificationRecord: Sendable {
    let recordID: Int64?
    let appName: String?
    let bundleID: String?
    let title: String?
    let body: String?
    let source: String
    let fingerprint: String

    var displayName: String {
        appName?.isEmpty == false ? appName! : "系统通知"
    }

    var displayText: String {
        let parts = [title, body]
            .compactMap { $0?.trimmingCharacters(in: .whitespacesAndNewlines) }
            .filter { !$0.isEmpty }
        return parts.joined(separator: "：")
    }
}

struct SystemNotificationAppBadge: Sendable {
    let appName: String
    let bundleID: String
    let badge: Int
}

struct SystemNotificationDatabaseSnapshot: Sendable {
    let records: [SystemNotificationRecord]
    let badges: [SystemNotificationAppBadge]
    let diagnostics: String
}

enum SystemNotificationDatabaseMonitor {
    static func snapshot(watchedApps: [String]) -> SystemNotificationDatabaseSnapshot {
        targetedUsernotedSnapshot(watchedApps: watchedApps)
    }

    static func diagnosticSnapshot(watchedApps: [String]) -> SystemNotificationDatabaseSnapshot {
        targetedUsernotedSnapshot(watchedApps: watchedApps, includeDiagnostics: true)
    }

    static func protectedSnapshot(watchedApps: [String]) -> SystemNotificationDatabaseSnapshot {
        targetedUsernotedSnapshot(watchedApps: watchedApps, includeDiagnostics: true)
    }

    private static func targetedUsernotedSnapshot(watchedApps: [String], includeDiagnostics: Bool = false) -> SystemNotificationDatabaseSnapshot {
        let url = usernotedDatabaseURL()
        guard FileManager.default.fileExists(atPath: url.path) else {
            return SystemNotificationDatabaseSnapshot(records: [], badges: [], diagnostics: "\(url.path): missing")
        }

        let result = readUsernotedDatabase(url, watchedApps: watchedApps)
        return SystemNotificationDatabaseSnapshot(
            records: result.records,
            badges: result.badges,
            diagnostics: includeDiagnostics ? result.diagnostic : ""
        )
    }

    static func openFullDiskAccessSettings() {
        if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_AllFiles") {
            NSWorkspace.shared.open(url)
        }
    }

    private static func candidateDirectories(includeProtectedSearch: Bool) -> [URL] {
        let home = FileManager.default.homeDirectoryForCurrentUser
        var directories = [
            home.appendingPathComponent("Library/Group Containers/group.com.apple.UserNotifications"),
            home.appendingPathComponent("Library/Group Containers/group.com.apple.usernoted"),
            home.appendingPathComponent("Library/Application Support/NotificationCenter")
        ]

        if includeProtectedSearch {
            directories.append(contentsOf: notificationDaemonContainerDirectories(in: home))
        }
        return directories
    }

    private static func usernotedDatabaseURL() -> URL {
        FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/Group Containers/group.com.apple.usernoted/db2/db")
    }

    private static func readUsernotedDatabase(_ url: URL, watchedApps: [String]) -> (records: [SystemNotificationRecord], badges: [SystemNotificationAppBadge], diagnostic: String) {
        let openResult = openUsernotedDatabase(url)
        switch openResult {
        case .success(let opened):
            let openedDB = opened.0
            let mode = opened.1
            let result = readOpenedUsernotedDatabase(openedDB, url: url, watchedApps: watchedApps, openMode: mode)
            sqlite3_close(openedDB)
            return result
        case .failure(let error):
            return ([], [], "\(url.path): open failed: \(permissionHint(forSQLiteError: error.message))")
        }
    }

    private static func openUsernotedDatabase(_ url: URL) -> Result<(OpaquePointer, String), DatabaseOpenError> {
        let flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_URI
        let attempts = [
            ("readonly", "file:\(url.path)?mode=ro"),
            ("immutable", "file:\(url.path)?mode=ro&immutable=1")
        ]

        var errors: [String] = []
        for (mode, sqliteURL) in attempts {
            var db: OpaquePointer?
            if sqlite3_open_v2(sqliteURL, &db, flags, nil) == SQLITE_OK, let db {
                return .success((db, mode))
            }
            let message = db.map { String(cString: sqlite3_errmsg($0)) } ?? "open failed"
            errors.append("\(mode): \(message)")
            if let db { sqlite3_close(db) }
        }
        return .failure(DatabaseOpenError(message: errors.joined(separator: "; ")))
    }

    private static func readOpenedUsernotedDatabase(
        _ db: OpaquePointer,
        url: URL,
        watchedApps: [String],
        openMode: String
    ) -> (records: [SystemNotificationRecord], badges: [SystemNotificationAppBadge], diagnostic: String) {
        let tables = Set(queryRows(db, sql: "select name from sqlite_master where type = 'table'").compactMap { $0["name"] })
        guard tables.contains("app"), tables.contains("record") else {
            return ([], [], "\(url.path): readable, app/record tables missing")
        }

        let appRows = queryRows(db, sql: "select app_id, identifier, badge from app")
        let appIdentifiersByID = appRows
            .reduce(into: [String: String]()) { output, row in
                guard let appID = row["app_id"], let identifier = row["identifier"], !identifier.isEmpty else { return }
                output[appID] = identifier
            }
        let badges = appRows.compactMap { badge(from: $0, watchedApps: watchedApps) }

        let sql = """
        select rec_id, app_id, uuid, data, request_date, request_last_date, delivered_date, presented, style
        from record
        order by rec_id desc
        limit 80
        """
        let rows = queryRowsWithBlob(db, sql: sql)
        let records = rows.compactMap { row in
            usernotedRecord(from: row, databasePath: url.path, appIdentifiersByID: appIdentifiersByID, watchedApps: watchedApps)
        }
        let recentApps = recentUsernotedAppDiagnostics(rows: rows, appIdentifiersByID: appIdentifiersByID)

        let watchedDescription = watchedApps.isEmpty ? "(all)" : watchedApps.joined(separator: ", ")
        let diagnostic = """
        \(url.path): readable
        mode:\(openMode)
        app rows:\(appIdentifiersByID.count), badge matched:\(badges.count), record sample:\(rows.count), matched:\(records.count)
        watched:\(watchedDescription)
        watched badges:
        \(badges.isEmpty ? "(empty)" : badges.map { "\($0.appName) | \($0.bundleID) | badge:\($0.badge)" }.joined(separator: "\n"))
        recent apps:
        \(recentApps.isEmpty ? "(empty)" : recentApps.joined(separator: "\n"))
        """
        return (records, badges, diagnostic)
    }

    private static func badge(from row: [String: String], watchedApps: [String]) -> SystemNotificationAppBadge? {
        guard let bundleID = row["identifier"], !bundleID.isEmpty else { return nil }
        let appName = appName(forBundleID: bundleID) ?? humanReadableAppName(bundleID)
        let watchedText = [bundleID, appName].joined(separator: "\n")
        if !watchedApps.isEmpty {
            let isWatched = watchedApps.contains { watchedApp in
                watchedText.localizedCaseInsensitiveContains(watchedApp)
                    || watchedApp.localizedCaseInsensitiveContains(watchedText)
                    || notificationAppAliases(for: watchedApp).contains { alias in
                        watchedText.localizedCaseInsensitiveContains(alias)
                    }
            }
            guard isWatched else { return nil }
        }

        return SystemNotificationAppBadge(
            appName: appName,
            bundleID: bundleID,
            badge: Int(row["badge"] ?? "") ?? 0
        )
    }

    private static func recentUsernotedAppDiagnostics(
        rows: [[String: DatabaseValue]],
        appIdentifiersByID: [String: String]
    ) -> [String] {
        rows.prefix(20).map { row in
            let recID = row["rec_id"]?.stringValue ?? "?"
            let appID = row["app_id"]?.stringValue ?? "?"
            let bundleID = appIdentifiersByID[appID] ?? "unknown"
            let appName = appName(forBundleID: bundleID) ?? humanReadableAppName(bundleID)
            let delivered = row["delivered_date"]?.stringValue ?? ""
            return "\(recID) | \(appName) | \(bundleID) | \(delivered)"
        }
    }

    private static func permissionHint(forSQLiteError message: String) -> String {
        if message.localizedCaseInsensitiveContains("authorization denied")
            || message.localizedCaseInsensitiveContains("not authorized")
            || message.localizedCaseInsensitiveContains("unable to open database file")
            || message.localizedCaseInsensitiveContains("operation not permitted") {
            return "\(message)。需要在 系统设置 > 隐私与安全性 > 完全磁盘访问 中允许当前 TopiPal.app。"
        }
        return message
    }

    private static func usernotedRecord(
        from row: [String: DatabaseValue],
        databasePath: String,
        appIdentifiersByID: [String: String],
        watchedApps: [String]
    ) -> SystemNotificationRecord? {
        guard let recID = row["rec_id"]?.stringValue else { return nil }
        let appID = row["app_id"]?.stringValue ?? ""
        let bundleID = appIdentifiersByID[appID]
        let appName = appName(forBundleID: bundleID)
        let watchedText = [bundleID, appName].compactMap(\.self).joined(separator: "\n")

        if !watchedApps.isEmpty {
            let isWatched = watchedApps.contains { watchedApp in
                watchedText.localizedCaseInsensitiveContains(watchedApp)
                    || watchedApp.localizedCaseInsensitiveContains(watchedText)
                    || notificationAppAliases(for: watchedApp).contains { alias in
                        watchedText.localizedCaseInsensitiveContains(alias)
                    }
            }
            guard isWatched else { return nil }
        }

        let payload = row["data"]?.dataValue.flatMap(cleanNotificationPayload)
        let uuid = row["uuid"]?.stringValue ?? ""
        let fingerprint = stableFingerprint([databasePath, "record", recID, uuid, bundleID ?? ""])
        let title = payload?.title
        let body = payload?.body

        return SystemNotificationRecord(
            recordID: Int64(recID),
            appName: appName ?? bundleID.map(humanReadableAppName),
            bundleID: bundleID,
            title: title,
            body: body,
            source: "\(databasePath)#record:\(recID)",
            fingerprint: fingerprint
        )
    }

    private static func appName(forBundleID bundleID: String?) -> String? {
        guard let bundleID, !bundleID.isEmpty else { return nil }
        let known: [(String, String)] = [
            ("com.tencent.xinWeChat", "微信"),
            ("com.tencent.xinwechat", "微信"),
            ("WeChat", "微信"),
            ("com.tencent.WeWorkMac", "企业微信"),
            ("com.tencent.qq", "QQ"),
            ("com.tencent.tim", "TIM"),
            ("com.larksuite", "飞书"),
            ("com.bytedance.ee.lark", "飞书"),
            ("com.electron.lark", "飞书"),
            ("com.ss.lark", "飞书"),
            ("com.bytedance.lark", "飞书"),
            ("com.larksuite.feishu", "飞书"),
            ("Lark", "飞书"),
            ("Feishu", "飞书"),
            ("com.hnc.Discord", "Discord"),
            ("com.tinyspeck.slackmacgap", "Slack"),
            ("com.microsoft.teams", "Teams"),
            ("us.zoom.xos", "Zoom"),
            ("ru.keepcoder.Telegram", "Telegram"),
            ("com.apple.iChat", "信息"),
            ("com.apple.MobileSMS", "信息"),
            ("com.apple.mail", "邮件"),
            ("com.apple.reminders", "提醒事项"),
            ("com.apple.iCal", "日历")
        ]
        return known.first { knownBundleID, _ in
            bundleID.localizedCaseInsensitiveContains(knownBundleID)
                || knownBundleID.localizedCaseInsensitiveContains(bundleID)
        }?.1
    }

    private static func notificationAppAliases(for watchedApp: String) -> [String] {
        if watchedApp.localizedCaseInsensitiveContains("微信")
            || watchedApp.localizedCaseInsensitiveContains("WeChat") {
            return ["com.tencent.xinWeChat", "com.tencent.xinwechat", "WeChat"]
        }
        if watchedApp.localizedCaseInsensitiveContains("飞书")
            || watchedApp.localizedCaseInsensitiveContains("Lark")
            || watchedApp.localizedCaseInsensitiveContains("Feishu") {
            return ["com.larksuite", "com.bytedance.ee.lark", "com.electron.lark", "com.ss.lark", "com.bytedance.lark", "com.larksuite.feishu", "Lark", "Feishu"]
        }
        return []
    }

    private static func cleanNotificationPayload(_ data: Data) -> (title: String?, body: String?) {
        // The usernoted payload is an NSKeyedArchiver blob. Until we parse the
        // archive structurally, showing extracted strings risks leaking internal
        // tokens such as wxid/category identifiers. The notification source is
        // reliable from the app table, so bubbles intentionally omit body text.
        _ = data
        return (nil, nil)
    }

    private static func isCleanPayloadText(_ token: String) -> Bool {
        guard token.count >= 2, token.count <= 80 else { return false }
        let rejectedFragments = [
            "$archiver",
            "$objects",
            "$top",
            "$version",
            "NSKeyedArchiver",
            "NSObject",
            "WNS.keys",
            "ZNS.objects",
            "Tbody",
            "Ttitl",
            "Troot",
            "Tstyl",
            "Sapp",
            "Sreq",
            "Snam",
            "notify.caf",
            "bplist",
            "wxid_",
            "unique_id",
            "$classname",
            "chatname",
            "Tdest",
            "Tusda",
            "Tsoun",
            "Tdate",
            "Torig",
            "Sopt"
        ]
        guard !rejectedFragments.contains(where: { token.localizedCaseInsensitiveContains($0) }) else {
            return false
        }
        guard !token.allSatisfy({ $0.isNumber || $0 == "." || $0 == "-" || $0 == "_" }) else {
            return false
        }
        return true
    }

    private static func notificationDaemonContainerDirectories(in home: URL) -> [URL] {
        let daemonContainers = home.appendingPathComponent("Library/Daemon Containers")
        guard let contents = try? FileManager.default.contentsOfDirectory(
            at: daemonContainers,
            includingPropertiesForKeys: [.isDirectoryKey],
            options: []
        ) else {
            return []
        }

        var directories: [URL] = []
        for url in contents {
            let values = try? url.resourceValues(forKeys: [.isDirectoryKey])
            guard values?.isDirectory == true else { continue }
            directories.append(url.appendingPathComponent("Data/Library/UserNotifications"))
            directories.append(url.appendingPathComponent("Data/Library/NotificationCenter"))
        }
        return directories
    }

    private static func databaseFiles(in directory: URL) throws -> [URL] {
        guard let enumerator = FileManager.default.enumerator(
            at: directory,
            includingPropertiesForKeys: [.isRegularFileKey],
            options: []
        ) else {
            return []
        }

        return enumerator.compactMap { item -> URL? in
            guard let url = item as? URL else { return nil }
            let name = url.lastPathComponent.lowercased()
            let pathExtension = url.pathExtension.lowercased()
            let looksNamedLikeDatabase = name == "db"
                    || name == "db2"
                    || pathExtension == "db"
                    || pathExtension == "sqlite"
                    || pathExtension == "sqlite3"
                    || pathExtension == "storedata"
                    || pathExtension == "store"
            let values = try? url.resourceValues(forKeys: [.isRegularFileKey])
            guard values?.isRegularFile == true else { return nil }
            if looksNamedLikeDatabase || isSQLiteDatabase(url) {
                return url
            }
            return nil
        }
    }

    private static func directorySamples(in directory: URL) throws -> [String] {
        guard let enumerator = FileManager.default.enumerator(
            at: directory,
            includingPropertiesForKeys: [.isRegularFileKey, .fileSizeKey],
            options: []
        ) else {
            return []
        }

        return enumerator.compactMap { item -> String? in
            guard let url = item as? URL else { return nil }
            let values = try? url.resourceValues(forKeys: [.isRegularFileKey, .fileSizeKey])
            guard values?.isRegularFile == true else { return nil }
            let relativePath = url.path.replacingOccurrences(of: directory.path + "/", with: "")
            let size = values?.fileSize ?? 0
            let marker = isSQLiteDatabase(url) ? " sqlite" : ""
            return "\(relativePath) \(size)b\(marker)"
        }
        .prefix(80)
        .map { $0 }
    }

    private static func isSQLiteDatabase(_ url: URL) -> Bool {
        guard let handle = try? FileHandle(forReadingFrom: url) else { return false }
        defer { try? handle.close() }
        let data = (try? handle.read(upToCount: 16)) ?? Data()
        guard let header = String(data: data, encoding: .ascii) else { return false }
        return header.hasPrefix("SQLite format 3")
    }

    private static func readDatabase(_ url: URL, watchedApps: [String]) -> (records: [SystemNotificationRecord], diagnostic: String) {
        var db: OpaquePointer?
        let flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX
        guard sqlite3_open_v2(url.path, &db, flags, nil) == SQLITE_OK, let db else {
            let message = db.map { String(cString: sqlite3_errmsg($0)) } ?? "open failed"
            if let db { sqlite3_close(db) }
            return ([], "\(url.path): open failed: \(message)")
        }
        defer { sqlite3_close(db) }

        let tables = queryRows(db, sql: "select name from sqlite_master where type = 'table'")
            .compactMap { $0["name"] }
            .filter { !$0.hasPrefix("sqlite_") }

        var records: [SystemNotificationRecord] = []
        var tableDiagnostics: [String] = []
        let appHints = buildAppHints(db: db, tables: tables)

        for table in tables {
            let columns = tableColumns(db, table: table)
            guard isLikelyNotificationTable(columns: columns) else { continue }

            let selectedColumns = columns.map { quoteIdentifier($0) }.joined(separator: ", ")
            let sql = "select rowid as __topi_rowid, \(selectedColumns) from \(quoteIdentifier(table)) order by rowid desc limit 40"
            let rows = queryRows(db, sql: sql)
            tableDiagnostics.append("\(table): \(columns.joined(separator: ", ")) rows:\(rows.count)")

            for row in rows {
                guard let record = record(from: row, databasePath: url.path, table: table, watchedApps: watchedApps, appHints: appHints) else {
                    continue
                }
                records.append(record)
            }
        }

        let diagnostic = "\(url.path): readable\n" + (tableDiagnostics.isEmpty ? "no likely notification table" : tableDiagnostics.joined(separator: "\n"))
        return (records, diagnostic)
    }

    private static func tableColumns(_ db: OpaquePointer, table: String) -> [String] {
        queryRows(db, sql: "pragma table_info(\(quoteIdentifier(table)))")
            .compactMap { $0["name"] }
    }

    private static func isLikelyNotificationTable(columns: [String]) -> Bool {
        let text = columns.joined(separator: " ").lowercased()
        let hasNotificationSignal = [
            "notification",
            "request",
            "record",
            "bundle",
            "identifier",
            "content",
            "payload",
            "body",
            "title"
        ].contains { text.contains($0) }
        let hasTimeSignal = [
            "date",
            "time",
            "timestamp",
            "delivered",
            "presented",
            "created"
        ].contains { text.contains($0) }
        return hasNotificationSignal || hasTimeSignal
    }

    private static func record(
        from row: [String: String],
        databasePath: String,
        table: String,
        watchedApps: [String],
        appHints: [String: AppHint]
    ) -> SystemNotificationRecord? {
        let haystack = row.values.joined(separator: "\n")
        guard !haystack.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else { return nil }

        let directBundleID = inferredBundleID(from: row, fallbackText: haystack)
        let hint = inferredAppHint(from: row, appHints: appHints)
        let bundleID = directBundleID ?? hint?.bundleID
        let appName = inferredAppName(
            from: row,
            fallbackText: haystack,
            bundleID: bundleID,
            hintedName: hint?.name,
            watchedApps: watchedApps
        )
        let title = preferredValue(in: row, matching: ["title", "subtitle", "header", "summary"])
        let body = preferredValue(in: row, matching: ["body", "message", "text", "content", "payload", "data"])
        let rowID = row["__topi_rowid"] ?? UUID().uuidString
        let fingerprint = stableFingerprint([databasePath, table, rowID, appName ?? "", bundleID ?? "", title ?? "", body ?? ""])

        if appName == nil, title == nil, body == nil {
            return nil
        }

        return SystemNotificationRecord(
            recordID: Int64(rowID),
            appName: appName,
            bundleID: bundleID,
            title: title,
            body: body,
            source: "\(databasePath)#\(table):\(rowID)",
            fingerprint: fingerprint
        )
    }

    private static func inferredBundleID(from row: [String: String], fallbackText text: String) -> String? {
        let candidates = row
            .filter { key, value in
                !value.isEmpty && (
                    key.localizedCaseInsensitiveContains("bundle")
                        || key.localizedCaseInsensitiveContains("identifier")
                        || key.localizedCaseInsensitiveContains("app")
                        || key.localizedCaseInsensitiveContains("source")
                )
            }
            .map(\.value) + [text]

        let pattern = #"[A-Za-z0-9-]+(?:\.[A-Za-z0-9_-]+){2,}"#
        for candidate in candidates {
            guard let range = candidate.range(of: pattern, options: .regularExpression) else { continue }
            return String(candidate[range])
        }
        return nil
    }

    private static func inferredAppName(
        from row: [String: String],
        fallbackText text: String,
        bundleID: String?,
        hintedName: String?,
        watchedApps: [String]
    ) -> String? {
        let sourceText = preferredValue(
            in: row,
            matching: ["bundle", "bundleid", "bundle_identifier", "app", "application", "source", "publisher", "identifier"]
        ) ?? [bundleID, hintedName, text].compactMap(\.self).joined(separator: "\n")

        if let app = watchedApps.first(where: { !$0.isEmpty && text.localizedCaseInsensitiveContains($0) }) {
            return app
        }

        let known: [(String, String)] = [
            ("com.tencent.xinWeChat", "微信"),
            ("com.tencent.xinwechat", "微信"),
            ("WeChat", "微信"),
            ("com.tencent.WeWorkMac", "企业微信"),
            ("com.tencent.qq", "QQ"),
            ("com.tencent.tim", "TIM"),
            ("com.larksuite", "飞书"),
            ("com.bytedance.ee.lark", "飞书"),
            ("com.electron.lark", "飞书"),
            ("Lark", "飞书"),
            ("Feishu", "飞书"),
            ("com.hnc.Discord", "Discord"),
            ("com.tinyspeck.slackmacgap", "Slack"),
            ("com.microsoft.teams", "Teams"),
            ("us.zoom.xos", "Zoom"),
            ("ru.keepcoder.Telegram", "Telegram"),
            ("com.apple.iChat", "信息"),
            ("com.apple.MobileSMS", "信息"),
            ("com.apple.mail", "邮件"),
            ("com.apple.reminders", "提醒事项"),
            ("com.apple.iCal", "日历")
        ]

        if let matched = known.first(where: { bundleID, _ in
            sourceText.localizedCaseInsensitiveContains(bundleID) || text.localizedCaseInsensitiveContains(bundleID)
        }) {
            return matched.1
        }

        if let hintedName, !hintedName.isEmpty {
            return humanReadableAppName(hintedName)
        }

        return row.first { key, value in
            !value.isEmpty && (
                key.localizedCaseInsensitiveContains("app")
                    || key.localizedCaseInsensitiveContains("source")
                    || key.localizedCaseInsensitiveContains("bundle")
            )
        }.map { _, value in
            humanReadableAppName(value)
        }
    }

    private static func humanReadableAppName(_ rawValue: String) -> String {
        let trimmed = rawValue.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return "系统通知" }
        if trimmed.contains(".") {
            return trimmed
                .split(separator: ".")
                .last
                .map(String.init)?
                .replacingOccurrences(of: "-", with: " ")
                .replacingOccurrences(of: "_", with: " ")
                ?? trimmed
        }
        return trimmed
    }

    private static func buildAppHints(db: OpaquePointer, tables: [String]) -> [String: AppHint] {
        var hints: [String: AppHint] = [:]

        for table in tables {
            let columns = tableColumns(db, table: table)
            let joinedColumns = columns.joined(separator: " ").lowercased()
            guard joinedColumns.contains("bundle")
                    || joinedColumns.contains("app")
                    || joinedColumns.contains("identifier")
                    || joinedColumns.contains("section")
                    || joinedColumns.contains("source") else {
                continue
            }

            let selectedColumns = columns.map { quoteIdentifier($0) }.joined(separator: ", ")
            let rows = queryRows(db, sql: "select rowid as __topi_rowid, \(selectedColumns) from \(quoteIdentifier(table)) limit 300")
            for row in rows {
                let text = row.values.joined(separator: "\n")
                guard let bundleID = inferredBundleID(from: row, fallbackText: text) else { continue }
                let name = preferredValue(in: row, matching: ["display", "name", "title", "app", "application", "section", "source"])
                    ?? humanReadableAppName(bundleID)
                let hint = AppHint(bundleID: bundleID, name: name)

                for key in row.values where !key.isEmpty {
                    hints[key] = hint
                }
                if let rowID = row["__topi_rowid"] {
                    hints[rowID] = hint
                }
                hints[bundleID] = hint
            }
        }

        return hints
    }

    private static func inferredAppHint(from row: [String: String], appHints: [String: AppHint]) -> AppHint? {
        for value in row.values where !value.isEmpty {
            if let exact = appHints[value] {
                return exact
            }
            if let contained = appHints.first(where: { key, _ in
                key.count > 3 && value.localizedCaseInsensitiveContains(key)
            })?.value {
                return contained
            }
        }
        return nil
    }

    private static func preferredValue(in row: [String: String], matching names: [String]) -> String? {
        let candidates = row
            .filter { key, value in
                names.contains { key.localizedCaseInsensitiveContains($0) }
                    && !value.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
            }
            .map(\.value)
        return candidates
            .map { cleanedNotificationText($0) }
            .first { !$0.isEmpty }
    }

    private static func cleanedNotificationText(_ value: String) -> String {
        let trimmed = value.trimmingCharacters(in: .whitespacesAndNewlines)
        guard trimmed.count <= 240 else {
            return String(trimmed.prefix(240))
        }
        return trimmed
    }

    private static func queryRows(_ db: OpaquePointer, sql: String) -> [[String: String]] {
        var statement: OpaquePointer?
        guard sqlite3_prepare_v2(db, sql, -1, &statement, nil) == SQLITE_OK, let statement else {
            return []
        }
        defer { sqlite3_finalize(statement) }

        var rows: [[String: String]] = []
        while sqlite3_step(statement) == SQLITE_ROW {
            var row: [String: String] = [:]
            for index in 0..<sqlite3_column_count(statement) {
                guard let namePointer = sqlite3_column_name(statement, index) else { continue }
                let name = String(cString: namePointer)
                row[name] = columnValue(statement, index: index)
            }
            rows.append(row)
        }
        return rows
    }

    private static func queryRowsWithBlob(_ db: OpaquePointer, sql: String) -> [[String: DatabaseValue]] {
        var statement: OpaquePointer?
        guard sqlite3_prepare_v2(db, sql, -1, &statement, nil) == SQLITE_OK, let statement else {
            return []
        }
        defer { sqlite3_finalize(statement) }

        var rows: [[String: DatabaseValue]] = []
        while sqlite3_step(statement) == SQLITE_ROW {
            var row: [String: DatabaseValue] = [:]
            for index in 0..<sqlite3_column_count(statement) {
                guard let namePointer = sqlite3_column_name(statement, index) else { continue }
                let name = String(cString: namePointer)
                row[name] = databaseValue(statement, index: index)
            }
            rows.append(row)
        }
        return rows
    }

    private static func databaseValue(_ statement: OpaquePointer, index: Int32) -> DatabaseValue {
        switch sqlite3_column_type(statement, index) {
        case SQLITE_INTEGER:
            return .text(String(sqlite3_column_int64(statement, index)))
        case SQLITE_FLOAT:
            return .text(String(sqlite3_column_double(statement, index)))
        case SQLITE_TEXT:
            guard let text = sqlite3_column_text(statement, index) else { return .text("") }
            return .text(String(cString: text))
        case SQLITE_BLOB:
            let size = sqlite3_column_bytes(statement, index)
            guard size > 0, size < 262_144, let bytes = sqlite3_column_blob(statement, index) else {
                return .data(Data())
            }
            return .data(Data(bytes: bytes, count: Int(size)))
        default:
            return .text("")
        }
    }

    private static func columnValue(_ statement: OpaquePointer, index: Int32) -> String {
        switch sqlite3_column_type(statement, index) {
        case SQLITE_INTEGER:
            return String(sqlite3_column_int64(statement, index))
        case SQLITE_FLOAT:
            return String(sqlite3_column_double(statement, index))
        case SQLITE_TEXT:
            guard let text = sqlite3_column_text(statement, index) else { return "" }
            return String(cString: text)
        case SQLITE_BLOB:
            let size = sqlite3_column_bytes(statement, index)
            guard size > 0, size < 262_144, let bytes = sqlite3_column_blob(statement, index) else { return "" }
            let data = Data(bytes: bytes, count: Int(size))
            if let string = String(data: data, encoding: .utf8), !string.isEmpty {
                return string
            }
            return printableStrings(from: data).joined(separator: "\n")
        default:
            return ""
        }
    }

    private static func quoteIdentifier(_ identifier: String) -> String {
        "\"\(identifier.replacingOccurrences(of: "\"", with: "\"\""))\""
    }

    private static func stableFingerprint(_ parts: [String]) -> String {
        var hash: UInt64 = 1469598103934665603
        for byte in parts.joined(separator: "\u{1F}").utf8 {
            hash ^= UInt64(byte)
            hash &*= 1099511628211
        }
        return String(hash, radix: 16)
    }

    private static func printableStrings(from data: Data) -> [String] {
        var results = Set<String>()

        if let utf16 = String(data: data, encoding: .utf16LittleEndian) {
            extractLikelyTokens(from: utf16, into: &results)
        }
        if let plist = try? PropertyListSerialization.propertyList(from: data, options: [], format: nil) {
            collectPropertyListStrings(plist, into: &results)
        }

        var current = [UInt8]()
        for byte in data {
            if byte >= 32 && byte <= 126 {
                current.append(byte)
            } else {
                flushASCII(&current, into: &results)
            }
        }
        flushASCII(&current, into: &results)

        return results
            .filter { token in
                token.count >= 3
                    && token.count <= 240
                    && !token.allSatisfy(\.isNumber)
            }
            .sorted()
    }

    private static func flushASCII(_ bytes: inout [UInt8], into results: inout Set<String>) {
        defer { bytes.removeAll(keepingCapacity: true) }
        guard bytes.count >= 3,
              let string = String(bytes: bytes, encoding: .utf8) else {
            return
        }
        extractLikelyTokens(from: string, into: &results)
    }

    private static func extractLikelyTokens(from string: String, into results: inout Set<String>) {
        let separators = CharacterSet(charactersIn: "\0\n\r\t")
        for token in string.components(separatedBy: separators) {
            let trimmed = token.trimmingCharacters(in: .whitespacesAndNewlines)
            guard trimmed.count >= 3, trimmed.count <= 240 else { continue }
            results.insert(trimmed)
        }
    }

    private static func collectPropertyListStrings(_ value: Any, into results: inout Set<String>) {
        if let string = value as? String {
            extractLikelyTokens(from: string, into: &results)
        } else if let dictionary = value as? [AnyHashable: Any] {
            for (key, value) in dictionary {
                extractLikelyTokens(from: String(describing: key), into: &results)
                collectPropertyListStrings(value, into: &results)
            }
        } else if let array = value as? [Any] {
            for value in array {
                collectPropertyListStrings(value, into: &results)
            }
        }
    }
}

private struct AppHint {
    let bundleID: String
    let name: String
}

private struct DatabaseOpenError: Error {
    let message: String
}

private enum DatabaseValue {
    case text(String)
    case data(Data)

    var stringValue: String? {
        switch self {
        case .text(let string):
            return string
        case .data(let data):
            return String(data: data, encoding: .utf8)
        }
    }

    var dataValue: Data? {
        switch self {
        case .text(let string):
            return string.data(using: .utf8)
        case .data(let data):
            return data
        }
    }
}
