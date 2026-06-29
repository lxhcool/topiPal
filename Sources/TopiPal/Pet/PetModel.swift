import Foundation

enum PetPhase {
    case compact
    case expanded
}

struct PetNotificationBubble: Equatable {
    let appName: String
    let bundleID: String?
    let message: String?
}

@MainActor
final class PetModel: ObservableObject {
    @Published private(set) var phase: PetPhase = .compact
    @Published var selectedCharacterID: String {
        didSet {
            UserDefaults.standard.set(selectedCharacterID, forKey: Self.characterKey)
        }
    }
    @Published private(set) var activeAction: String?
    @Published private(set) var bubbleMessage: String?
    @Published private(set) var notificationBubble: PetNotificationBubble?
    @Published private(set) var importErrorMessage: String?
    @Published private(set) var notificationDiagnosticReport: String?
    @Published private(set) var lastNotificationMonitorEvent: String?
    @Published private(set) var smartTestResultMessage: String?
    @Published private(set) var activityContext: ActivityContext?
    @Published private var clickActionMappings: [String: [String: String]]
    @Published private(set) var runtimeState = PetRuntimeState(
        mood: .calm,
        activity: .unknown,
        shouldSpeak: true,
        speakInterval: 18...35,
        reason: "initial"
    )

    let smartSettings = SmartCompanionSettings()

    @Published private(set) var characters = PetAssetLibrary.characters
    private var activityTask: Task<Void, Never>?
    private var ambientTask: Task<Void, Never>?
    private var autoActionTask: Task<Void, Never>?
    private var actionTask: Task<Void, Never>?
    private var bubbleTask: Task<Void, Never>?
    private var smartBubbleTask: Task<Void, Never>?
    private var notificationBubbleTask: Task<Void, Never>?
    private var bubbleRevision = 0
    private var interactionSequence = 0
    private var tapCount = 0
    private var lastTapDate: Date?
    private var pointerFollowStartDate: Date?
    private var lastPointerPoint: CGPoint?
    private var pointerTravelDistance: CGFloat = 0
    private var lastPointerFollowHintDate: Date?
    private var lastNotificationAppName: String?
    private var lastRelationshipTapCreditDate: Date?
    private var lastRelationshipNotificationCreditDateByApp: [String: Date] = [:]
    private var lastAmbientBubbleDate: Date?
    private var lastAutoActionBubbleDate: Date?
    private var deliveredWeatherMoments: Set<String> = []
    private var deliveredLifeMoments: Set<String> = []
    private var relationshipMemory = PetRelationshipMemory.load()
    private var messageMemory = PetMemory(
        mood: .calm,
        energy: 70,
        tapStreak: 0,
        lastArea: nil,
        lastAppName: nil,
        notificationStreak: 0,
        recentMessages: [],
        relationship: PetRelationshipSnapshot(
            level: .new,
            totalInteractions: 0,
            activeDays: 1,
            favoriteArea: nil,
            favoriteAppName: nil,
            daysKnown: 1
        )
    )

    private static let characterKey = "Pet.SelectedCharacterID"
    private static let clickActionMappingsKey = "Pet.ClickActionMappings.v1"
    private static let deliveredWeatherMomentsKey = "Pet.DeliveredWeatherMoments.v1"
    private static let deliveredLifeMomentsKey = "Pet.DeliveredLifeMoments.v1"

    init() {
        clickActionMappings = Self.loadClickActionMappings()

        let savedID = UserDefaults.standard.string(forKey: Self.characterKey)
        if let savedID, PetAssetLibrary.characters.contains(where: { $0.id == savedID }) {
            selectedCharacterID = savedID
        } else {
            selectedCharacterID = PetAssetLibrary.characters.first?.id ?? ""
        }
        activeAction = characters.first { $0.id == selectedCharacterID }?.defaultAction

        deliveredWeatherMoments = Set(UserDefaults.standard.stringArray(forKey: Self.deliveredWeatherMomentsKey) ?? [])
        deliveredLifeMoments = Set(UserDefaults.standard.stringArray(forKey: Self.deliveredLifeMomentsKey) ?? [])
        relationshipMemory.markActive()
        relationshipMemory.save()
        syncRelationshipSnapshot()
    }

