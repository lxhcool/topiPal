import Foundation

@MainActor
final class SmartCompanionSettings: ObservableObject {
    @Published var isEnabled: Bool {
        didSet { UserDefaults.standard.set(isEnabled, forKey: Keys.isEnabled) }
    }

    @Published var provider: SmartProvider {
        didSet { UserDefaults.standard.set(provider.rawValue, forKey: Keys.provider) }
    }

    @Published var baseURL: String {
        didSet { UserDefaults.standard.set(baseURL, forKey: Keys.baseURL) }
    }

    @Published var modelName: String {
        didSet { UserDefaults.standard.set(modelName, forKey: Keys.modelName) }
    }

    @Published var apiKey: String {
        didSet { UserDefaults.standard.set(apiKey, forKey: Keys.apiKey) }
    }

    @Published var weatherCity: String {
        didSet { UserDefaults.standard.set(weatherCity, forKey: Keys.weatherCity) }
    }

    @Published var enabledTopics: Set<SmartMessageTopic> {
        didSet {
            UserDefaults.standard.set(enabledTopics.map(\.rawValue), forKey: Keys.enabledTopics)
        }
    }

    @Published var isNotificationListeningEnabled: Bool {
        didSet { UserDefaults.standard.set(isNotificationListeningEnabled, forKey: Keys.isNotificationListeningEnabled) }
    }

    @Published var notificationAppNames: String {
        didSet { UserDefaults.standard.set(notificationAppNames, forKey: Keys.notificationAppNames) }
    }

    @Published var isActivityAwarenessEnabled: Bool {
        didSet { UserDefaults.standard.set(isActivityAwarenessEnabled, forKey: Keys.isActivityAwarenessEnabled) }
    }

    @Published var canReadWindowTitles: Bool {
        didSet { UserDefaults.standard.set(canReadWindowTitles, forKey: Keys.canReadWindowTitles) }
    }

    init() {
        isEnabled = UserDefaults.standard.object(forKey: Keys.isEnabled) as? Bool ?? false
        provider = SmartProvider(rawValue: UserDefaults.standard.string(forKey: Keys.provider) ?? "") ?? .deepSeek
        baseURL = UserDefaults.standard.string(forKey: Keys.baseURL) ?? SmartProvider.deepSeek.defaultBaseURL
        modelName = UserDefaults.standard.string(forKey: Keys.modelName) ?? SmartProvider.deepSeek.defaultModel
        apiKey = UserDefaults.standard.string(forKey: Keys.apiKey) ?? ""
        weatherCity = UserDefaults.standard.string(forKey: Keys.weatherCity) ?? ""

        let savedTopics = UserDefaults.standard.stringArray(forKey: Keys.enabledTopics) ?? []
        let topics = Set(savedTopics.compactMap(SmartMessageTopic.init(rawValue:)))
        enabledTopics = topics.isEmpty ? [.comfort, .joke] : topics
        isNotificationListeningEnabled = UserDefaults.standard.object(forKey: Keys.isNotificationListeningEnabled) as? Bool ?? true
        notificationAppNames = UserDefaults.standard.string(forKey: Keys.notificationAppNames) ?? "微信,WeChat,飞书,Lark"
        isActivityAwarenessEnabled = UserDefaults.standard.object(forKey: Keys.isActivityAwarenessEnabled) as? Bool ?? true
        canReadWindowTitles = UserDefaults.standard.object(forKey: Keys.canReadWindowTitles) as? Bool ?? false
    }

    func applyProviderDefaults() {
        baseURL = provider.defaultBaseURL
        modelName = provider.defaultModel
    }

    func snapshot() -> SmartCompanionConfiguration {
        SmartCompanionConfiguration(
            isEnabled: isEnabled,
            baseURL: baseURL,
            modelName: modelName,
            apiKey: apiKey,
            weatherCity: weatherCity,
            enabledTopics: enabledTopics
        )
    }

    private enum Keys {
        static let isEnabled = "SmartCompanion.IsEnabled"
        static let provider = "SmartCompanion.Provider"
        static let baseURL = "SmartCompanion.BaseURL"
        static let modelName = "SmartCompanion.ModelName"
        static let apiKey = "SmartCompanion.APIKey"
        static let weatherCity = "SmartCompanion.WeatherCity"
        static let enabledTopics = "SmartCompanion.EnabledTopics"
        static let isNotificationListeningEnabled = "SmartCompanion.IsNotificationListeningEnabled"
        static let notificationAppNames = "SmartCompanion.NotificationAppNames"
        static let isActivityAwarenessEnabled = "SmartCompanion.IsActivityAwarenessEnabled"
        static let canReadWindowTitles = "SmartCompanion.CanReadWindowTitles"
    }
}

enum SmartProvider: String, CaseIterable, Identifiable, Sendable {
    case deepSeek
    case custom

    var id: String { rawValue }

    var displayName: String {
        switch self {
        case .deepSeek: "DeepSeek"
        case .custom: "自定义"
        }
    }

    var defaultBaseURL: String {
        switch self {
        case .deepSeek: "https://api.deepseek.com"
        case .custom: ""
        }
    }

    var defaultModel: String {
        switch self {
        case .deepSeek: "deepseek-v4-flash"
        case .custom: ""
        }
    }
}

enum SmartMessageTopic: String, CaseIterable, Identifiable, Sendable {
    case joke
    case weather
    case comfort
    case focus

    var id: String { rawValue }

    var title: String {
        switch self {
        case .joke: "冷笑话"
        case .weather: "天气"
        case .comfort: "安慰"
        case .focus: "工作提醒"
        }
    }

    var icon: String {
        switch self {
        case .joke: "theatermasks"
        case .weather: "cloud.sun"
        case .comfort: "heart"
        case .focus: "brain"
        }
    }

    var promptName: String {
        switch self {
        case .joke: "冷笑话"
        case .weather: "天气提醒"
        case .comfort: "安慰"
        case .focus: "工作陪伴"
        }
    }
}

struct SmartCompanionConfiguration: Sendable {
    let isEnabled: Bool
    let baseURL: String
    let modelName: String
    let apiKey: String
    let weatherCity: String
    let enabledTopics: Set<SmartMessageTopic>

    var canRequestModel: Bool {
        isEnabled
            && !baseURL.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
            && !modelName.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
            && !apiKey.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
    }
}
