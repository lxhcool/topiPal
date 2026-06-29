import Foundation

enum SmartMessageService {
    static func rewriteMessage(
        draft: String,
        event: SmartMessageEvent,
        activity: ActivityContext?,
        memory: PetMemory,
        configuration: SmartCompanionConfiguration
    ) async -> String? {
        guard configuration.canRequestModel else { return nil }
        let trimmedDraft = draft.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmedDraft.isEmpty else { return nil }

        let weather = configuration.enabledTopics.contains(.weather)
            ? await WeatherService.currentWeatherSummary(city: configuration.weatherCity)
            : nil

        let prompt = rewritePrompt(
            draft: trimmedDraft,
            event: event,
            activity: activity,
            memory: memory,
            weather: weather
        )
        let message = try? await ChatCompletionClient(configuration: configuration).complete(prompt: prompt)
        return sanitizeBubbleMessage(message, fallback: trimmedDraft, maxLength: event.maxLength)
    }

    static func makeWeatherMomentMessage(
        moment: WeatherMoment,
        activity: ActivityContext?,
        memory: PetMemory,
        configuration: SmartCompanionConfiguration
    ) async -> String? {
        guard configuration.canRequestModel else { return nil }
        guard configuration.enabledTopics.contains(.weather) else { return nil }
        guard let weather = await WeatherService.currentWeatherSummary(city: configuration.weatherCity) else { return nil }

        let activityLine = activity.map { "\($0.mode.title)，当前应用：\($0.appName ?? "未知")" } ?? "未知"
        let prompt = """
        你是一个会观察桌面状态的 2D 桌面宠物。根据真实天气，在合适时间轻轻提醒用户一句。
        时间段：\(moment.promptName)
        当前活动：\(activityLine)
        心情：\(memory.mood.rawValue)
        天气：\(weather)

        要求：不要编造天气；语气有趣但不油；不要说教；不要解释；不要引号；不要 emoji；28 个汉字以内。
        """
        let message = try? await ChatCompletionClient(configuration: configuration).complete(prompt: prompt)
        return sanitizeBubbleMessage(message, fallback: "", maxLength: 28)
    }

    static func makeAmbientMessage(configuration: SmartCompanionConfiguration) async -> String? {
        guard configuration.canRequestModel else { return nil }
        guard let topic = pickTopic(from: configuration) else { return nil }

        let weather = topic == .weather
            ? await WeatherService.currentWeatherSummary(city: configuration.weatherCity)
            : nil
        if topic == .weather, weather == nil {
            return nil
        }

        let prompt = makePrompt(topic: topic, weather: weather)
        return try? await ChatCompletionClient(configuration: configuration).complete(prompt: prompt)
    }

    static func makeInteractionMessage(area: PetHitArea, configuration: SmartCompanionConfiguration) async -> String? {
        guard configuration.canRequestModel else { return nil }

        let areaName: String
        switch area {
        case .head: areaName = "头部"
        case .body: areaName = "身体"
        case .left: areaName = "左侧"
        case .right: areaName = "右侧"
        case .feet: areaName = "脚边"
        }

        let prompt = """
        你是一个安静但有性格的桌面宠物。用户刚刚点击了你的\(areaName)。
        写一句像真实陪伴者的中文气泡文案，要有一点观察感。
        要求：16 个汉字以内；不要泛泛加油；不要解释；不要引号；不要 emoji；不要提到 AI。
        """
        return try? await ChatCompletionClient(configuration: configuration).complete(prompt: prompt)
    }

    static func testMessage(configuration: SmartCompanionConfiguration) async -> SmartMessageTestResult {
        guard configuration.canRequestModel else {
            return .failure("配置不完整：需要启用智能气泡，并填写 Base URL、模型和 API Key")
        }

        do {
            let message = try await ChatCompletionClient(configuration: configuration).complete(
                prompt: "写一句 12 个汉字以内的中文桌面宠物问候。不要引号，不要 emoji。"
            )
            guard let message, !message.isEmpty else {
                return .failure("模型返回为空")
            }
            return .success(message)
        } catch {
            return .failure(error.localizedDescription)
        }
    }

    private static func pickTopic(from configuration: SmartCompanionConfiguration) -> SmartMessageTopic? {
        let topics = Array(configuration.enabledTopics)
        guard !topics.isEmpty else { return nil }
        return topics.randomElement()
    }