    func startActivityAwareness() {
        activityTask?.cancel()
        activityTask = Task { [weak self] in
            while !Task.isCancelled {
                let readTitle = await MainActor.run { self?.smartSettings.canReadWindowTitles ?? false }
                let isEnabled = await MainActor.run { self?.smartSettings.isActivityAwarenessEnabled ?? false }
                let context = isEnabled
                    ? ActivityClassifier.classify(ActivityContextMonitor.snapshot(readWindowTitle: readTitle))
                    : nil

                await MainActor.run {
                    guard let self else { return }
                    self.activityContext = context
                    self.runtimeState = PetStateEngine.runtimeState(activity: context, memory: self.messageMemory)
                    self.messageMemory.mood = self.runtimeState.mood
                }

                try? await Task.sleep(nanoseconds: 3_000_000_000)
            }
        }
    }

    var selectedCharacter: PetCharacter {
        characters.first { $0.id == selectedCharacterID } ?? characters[0]
    }

    func toggle() {
        switch phase {
        case .compact:
            expand()
        case .expanded:
            collapse()
        }
    }

    func expand() {
        guard phase != .expanded else { return }
        phase = .expanded
    }

    func collapse() {
        guard phase != .compact else { return }
        phase = .compact
    }

    func selectCharacter(_ character: PetCharacter) {
        selectedCharacterID = character.id
        activeAction = character.defaultAction
        showBubble("\(character.name)")
    }

    func importCharacter(kind: PetRendererKind, from url: URL) {
        do {
            let character = try PetAssetLibrary.importCharacter(kind: kind, from: url)
            characters = PetAssetLibrary.characters
            selectCharacter(character)
            importErrorMessage = nil
            showBubble("已导入：\(character.name)")
        } catch {
            importErrorMessage = error.localizedDescription
            showBubble(error.localizedDescription)
        }
    }

    func deleteCharacter(_ character: PetCharacter) {
        guard character.isCustom else { return }
        PetAssetLibrary.deleteCustomCharacter(id: character.id)
        characters = PetAssetLibrary.characters

        if selectedCharacterID == character.id {
            let nextCharacter = characters.first
            selectedCharacterID = nextCharacter?.id ?? ""
            activeAction = nextCharacter?.defaultAction
        }

        importErrorMessage = nil
        showBubble("已删除：\(character.name)")
    }

    func clickActionSelection(for character: PetCharacter, area: PetHitArea) -> String? {
        guard let action = clickActionMappings[character.id]?[area.rawValue],
              character.actions.contains(action) else {
            return nil
        }
        return action
    }

    func resolvedClickAction(for character: PetCharacter, area: PetHitArea) -> String? {
        clickActionSelection(for: character, area: area) ?? character.action(for: area)
    }

    func setClickAction(_ action: String?, for character: PetCharacter, area: PetHitArea) {
        var mappings = clickActionMappings
        var characterMappings = mappings[character.id] ?? [:]

        if let action, character.actions.contains(action) {
            characterMappings[area.rawValue] = action
        } else {
            characterMappings.removeValue(forKey: area.rawValue)
        }

        if characterMappings.isEmpty {
            mappings.removeValue(forKey: character.id)
        } else {
            mappings[character.id] = characterMappings
        }

        clickActionMappings = mappings
        Self.saveClickActionMappings(mappings)
    }

    func testClickAction(for character: PetCharacter, area: PetHitArea) {
        if selectedCharacterID != character.id {
            selectedCharacterID = character.id
            activeAction = character.defaultAction
        }
        guard let action = resolvedClickAction(for: character, area: area) else {
            showBubble("这个区域还没有可用动作")
            return
        }
        playAction(action)
    }

    func handleTap(in area: PetHitArea) {
        let now = Date()
        if let lastTapDate, now.timeIntervalSince(lastTapDate) <= 3 {
            tapCount += 1
        } else {
            tapCount = 1
        }
        lastTapDate = now
        messageMemory.tapStreak = tapCount
        messageMemory.lastArea = area
        messageMemory.energy = min(100, messageMemory.energy + 4)
        _ = recordRelationshipTapIfNeeded(area: area, at: now)
        messageMemory.mood = PetMessageComposer.nextMood(
            afterInteraction: area,
            tapStreak: tapCount,
            hour: Calendar.current.component(.hour, from: now)
        )

        interactionSequence += 1
        smartBubbleTask?.cancel()

        if let action = resolvedClickAction(for: selectedCharacter, area: area) {
            playAction(action)
        }
    }

