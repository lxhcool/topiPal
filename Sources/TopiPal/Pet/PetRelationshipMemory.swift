import Foundation

enum PetRelationshipLevel: String, Codable, Sendable {
    case new
    case familiar
    case close
    case trusted

    var title: String {
        switch self {
        case .new:
            return "刚认识"
        case .familiar:
            return "熟悉"
        case .close:
            return "亲近"
        case .trusted:
            return "默契"
        }
    }

    static func level(interactions: Int, activeDays: Int) -> PetRelationshipLevel {
        if interactions >= 220 || activeDays >= 21 { return .trusted }
        if interactions >= 80 || activeDays >= 10 { return .close }
        if interactions >= 20 || activeDays >= 3 { return .familiar }
        return .new
    }
}

struct PetRelationshipSnapshot: Codable, Sendable, Equatable {
    var level: PetRelationshipLevel
    var totalInteractions: Int
    var activeDays: Int
    var favoriteArea: PetHitArea?
    var favoriteAppName: String?
    var daysKnown: Int
}

struct PetRelationshipMemory: Codable, Sendable {
    private(set) var firstSeenAt: Date
    private(set) var lastSeenAt: Date?
    private(set) var totalInteractions: Int
    private(set) var tapInteractions: Int
    private(set) var pointerPlayInteractions: Int
    private(set) var notificationInteractions: Int
    private(set) var activeDayKeys: Set<String>
    private(set) var areaCounts: [String: Int]
    private(set) var appCounts: [String: Int]

    private static let storageKey = "Pet.RelationshipMemory.v1"

    init(
        firstSeenAt: Date = Date(),
        lastSeenAt: Date? = nil,
        totalInteractions: Int = 0,
        tapInteractions: Int = 0,
        pointerPlayInteractions: Int = 0,
        notificationInteractions: Int = 0,
        activeDayKeys: Set<String> = [],
        areaCounts: [String: Int] = [:],
        appCounts: [String: Int] = [:]
    ) {
        self.firstSeenAt = firstSeenAt
        self.lastSeenAt = lastSeenAt
        self.totalInteractions = totalInteractions
        self.tapInteractions = tapInteractions
        self.pointerPlayInteractions = pointerPlayInteractions
        self.notificationInteractions = notificationInteractions
        self.activeDayKeys = activeDayKeys
        self.areaCounts = areaCounts
        self.appCounts = appCounts
    }

    static func load() -> PetRelationshipMemory {
        guard let data = UserDefaults.standard.data(forKey: storageKey),
              let memory = try? JSONDecoder().decode(PetRelationshipMemory.self, from: data) else {
            var memory = PetRelationshipMemory()
            memory.markActive()
            memory.save()
            return memory
        }
        return memory
    }

    func save() {
        guard let data = try? JSONEncoder().encode(self) else { return }
        UserDefaults.standard.set(data, forKey: Self.storageKey)
    }

    var snapshot: PetRelationshipSnapshot {
        PetRelationshipSnapshot(
            level: currentLevel,
            totalInteractions: totalInteractions,
            activeDays: activeDayKeys.count,
            favoriteArea: favoriteArea,
            favoriteAppName: favoriteAppName,
            daysKnown: daysKnown
        )
    }

    var currentLevel: PetRelationshipLevel {
        PetRelationshipLevel.level(interactions: totalInteractions, activeDays: activeDayKeys.count)
    }

    mutating func recordTap(area: PetHitArea, at date: Date = Date()) -> PetRelationshipLevel? {
        let previous = currentLevel
        markActive(at: date)
        totalInteractions += 1
        tapInteractions += 1
        areaCounts[area.rawValue, default: 0] += 1
        lastSeenAt = date
        save()
        return leveledUp(from: previous)
    }

    mutating func recordPointerPlay(at date: Date = Date()) -> PetRelationshipLevel? {
        let previous = currentLevel
        markActive(at: date)
        totalInteractions += 1
        pointerPlayInteractions += 1
        lastSeenAt = date
        save()
        return leveledUp(from: previous)
    }

    mutating func recordNotification(appName: String, at date: Date = Date()) -> PetRelationshipLevel? {
        let previous = currentLevel
        markActive(at: date)
        totalInteractions += 1
        notificationInteractions += 1
        appCounts[normalizedAppName(appName), default: 0] += 1
        lastSeenAt = date
        save()
        return leveledUp(from: previous)
    }

    mutating func markActive(at date: Date = Date()) {
        activeDayKeys.insert(dayKey(for: date))
        lastSeenAt = date
    }

    private func leveledUp(from previous: PetRelationshipLevel) -> PetRelationshipLevel? {
        let current = currentLevel
        return current != previous ? current : nil
    }

    private var favoriteArea: PetHitArea? {
        guard let rawValue = areaCounts.max(by: { $0.value < $1.value })?.key else { return nil }
        return PetHitArea(rawValue: rawValue)
    }

    private var favoriteAppName: String? {
        appCounts.max(by: { $0.value < $1.value })?.key
    }

    private var daysKnown: Int {
        let start = Calendar.current.startOfDay(for: firstSeenAt)
        let end = Calendar.current.startOfDay(for: Date())
        return max(1, (Calendar.current.dateComponents([.day], from: start, to: end).day ?? 0) + 1)
    }

    private func normalizedAppName(_ appName: String) -> String {
        if appName.localizedCaseInsensitiveContains("微信") || appName.localizedCaseInsensitiveContains("WeChat") {
            return "微信"
        }
        if appName.localizedCaseInsensitiveContains("飞书") || appName.localizedCaseInsensitiveContains("Lark") {
            return "飞书"
        }
        return appName
    }

    private func dayKey(for date: Date) -> String {
        let components = Calendar.current.dateComponents([.year, .month, .day], from: date)
        return "\(components.year ?? 0)-\(components.month ?? 0)-\(components.day ?? 0)"
    }
}