    private static func makePrompt(topic: SmartMessageTopic, weather: String?) -> String {
        switch topic {
        case .joke:
            return """
            你是一个安静但有性格的桌面宠物。写一句轻松的中文冷笑话气泡文案。
            要求：24 个汉字以内；像随口吐槽；不要解释；不要引号；不要 emoji；不要网络烂梗。
            """
        case .weather:
            return """
            你是一个桌面宠物。根据真实天气写一句关心用户的中文气泡文案。
            天气信息：\(weather ?? "")
            要求：26 个汉字以内；不要编造天气；不要解释；不要引号；不要 emoji。
            """
        case .comfort:
            return """
            你是一个安静但有性格的桌面宠物。写一句温柔但不油腻的中文安慰文案。
            要求：22 个汉字以内；像陪在旁边；不要说教；不要引号；不要 emoji；不要提到 AI。
            """
        case .focus:
            return """
            你是一个安静但有性格的桌面宠物。写一句轻量的工作陪伴提醒。
            要求：20 个汉字以内；像朋友提醒；不要命令；不要引号；不要 emoji。
            """
        }
    }

    private static func rewritePrompt(
        draft: String,
        event: SmartMessageEvent,
        activity: ActivityContext?,
        memory: PetMemory,
        weather: String?
    ) -> String {
        let activityLine: String
        if let activity {
            activityLine = "\(activity.mode.title)，当前应用：\(activity.appName ?? "未知")，置信度：\(activity.confidence)"
        } else {
            activityLine = "未知"
        }

        let weatherLine = weather.map { "天气：\($0)" } ?? "天气：无"

        return """
        你是一个会观察桌面状态的 2D 桌面宠物。下面是本地生成的气泡草稿，请只做一次自然改写。
        事件：\(event.promptName)
        活动：\(activityLine)
        心情：\(memory.mood.rawValue)，点击连续次数：\(memory.tapStreak)，消息连续次数：\(memory.notificationStreak)
        关系：\(memory.relationship.level.title)，认识 \(memory.relationship.daysKnown) 天，总互动 \(memory.relationship.totalInteractions) 次
        \(weatherLine)
        草稿：\(draft)

        要求：
        1. 必须保留草稿含义和当前事件，不要改成无关话题。
        2. 语气像一个有性格但不吵闹的桌面宠物，可以有一点机灵和观察感。
        3. 不要解释，不要引号，不要 emoji，不要提到 AI 或模型。
        4. \(event.maxLength) 个汉字以内。
        """
    }

    private static func sanitizeBubbleMessage(_ message: String?, fallback: String, maxLength: Int) -> String? {
        guard let message else { return nil }
        let cleaned = message
            .trimmingCharacters(in: .whitespacesAndNewlines)
            .trimmingCharacters(in: CharacterSet(charactersIn: "\"“”"))
        guard !cleaned.isEmpty, cleaned != fallback else { return nil }
        if cleaned.count <= maxLength { return cleaned }
        return String(cleaned.prefix(maxLength))
    }
}

enum SmartMessageEvent: Sendable {
    case ambient
    case interaction(PetHitArea)
    case autoAction(PetHitArea)
    case lifeMoment(LifeMoment)

    var promptName: String {
        switch self {
        case .ambient:
            return "空闲陪伴"
        case .interaction(let area):
            return "用户点击了\(area.displayName)"
        case .autoAction(let area):
            return "宠物自己做了\(area.displayName)动作"
        case .lifeMoment(let moment):
            return moment.promptName
        }
    }

    var maxLength: Int {
        switch self {
        case .ambient:
            return 30
        case .interaction:
            return 24
        case .autoAction, .lifeMoment:
            return 28
        }
    }
}

enum LifeMoment: String, CaseIterable, Sendable {
    case morningStart
    case lunch
    case afternoonReset
    case offWork
    case dinner
    case lateNight

    var promptName: String {
        switch self {
        case .morningStart:
            return "早间开工"
        case .lunch:
            return "午饭时间"
        case .afternoonReset:
            return "下午回血"
        case .offWork:
            return "下班时间"
        case .dinner:
            return "晚饭时间"
        case .lateNight:
            return "夜间收尾"
        }
    }
}

enum WeatherMoment: String, Sendable {
    case morning
    case noon
    case evening

    var promptName: String {
        switch self {
        case .morning:
            return "早上"
        case .noon:
            return "中午"
        case .evening:
            return "傍晚"
        }
    }
}

enum SmartMessageTestResult {
    case success(String)
    case failure(String)
}

private struct ChatCompletionClient {
    let configuration: SmartCompanionConfiguration

    func complete(prompt: String) async throws -> String? {
        let endpoints = try chatCompletionsURLs()
        var lastError: Error?

        for endpoint in endpoints {
            do {
                return try await complete(prompt: prompt, endpoint: endpoint)
            } catch {
                lastError = error
                if !shouldTryNextEndpoint(after: error) {
                    throw error
                }
            }
        }

        throw lastError ?? URLError(.badURL)
    }