    func handlePointerMove(at point: CGPoint, in size: CGSize) {
        let now = Date()
        if let lastPointerPoint {
            pointerTravelDistance += hypot(point.x - lastPointerPoint.x, point.y - lastPointerPoint.y)
        } else {
            pointerFollowStartDate = now
            pointerTravelDistance = 0
        }
        lastPointerPoint = point

        guard let startDate = pointerFollowStartDate else { return }
        let hasMovedEnough = pointerTravelDistance > min(size.width, size.height) * 1.8
        let hasFollowedLongEnough = now.timeIntervalSince(startDate) >= 2.2
        let canSpeak = lastPointerFollowHintDate.map { now.timeIntervalSince($0) >= 12 } ?? true
        guard hasMovedEnough, hasFollowedLongEnough, canSpeak else { return }

        lastPointerFollowHintDate = now
        pointerFollowStartDate = now
        pointerTravelDistance = 0
        let leveledUp = relationshipMemory.recordPointerPlay(at: now)
        syncRelationshipSnapshot()
        if let leveledUp {
            showBubble(PetMessageComposer.relationshipLevelUpMessage(level: leveledUp, memory: messageMemory))
        } else {
            showBubble(PetMessageComposer.pointerFollowMessage(memory: messageMemory))
        }

        if let action = resolvedClickAction(for: selectedCharacter, area: .head)
            ?? resolvedClickAction(for: selectedCharacter, area: .right) {
            playAction(action)
        }
    }

    func handlePointerExit() {
        pointerFollowStartDate = nil
        lastPointerPoint = nil
        pointerTravelDistance = 0
    }

    func startAutoActions() {
        autoActionTask?.cancel()
        autoActionTask = Task { [weak self] in
            while !Task.isCancelled {
                let delayRange = await MainActor.run { self?.autoActionDelayRange ?? 40...90 }
                let delay = UInt64(Int.random(in: delayRange)) * 1_000_000_000
                try? await Task.sleep(nanoseconds: delay)
                await MainActor.run {
                    guard let self else { return }
                    guard self.phase == .compact, self.isAtRest else { return }
                    self.performRandomAutoAction()
                }
            }
        }
    }

    func startAmbientMessages() {
        ambientTask?.cancel()
        ambientTask = Task { [weak self] in
            while !Task.isCancelled {
                let interval = await MainActor.run { self?.ambientDelayRange ?? 120...240 }
                let delay = UInt64(Int.random(in: interval)) * 1_000_000_000
                try? await Task.sleep(nanoseconds: delay)
                guard !Task.isCancelled else { return }

                let state = await MainActor.run { () -> (PetPhase, SmartCompanionConfiguration, String?, Bool, ActivityContext?, PetMemory, LifeMoment?, WeatherMoment?)? in
                    guard let self else { return nil }
                    self.messageMemory.energy = max(0, self.messageMemory.energy - 2)
                    return (
                        self.phase,
                        self.smartSettings.snapshot(),
                        PetMessageComposer.ambientMessage(memory: self.messageMemory, activity: self.activityContext),
                        self.runtimeState.shouldSpeak,
                        self.activityContext,
                        self.messageMemory,
                        self.nextLifeMomentIfNeeded(),
                        self.nextWeatherMomentIfNeeded()
                    )
                }
                guard let state, state.0 == .compact, state.3 else { continue }

                if let lifeMoment = state.6 {
                    let localMessage = PetMessageComposer.lifeMomentMessage(
                        moment: lifeMoment,
                        memory: state.5,
                        activity: state.4
                    )
                    await MainActor.run {
                        guard let self, self.phase == .compact else { return }
                        guard self.canShowAmbientBubble(important: true) else { return }
                        self.markLifeMomentDelivered(lifeMoment)
                        let revision = self.showBubble(localMessage, cancelsSmartRewrite: true)
                        let configuration = state.1
                        guard configuration.canRequestModel else { return }
                        self.smartBubbleTask = Task { [weak self] in
                            let smartMessage = await SmartMessageService.rewriteMessage(
                                draft: localMessage,
                                event: .lifeMoment(lifeMoment),
                                activity: state.4,
                                memory: state.5,
                                configuration: configuration
                            )
                            await MainActor.run {
                                guard let self, let smartMessage else { return }
                                self.updateBubble(smartMessage, matching: revision)
                            }
                        }
                    }
                    continue
                }

                if let weatherMoment = state.7 {
                    let weatherMessage = await SmartMessageService.makeWeatherMomentMessage(
                        moment: weatherMoment,
                        activity: state.4,
                        memory: state.5,
                        configuration: state.1
                    )
                    await MainActor.run {
                        guard let self, self.phase == .compact, let weatherMessage else { return }
                        guard self.canShowAmbientBubble(important: true) else { return }
                        self.markWeatherMomentDelivered(weatherMoment)
                        self.showBubble(weatherMessage, cancelsSmartRewrite: true)
                    }
                    continue
                }

                await MainActor.run {
                    guard let self, self.phase == .compact else { return }
                    guard self.canShowAmbientBubble(important: false) else { return }
                    if let message = state.2 {
                        let revision = self.showBubble(message, cancelsSmartRewrite: true)
                        let configuration = state.1
                        guard configuration.canRequestModel else { return }
                        self.smartBubbleTask = Task { [weak self] in
                            let smartMessage = await SmartMessageService.rewriteMessage(
                                draft: message,
                                event: .ambient,
                                activity: state.4,
                                memory: state.5,
                                configuration: configuration
                            )
                            await MainActor.run {
                                guard let self, let smartMessage else { return }
                                self.updateBubble(smartMessage, matching: revision)
                            }
                        }
                    }
                }
            }
        }
    }

    private func playAction(_ action: String, duration: UInt64 = 1_200_000_000) {
        actionTask?.cancel()
        activeAction = action

        actionTask = Task { [weak self] in
            try? await Task.sleep(nanoseconds: duration)
            guard !Task.isCancelled else { return }
            await MainActor.run {
                self?.activeAction = self?.selectedCharacter.defaultAction
            }
        }
    }

    private func playNotificationAction(for appName: String) {
        let preferredAreas = notificationActionAreas(for: appName)
        guard let action = preferredAreas.compactMap({ resolvedClickAction(for: selectedCharacter, area: $0) }).first else {
            return
        }
        playAction(action, duration: 1_850_000_000)
    }

    private func notificationActionAreas(for appName: String) -> [PetHitArea] {
        if appName.localizedCaseInsensitiveContains("微信")
            || appName.localizedCaseInsensitiveContains("WeChat")
            || appName.localizedCaseInsensitiveContains("飞书")
            || appName.localizedCaseInsensitiveContains("Lark")
            || appName.localizedCaseInsensitiveContains("QQ")
            || appName.localizedCaseInsensitiveContains("Telegram")
            || appName.localizedCaseInsensitiveContains("Slack")
            || appName.localizedCaseInsensitiveContains("Teams") {
            return [.head, .right, .body, .left, .feet]
        }

        if appName.localizedCaseInsensitiveContains("邮件")
            || appName.localizedCaseInsensitiveContains("Mail")
            || appName.localizedCaseInsensitiveContains("日历")
            || appName.localizedCaseInsensitiveContains("提醒") {
            return [.right, .body, .head, .left, .feet]
        }

        return [.body, .head, .right, .left, .feet]
    }

    private var isAtRest: Bool {
        activeAction == nil || activeAction == selectedCharacter.defaultAction
    }

    private var ambientDelayRange: ClosedRange<Int> {
        let base: ClosedRange<Int>
        switch smartSettings.talkativeness {
        case .quiet:
            base = 300...600
        case .standard:
            base = 120...240
        case .active:
            base = 55...120
        }

        let stateRange = runtimeState.speakInterval
        let lower = max(base.lowerBound, stateRange.lowerBound)
        let upper = max(lower, max(base.upperBound, stateRange.upperBound))
        return lower...upper
    }

    private var autoActionDelayRange: ClosedRange<Int> {
        switch smartSettings.talkativeness {
        case .quiet:
            120...240
        case .standard:
            55...110
        case .active:
            25...55
        }
    }