    private func complete(prompt: String, endpoint: URL) async throws -> String? {
        var request = URLRequest(url: endpoint)
        request.httpMethod = "POST"
        request.timeoutInterval = 18
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.setValue("Bearer \(configuration.apiKey)", forHTTPHeaderField: "Authorization")
        request.httpBody = try JSONEncoder().encode(
            ChatCompletionRequest(
                model: configuration.modelName,
                messages: [
                    ChatMessage(role: "system", content: "直接输出最终桌面宠物气泡文案。不要输出推理过程、解释、前后缀或引号。"),
                    ChatMessage(role: "user", content: prompt)
                ],
                temperature: 0.85,
                maxTokens: 256
            )
        )

        let (data, response) = try await URLSession.shared.data(for: request)
        guard let httpResponse = response as? HTTPURLResponse else {
            throw SmartMessageError.invalidResponse
        }
        guard (200..<300).contains(httpResponse.statusCode) else {
            let body = String(data: data, encoding: .utf8) ?? ""
            throw SmartMessageError.httpError(statusCode: httpResponse.statusCode, body: body)
        }

        let payload = try JSONDecoder().decode(ChatCompletionResponse.self, from: data)
        let content = payload.choices.compactMap { choice -> String? in
            if let content = choice.message?.content, !content.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
                return content
            }
            if let text = choice.text, !text.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
                return text
            }
            return nil
        }
        .first { !$0.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty }

        guard let content else {
            let body = String(data: data, encoding: .utf8) ?? ""
            let hasReasoningOnly = payload.choices.contains { choice in
                (choice.message?.reasoningContent?.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty == false)
            }
            throw SmartMessageError.emptyContent(body: body, hasReasoningOnly: hasReasoningOnly)
        }

        return content
            .trimmingCharacters(in: .whitespacesAndNewlines)
            .trimmingCharacters(in: CharacterSet(charactersIn: "\"“”"))
    }

    private func chatCompletionsURLs() throws -> [URL] {
        let trimmed = configuration.baseURL.trimmingCharacters(in: .whitespacesAndNewlines)
        guard var components = URLComponents(string: trimmed) else {
            throw URLError(.badURL)
        }

        let path = components.path.trimmingCharacters(in: CharacterSet(charactersIn: "/"))
        if path.hasSuffix("chat/completions") {
            guard let url = components.url else { throw URLError(.badURL) }
            return [url]
        }

        components.path = "/" + ([path, "chat/completions"].filter { !$0.isEmpty }.joined(separator: "/"))
        guard let primaryURL = components.url else { throw URLError(.badURL) }

        var fallbackComponents = URLComponents(string: trimmed)
        let fallbackPath = fallbackComponents?.path.trimmingCharacters(in: CharacterSet(charactersIn: "/")) ?? ""
        if fallbackPath.isEmpty {
            fallbackComponents?.path = "/v1/chat/completions"
        } else if !fallbackPath.hasSuffix("/v1") && fallbackPath != "v1" {
            fallbackComponents?.path = "/" + [fallbackPath, "v1", "chat/completions"].joined(separator: "/")
        }

        if let fallbackURL = fallbackComponents?.url, fallbackURL != primaryURL {
            return [primaryURL, fallbackURL]
        }
        return [primaryURL]
    }

    private func shouldTryNextEndpoint(after error: Error) -> Bool {
        if case SmartMessageError.httpError(let statusCode, _) = error {
            return statusCode == 404 || statusCode == 405
        }
        return false
    }
}

private enum SmartMessageError: LocalizedError {
    case invalidResponse
    case emptyContent(body: String, hasReasoningOnly: Bool)
    case httpError(statusCode: Int, body: String)

    var errorDescription: String? {
        switch self {
        case .invalidResponse:
            return "模型服务没有返回有效 HTTP 响应"
        case .emptyContent(let body, let hasReasoningOnly):
            let trimmed = body.trimmingCharacters(in: .whitespacesAndNewlines)
            if hasReasoningOnly {
                return "模型只返回了 reasoning_content，没有返回最终 content。请换 deepseek-v4-pro，或继续用 v4-flash 再测一次。"
            }
            if trimmed.isEmpty {
                return "模型返回为空：响应体为空"
            }
            return "模型返回为空：\(String(trimmed.prefix(220)))"
        case .httpError(let statusCode, let body):
            let trimmed = body.trimmingCharacters(in: .whitespacesAndNewlines)
            if trimmed.isEmpty {
                return "模型请求失败：HTTP \(statusCode)"
            }
            return "模型请求失败：HTTP \(statusCode)，\(String(trimmed.prefix(180)))"
        }
    }
}

private extension URLError {
    var localizedDescription: String {
        switch code {
        case .secureConnectionFailed:
            return "TLS 连接失败：请检查代理/VPN/抓包证书，或系统是否信任当前网络证书"
        case .serverCertificateUntrusted, .serverCertificateHasBadDate, .serverCertificateHasUnknownRoot, .serverCertificateNotYetValid:
            return "TLS 证书不受信任：请检查代理证书或系统钥匙串证书信任"
        case .appTransportSecurityRequiresSecureConnection:
            return "ATS 拒绝连接：App 网络安全策略阻止了请求"
        case .cannotFindHost, .cannotConnectToHost, .networkConnectionLost, .notConnectedToInternet, .dnsLookupFailed:
            return "网络连接失败：\(NSLocalizedString(code.rawValue.description, comment: ""))"
        default:
            return self.errorUserInfo[NSLocalizedDescriptionKey] as? String ?? "网络错误：\(code.rawValue)"
        }
    }
}

private enum WeatherService {
    static func currentWeatherSummary(city: String) async -> String? {
        let city = city.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !city.isEmpty else { return nil }

        do {
            guard let location = try await geocode(city: city) else { return nil }
            let weather = try await currentWeather(latitude: location.latitude, longitude: location.longitude)
            return "\(location.name)：\(weather.temperature)℃，\(weather.description)，风速 \(weather.windSpeed)km/h"
        } catch {
            return nil
        }
    }

    private static func geocode(city: String) async throws -> WeatherLocation? {
        var components = URLComponents(string: "https://geocoding-api.open-meteo.com/v1/search")
        components?.queryItems = [
            URLQueryItem(name: "name", value: city),
            URLQueryItem(name: "count", value: "1"),
            URLQueryItem(name: "language", value: "zh"),
            URLQueryItem(name: "format", value: "json")
        ]
        guard let url = components?.url else { throw URLError(.badURL) }
        let (data, _) = try await URLSession.shared.data(from: url)
        let payload = try JSONDecoder().decode(GeocodingResponse.self, from: data)
        guard let result = payload.results?.first else { return nil }
        return WeatherLocation(name: result.name, latitude: result.latitude, longitude: result.longitude)
    }

    private static func currentWeather(latitude: Double, longitude: Double) async throws -> WeatherSummary {
        var components = URLComponents(string: "https://api.open-meteo.com/v1/forecast")
        components?.queryItems = [
            URLQueryItem(name: "latitude", value: "\(latitude)"),
            URLQueryItem(name: "longitude", value: "\(longitude)"),
            URLQueryItem(name: "current", value: "temperature_2m,weather_code,wind_speed_10m")
        ]
        guard let url = components?.url else { throw URLError(.badURL) }
        let (data, _) = try await URLSession.shared.data(from: url)
        let payload = try JSONDecoder().decode(ForecastResponse.self, from: data)
        return WeatherSummary(
            temperature: Int(payload.current.temperature.rounded()),
            description: weatherDescription(code: payload.current.weatherCode),
            windSpeed: Int(payload.current.windSpeed.rounded())
        )
    }

    private static func weatherDescription(code: Int) -> String {
        switch code {
        case 0: "晴"
        case 1, 2, 3: "多云"
        case 45, 48: "有雾"
        case 51, 53, 55, 56, 57: "毛毛雨"
        case 61, 63, 65, 66, 67, 80, 81, 82: "下雨"
        case 71, 73, 75, 77, 85, 86: "下雪"
        case 95, 96, 99: "雷雨"
        default: "天气变化中"
        }
    }
}

private struct ChatCompletionRequest: Encodable {
    let model: String
    let messages: [ChatMessage]
    let temperature: Double
    let maxTokens: Int

    enum CodingKeys: String, CodingKey {
        case model
        case messages
        case temperature
        case maxTokens = "max_tokens"
    }
}

private struct ChatMessage: Codable {
    let role: String
    let content: String
    let reasoningContent: String?

    init(role: String, content: String, reasoningContent: String? = nil) {
        self.role = role
        self.content = content
        self.reasoningContent = reasoningContent
    }

    enum CodingKeys: String, CodingKey {
        case role
        case content
        case reasoningContent = "reasoning_content"
    }
}

private struct ChatCompletionResponse: Decodable {
    let choices: [Choice]

    struct Choice: Decodable {
        let message: ChatMessage?
        let text: String?
    }
}

private struct GeocodingResponse: Decodable {
    let results: [LocationResult]?

    struct LocationResult: Decodable {
        let name: String
        let latitude: Double
        let longitude: Double
    }
}

private struct ForecastResponse: Decodable {
    let current: Current

    struct Current: Decodable {
        let temperature: Double
        let weatherCode: Int
        let windSpeed: Double

        enum CodingKeys: String, CodingKey {
            case temperature = "temperature_2m"
            case weatherCode = "weather_code"
            case windSpeed = "wind_speed_10m"
        }
    }
}

private struct WeatherLocation {
    let name: String
    let latitude: Double
    let longitude: Double
}

private struct WeatherSummary {
    let temperature: Int
    let description: String
    let windSpeed: Int
}