    private func performRandomAutoAction() {
        let candidates = autoActionCandidates()
        guard let candidate = candidates.randomElement() else { return }
        messageMemory.energy = max(0, messageMemory.energy - 3)
        if shouldSpeakForAutoAction() {
            let localMessage = PetMessageComposer.autoActionMessage(area: candidate.area, memory: messageMemory)
            let revision = showBubble(localMessage, cancelsSmartRewrite: true)
            lastAutoActionBubbleDate = Date()
            let configuration = smartSettings.snapshot()
            if configuration.canRequestModel {
                let activityContext = activityContext
                let memory = messageMemory
                smartBubbleTask = Task { [weak self] in
                    let message = await SmartMessageService.rewriteMessage(
                        draft: localMessage,
                        event: .autoAction(candidate.area),
                        activity: activityContext,
                        memory: memory,
                        configuration: configuration
                    )
                    await MainActor.run {
                        guard let self, let message else { return }
                        self.updateBubble(message, matching: revision)
                    }
                }
            }
        }
        playAction(candidate.action)
    }

    private func canShowAmbientBubble(important: Bool) -> Bool {
        guard bubbleMessage == nil else { return false }
        if smartSettings.talkativeness == .quiet && !important {
            return false
        }

        let now = Date()
        let minimumInterval: TimeInterval = important ? 60 : {
            switch smartSettings.talkativeness {
            case .quiet: return 600
            case .standard: return 180
            case .active: return 75
            }
        }()

        if let lastAmbientBubbleDate, now.timeIntervalSince(lastAmbientBubbleDate) < minimumInterval {
            return false
        }

        lastAmbientBubbleDate = now
        return true
    }

    private func shouldSpeakForAutoAction() -> Bool {
        guard bubbleMessage == nil else { return false }
        guard smartSettings.talkativeness == .active else { return false }
        let now = Date()
        if let lastAutoActionBubbleDate, now.timeIntervalSince(lastAutoActionBubbleDate) < 180 {
            return false
        }
        return Int.random(in: 0..<100) < 25
    }

    private func autoActionCandidates() -> [(area: PetHitArea, action: String)] {
        let preferredAreas: [PetHitArea] = [.head, .body, .right, .left, .feet]
        var seenActions = Set<String>()
        var candidates: [(area: PetHitArea, action: String)] = []

        for area in preferredAreas {
            guard let action = resolvedClickAction(for: selectedCharacter, area: area),
                  !isIdleAction(action),
                  !seenActions.contains(action) else { continue }
            seenActions.insert(action)
            candidates.append((area, action))
        }

        if !candidates.isEmpty {
            return candidates
        }

        return selectedCharacter.actions
            .filter { !isIdleAction($0) }
            .map { (area: .body, action: $0) }
    }

    private func isIdleAction(_ action: String) -> Bool {
        action.localizedCaseInsensitiveContains("idle")
    }

    private static func loadClickActionMappings() -> [String: [String: String]] {
        guard let data = UserDefaults.standard.data(forKey: clickActionMappingsKey),
              let mappings = try? JSONDecoder().decode([String: [String: String]].self, from: data) else {
            return [:]
        }
        return mappings
    }

    private static func saveClickActionMappings(_ mappings: [String: [String: String]]) {
        guard let data = try? JSONEncoder().encode(mappings) else { return }
        UserDefaults.standard.set(data, forKey: clickActionMappingsKey)
    }

    private func nextLifeMomentIfNeeded(now: Date = Date()) -> LifeMoment? {
        let hour = Calendar.current.component(.hour, from: now)
        let moment: LifeMoment?
        switch hour {
        case 8...10:
            moment = .morningStart
        case 11...13:
            moment = .lunch
        case 15...16:
            moment = .afternoonReset
        case 18:
            moment = .offWork
        case 19...20:
            moment = .dinner
        case 22...23:
            moment = .lateNight
        default:
            moment = nil
        }

        guard let moment else { return nil }
        let key = lifeMomentKey(moment: moment, date: now)
        return deliveredLifeMoments.contains(key) ? nil : moment
    }

    private func markLifeMomentDelivered(_ moment: LifeMoment, now: Date = Date()) {
        deliveredLifeMoments.insert(lifeMomentKey(moment: moment, date: now))
        if deliveredLifeMoments.count > 18 {
            deliveredLifeMoments = Set(deliveredLifeMoments.sorted().suffix(18))
        }
        UserDefaults.standard.set(Array(deliveredLifeMoments), forKey: Self.deliveredLifeMomentsKey)
    }

    private func lifeMomentKey(moment: LifeMoment, date: Date) -> String {
        let components = Calendar.current.dateComponents([.year, .month, .day], from: date)
        return "\(components.year ?? 0)-\(components.month ?? 0)-\(components.day ?? 0)-\(moment.rawValue)"
    }

    private func nextWeatherMomentIfNeeded(now: Date = Date()) -> WeatherMoment? {
        let configuration = smartSettings.snapshot()
        guard configuration.canRequestModel,
              configuration.enabledTopics.contains(.weather),
              !configuration.weatherCity.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
            return nil
        }

        let hour = Calendar.current.component(.hour, from: now)
        let moment: WeatherMoment?
        switch hour {
        case 7...10:
            moment = .morning
        case 11...13:
            moment = .noon
        case 17...20:
            moment = .evening
        default:
            moment = nil
        }

        guard let moment else { return nil }
        let key = weatherMomentKey(moment: moment, date: now)
        return deliveredWeatherMoments.contains(key) ? nil : moment
    }

    private func markWeatherMomentDelivered(_ moment: WeatherMoment, now: Date = Date()) {
        deliveredWeatherMoments.insert(weatherMomentKey(moment: moment, date: now))
        if deliveredWeatherMoments.count > 12 {
            deliveredWeatherMoments = Set(deliveredWeatherMoments.sorted().suffix(12))
        }
        UserDefaults.standard.set(Array(deliveredWeatherMoments), forKey: Self.deliveredWeatherMomentsKey)
    }

    private func weatherMomentKey(moment: WeatherMoment, date: Date) -> String {
        let components = Calendar.current.dateComponents([.year, .month, .day], from: date)
        return "\(components.year ?? 0)-\(components.month ?? 0)-\(components.day ?? 0)-\(moment.rawValue)"
    }

    func showWelcomeHint() {
        showBubble("点点不同位置，我会换反应。")
    }

    func previewResourceImport(at url: URL) {
        showBubble("已选择资源：\(url.lastPathComponent)")
    }

    func requestSmartBubbleNow() {
        let configuration = smartSettings.snapshot()
        smartTestResultMessage = "正在测试模型..."
        Task { [weak self] in
            let result = await SmartMessageService.testMessage(configuration: configuration)
            await MainActor.run {
                guard let self else { return }
                switch result {
                case .success(let message):
                    self.smartTestResultMessage = message
                    self.showBubble(message)
                case .failure(let error):
                    self.smartTestResultMessage = error
                    self.showBubble(error)
                }
            }
        }
    }

    func receiveAppNotification(appName: String) {
        receiveAppNotification(appName: appName, reason: nil)
    }

    func receiveAppNotification(appName: String, reason: String?) {
        receiveAppNotification(appName: appName, bundleID: nil, message: nil, reason: reason)
    }

    func receiveAppNotification(appName: String, bundleID: String?, message: String?, reason: String?) {
        if let reason {
            lastNotificationMonitorEvent = "\(appName)：\(reason)"
        } else {
            lastNotificationMonitorEvent = "\(appName)：手动测试"
        }
        messageMemory.lastAppName = appName
        if let lastNotificationAppName,
           lastNotificationAppName.localizedCaseInsensitiveContains(appName)
            || appName.localizedCaseInsensitiveContains(lastNotificationAppName) {
            messageMemory.notificationStreak += 1
        } else {
            messageMemory.notificationStreak = 1
        }
        lastNotificationAppName = appName
        let leveledUp = recordRelationshipNotificationIfNeeded(appName: appName)
        messageMemory.mood = PetMessageComposer.nextMoodAfterNotification(appName: appName)
        messageMemory.energy = max(0, messageMemory.energy - 1)
        let trimmedMessage = message?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
        if trimmedMessage.isEmpty {
            let relationshipMessage = leveledUp.map { PetMessageComposer.relationshipLevelUpMessage(level: $0, memory: messageMemory) }
            showNotificationBubble(appName: appName, bundleID: bundleID, message: relationshipMessage ?? PetMessageComposer.notificationMessage(appName: appName, memory: messageMemory))
        } else {
            showNotificationBubble(appName: appName, bundleID: bundleID, message: trimmedMessage)
        }
        playNotificationAction(for: appName)
    }

    func receiveGenericNotification(reason: String?) {
        lastNotificationMonitorEvent = reason ?? "系统通知：检测到新通知"
        showNotificationBubble(appName: "系统通知", bundleID: nil, message: "有新消息")
        playNotificationAction(for: "系统通知")
    }

    func showSystemMessage(_ message: String) {
        showBubble(message)
    }

    func runNotificationDiagnostics() {
        let report = AppNotificationMonitor(model: self, settings: smartSettings).diagnosticReport()
        notificationDiagnosticReport = report
        showBubble("监听诊断已刷新")
    }

    func showNotificationBubble(appName: String, bundleID: String?, message: String?) {
        guard !appName.isEmpty else { return }
        notificationBubbleTask?.cancel()
        if let message, !message.isEmpty {
            rememberBubbleMessage(message)
        }
        notificationBubble = PetNotificationBubble(appName: appName, bundleID: bundleID, message: message)

        notificationBubbleTask = Task { [weak self] in
            try? await Task.sleep(nanoseconds: 4_000_000_000)
            guard !Task.isCancelled else { return }
            await MainActor.run {
                self?.notificationBubble = nil
            }
        }
    }

    @discardableResult
    private func showBubble(_ message: String, cancelsSmartRewrite: Bool = true) -> Int {
        guard !message.isEmpty else { return bubbleRevision }
        bubbleTask?.cancel()
        if cancelsSmartRewrite {
            smartBubbleTask?.cancel()
            smartBubbleTask = nil
        }
        bubbleRevision += 1
        let revision = bubbleRevision
        bubbleMessage = message
        rememberBubbleMessage(message)

        bubbleTask = Task { [weak self] in
            try? await Task.sleep(nanoseconds: 5_000_000_000)
            guard !Task.isCancelled else { return }
            await MainActor.run {
                guard let self, self.bubbleRevision == revision else { return }
                self.bubbleMessage = nil
            }
        }
        return revision
    }

    private func updateBubble(_ message: String, matching revision: Int) {
        guard !message.isEmpty, bubbleRevision == revision, bubbleMessage != nil else { return }
        bubbleMessage = message
        rememberBubbleMessage(message)
    }

    private func rememberBubbleMessage(_ message: String) {
        let trimmed = message.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return }
        messageMemory.recentMessages.removeAll { $0 == trimmed }
        messageMemory.recentMessages.insert(trimmed, at: 0)
        if messageMemory.recentMessages.count > 12 {
            messageMemory.recentMessages = Array(messageMemory.recentMessages.prefix(12))
        }
    }

    private func syncRelationshipSnapshot() {
        messageMemory.relationship = relationshipMemory.snapshot
    }

    private func recordRelationshipTapIfNeeded(area: PetHitArea, at date: Date) -> PetRelationshipLevel? {
        let canCredit = lastRelationshipTapCreditDate.map { date.timeIntervalSince($0) >= 8 } ?? true
        guard canCredit else { return nil }
        lastRelationshipTapCreditDate = date
        let leveledUp = relationshipMemory.recordTap(area: area, at: date)
        syncRelationshipSnapshot()
        return leveledUp
    }

    private func recordRelationshipNotificationIfNeeded(appName: String, at date: Date = Date()) -> PetRelationshipLevel? {
        let key = normalizedNotificationAppName(appName)
        let canCredit = lastRelationshipNotificationCreditDateByApp[key].map { date.timeIntervalSince($0) >= 30 } ?? true
        guard canCredit else {
            syncRelationshipSnapshot()
            return nil
        }
        lastRelationshipNotificationCreditDateByApp[key] = date
        let leveledUp = relationshipMemory.recordNotification(appName: appName, at: date)
        syncRelationshipSnapshot()
        return leveledUp
    }

    private func normalizedNotificationAppName(_ appName: String) -> String {
        if appName.localizedCaseInsensitiveContains("微信") || appName.localizedCaseInsensitiveContains("WeChat") {
            return "微信"
        }
        if appName.localizedCaseInsensitiveContains("飞书") || appName.localizedCaseInsensitiveContains("Lark") {
            return "飞书"
        }
        return appName
    }
}
