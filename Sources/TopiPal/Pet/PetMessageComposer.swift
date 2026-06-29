import Foundation

enum PetMood: String, Sendable {
    case calm
    case curious
    case clingy
    case focused
    case sleepy
    case alert
}

struct PetMemory: Sendable {
    var mood: PetMood
    var energy: Int
    var tapStreak: Int
    var lastArea: PetHitArea?
    var lastAppName: String?
    var notificationStreak: Int
    var recentMessages: [String]
    var relationship: PetRelationshipSnapshot
}

struct PetInteractionContext: Sendable {
    let area: PetHitArea
    let hour: Int
    let memory: PetMemory
}

enum PetMessageComposer {
    static func ambientMessage(memory: PetMemory, activity: ActivityContext? = nil, hour: Int = Calendar.current.component(.hour, from: Date())) -> String {
        if let relationshipLine = relationshipAmbientMessage(memory: memory) {
            return relationshipLine
        }

        if let activitySpecific = activityAmbientMessage(activity: activity, memory: memory) {
            return activitySpecific
        }

        return pick(generalAmbientMessages(hour: hour, memory: memory), avoiding: memory)
    }

    static func interactionMessage(context: PetInteractionContext) -> String {
        if let familiarLine = familiarInteractionMessage(context: context) {
            return familiarLine
        }

        if context.memory.tapStreak >= 7 {
            return pick([
                "点了很多次，我先安静一下。",
                "我已经收到你的操作了。",
                "连续点击会切换动作，不会一直说话。"
            ], avoiding: context.memory)
        }

        if context.memory.tapStreak >= 4 {
            return pick([
                "我在切换动作了。",
                "已经响应了。",
                "别点太快，动作会跟不上。"
            ], avoiding: context.memory)
        }

        return areaSubject(context.area)
    }

    static func autoActionMessage(area: PetHitArea, memory: PetMemory, hour: Int = Calendar.current.component(.hour, from: Date())) -> String {
        switch area {
        case .head:
            return pick(["我换个动作。", "我活动一下。", "我动一下头。"], avoiding: memory)
        case .body:
            return pick(["我换个姿势。", "我活动一下。", "我待在这里。"], avoiding: memory)
        case .left:
            return pick(["我看一下左边。", "我往左边动一下。"], avoiding: memory)
        case .right:
            return pick(["我看一下右边。", "我往右边动一下。"], avoiding: memory)
        case .feet:
            return pick(["我活动一下。", "我换个站姿。"], avoiding: memory)
        }
    }

    static func lifeMomentMessage(moment: LifeMoment, memory: PetMemory, activity: ActivityContext?) -> String {
        let isWorking = activity.map { [.coding, .terminal, .designing, .reading].contains($0.mode) } ?? false

        switch moment {
        case .morningStart:
            return pick([
                "早上好，今天开始了。",
                "早上好，先慢慢进入状态。",
                "早上好，我在桌面上。"
            ], avoiding: memory)
        case .lunch:
            if isWorking {
                return pick([
                    "到饭点了，先去吃饭吧。",
                    "午饭时间到了，休息一下。",
                    "先吃饭，回来再继续。"
                ], avoiding: memory)
            }
            return pick([
                "午饭时间到了。",
                "该吃午饭了。",
                "先吃点东西吧。"
            ], avoiding: memory)
        case .afternoonReset:
            return pick([
                "下午容易累，喝口水吧。",
                "休息几分钟，眼睛会舒服一点。",
                "起来活动一下吧。"
            ], avoiding: memory)
        case .offWork:
            if isWorking {
                return pick([
                    "到下班时间了，可以收尾了。",
                    "今天先做到这里也可以。",
                    "工作时间差不多了，慢慢收尾吧。"
                ], avoiding: memory)
            }
            return pick([
                "到下班时间了。",
                "今天辛苦了。",
                "可以放松一下了。"
            ], avoiding: memory)
        case .dinner:
            return pick([
                "晚饭时间到了。",
                "去吃晚饭吧。",
                "晚上也要记得吃饭。"
            ], avoiding: memory)
        case .lateNight:
            if isWorking {
                return pick([
                    "已经很晚了，注意休息。",
                    "夜深了，可以先保存进度。",
                    "太晚了，别一直熬着。"
                ], avoiding: memory)
            }
            return pick([
                "已经很晚了，早点休息。",
                "夜深了，准备休息吧。",
                "今天差不多可以结束了。"
            ], avoiding: memory)
        }
    }

    static func pointerFollowMessage(memory: PetMemory) -> String {
        switch memory.mood {
        case .sleepy:
            return pick([
                "鼠标移动太快了。",
                "慢一点，我在跟着看。",
                "我有点跟不上鼠标。"
            ], avoiding: memory)
        case .focused:
            return pick([
                "我在跟着鼠标看。",
                "鼠标移动有点快。",
                "我看到你在逗我了。"
            ], avoiding: memory)
        case .curious, .alert, .clingy, .calm:
            return pick([
                "我看到鼠标在绕我。",
                "我在跟着鼠标看。",
                "别绕太快。"
            ], avoiding: memory)
        }
    }

    static func notificationMessage(appName: String, memory: PetMemory) -> String? {
        if appName.localizedCaseInsensitiveContains("飞书") || appName.localizedCaseInsensitiveContains("Lark") {
            return larkNotificationMessage(memory: memory)
        }

        if appName.localizedCaseInsensitiveContains("微信") || appName.localizedCaseInsensitiveContains("WeChat") {
            return wechatNotificationMessage(memory: memory)
        }

        let source = notificationSourceName(appName)
        if memory.notificationStreak >= 3 {
            return pick([
                "\(source)连续来了几条消息。",
                "\(source)消息比较多。",
                "\(source)又有新提醒。"
            ], avoiding: memory)
        }

        return pick([
            "\(source)有新消息。",
            "\(source)有新提醒。",
            "\(source)刚刚发来通知。"
        ], avoiding: memory)
    }

    static func relationshipLevelUpMessage(level: PetRelationshipLevel, memory: PetMemory) -> String {
        switch level {
        case .new:
            return pick(["你好，我会待在桌面上。", "你好，我会记住你的常用操作。"], avoiding: memory)
        case .familiar:
            return pick([
                "我开始熟悉你的使用习惯了。",
                "我记住了一些你的常用操作。",
                "以后我会少打扰一点。"
            ], avoiding: memory)
        case .close:
            return pick([
                "我已经比较熟悉你的桌面习惯了。",
                "我会更注意什么时候提醒你。",
                "我会尽量在合适的时候出现。"
            ], avoiding: memory)
        case .trusted:
            return pick([
                "我已经很熟悉你的使用节奏了。",
                "我会更克制地提醒你。",
                "我会安静陪着，不频繁打扰。"
            ], avoiding: memory)
        }
    }

    static func nextMood(afterInteraction area: PetHitArea, tapStreak: Int, hour: Int) -> PetMood {
        if hour < 6 { return .sleepy }
        if tapStreak >= 5 { return .clingy }
        switch area {
        case .head, .left, .right:
            return .curious
        case .body:
            return .calm
        case .feet:
            return .focused
        }
    }

    static func nextMoodAfterNotification(appName: String) -> PetMood {
        if appName.localizedCaseInsensitiveContains("飞书") || appName.localizedCaseInsensitiveContains("Lark") {
            return .focused
        }
        return .alert
    }

    private static func activityAmbientMessage(activity: ActivityContext?, memory: PetMemory) -> String? {
        guard let activity, activity.confidence >= 65 else { return nil }

        switch activity.mode {
        case .coding:
            return pick([
                "你在写代码，我先少说话。",
                "写代码时我会降低提醒频率。",
                "先专心写代码。"
            ], avoiding: memory)
        case .terminal:
            return pick([
                "终端正在运行，我先不打扰。",
                "等命令跑完再看结果。",
                "先看终端输出。"
            ], avoiding: memory)
        case .chatting:
            return pick([
                "你在聊天，我先安静一点。",
                "先回消息吧。",
                "聊天窗口有点忙。"
            ], avoiding: memory)
        case .watchingVideo:
            return pick([
                "你在看视频，我先安静。",
                "看视频时我会少提醒。",
                "我先不打断。"
            ], avoiding: memory)
        case .meeting:
            return pick([
                "会议中，我会保持安静。",
                "你在开会，我先不提醒。",
                "会议期间我会少打扰。"
            ], avoiding: memory)
        case .reading:
            return pick([
                "你在阅读，我先安静。",
                "看久了记得休息眼睛。",
                "先看完这一段吧。"
            ], avoiding: memory)
        case .designing:
            return pick([
                "你在调整设计，我先不打断。",
                "先看画布吧。",
                "做设计时我会少说话。"
            ], avoiding: memory)
        case .browsing:
            return pick([
                "你在浏览网页。",
                "先看网页内容吧。",
                "我先安静一点。"
            ], avoiding: memory)
        case .idle, .unknown:
            return nil
        }
    }

    private static func areaSubject(_ area: PetHitArea) -> String {
        switch area {
        case .head:
            return "切换头部动作。"
        case .body:
            return "切换身体动作。"
        case .left:
            return "切换左侧动作。"
        case .right:
            return "切换右侧动作。"
        case .feet:
            return "切换下方动作。"
        }
    }

    private static func familiarInteractionMessage(context: PetInteractionContext) -> String? {
        guard context.memory.tapStreak < 4 else { return nil }

        let favoriteArea = context.memory.relationship.favoriteArea
        let isFavoriteArea = favoriteArea == context.area

        switch context.memory.relationship.level {
        case .new:
            return nil
        case .familiar:
            return isFavoriteArea ? pick([
                "你经常点这个位置。",
                "这个位置我记住了。"
            ], avoiding: context.memory) : nil
        case .close:
            if isFavoriteArea {
                return pick([
                    "你还是喜欢点这个位置。",
                    "这个位置是你常用的互动点。"
                ], avoiding: context.memory)
            }
            return pick([
                "收到，我切换动作。",
                "我知道你在叫我。"
            ], avoiding: context.memory)
        case .trusted:
            if isFavoriteArea {
                return pick([
                    "收到，还是这个位置。",
                    "我知道这是你常点的位置。"
                ], avoiding: context.memory)
            }
            return pick([
                "收到，我在。",
                "我会切换动作。"
            ], avoiding: context.memory)
        }
    }

    private static func relationshipAmbientMessage(memory: PetMemory) -> String? {
        guard Int.random(in: 0..<100) < 12 else { return nil }

        switch memory.relationship.level {
        case .new:
            return nil
        case .familiar:
            return pick([
                "我在熟悉你的使用习惯。",
                "我会慢慢减少不必要的提醒。",
                "我记住了一些你的常用操作。"
            ], avoiding: memory)
        case .close:
            if let appName = memory.relationship.favoriteAppName {
                return pick([
                    "你经常收到\(appName)消息。",
                    "\(appName)有通知时我会提醒你。",
                    "我会留意\(appName)的消息。"
                ], avoiding: memory)
            }
            return pick([
                "你忙的时候，我会少说话。",
                "我会控制提醒频率。",
                "我会尽量不打断你。"
            ], avoiding: memory)
        case .trusted:
            if let appName = memory.relationship.favoriteAppName {
                return pick([
                    "我会继续留意\(appName)消息。",
                    "\(appName)有新通知时我会提醒你。",
                    "我知道\(appName)对你比较重要。"
                ], avoiding: memory)
            }
            return pick([
                "我会安静待着。",
                "我会少打扰你。",
                "需要时我会提醒你。"
            ], avoiding: memory)
        }
    }

    private static func wechatNotificationMessage(memory: PetMemory) -> String {
        let streak = memory.notificationStreak
        if streak >= 5 {
            return pick([
                "微信连续来了多条消息。",
                "微信消息比较多。",
                "微信又有新消息。"
            ], avoiding: memory)
        }

        if streak >= 3 {
            return pick([
                "微信连续来了几条消息。",
                "微信又有新消息。",
                "微信消息比较密。"
            ], avoiding: memory)
        }

        return pick([
            "微信来消息了。",
            "微信有新消息。",
            "有人在微信找你。"
        ], avoiding: memory)
    }

    private static func larkNotificationMessage(memory: PetMemory) -> String {
        let streak = memory.notificationStreak
        if streak >= 5 {
            return pick([
                "飞书连续来了多条消息。",
                "飞书消息比较多。",
                "飞书又有新消息。"
            ], avoiding: memory)
        }

        if streak >= 3 {
            return pick([
                "飞书连续来了几条消息。",
                "飞书又有新消息。",
                "飞书消息比较密。"
            ], avoiding: memory)
        }

        return pick([
            "飞书来消息了。",
            "飞书有新消息。",
            "飞书有新提醒。"
        ], avoiding: memory)
    }

    private static func generalAmbientMessages(hour: Int, memory: PetMemory) -> [String] {
        var messages: [String]
        if hour < 6 {
            messages = ["时间很晚了，注意休息。", "已经很晚了，早点休息。"]
        } else if hour < 11 {
            messages = ["早上好。", "今天开始了。", "先慢慢进入状态。"]
        } else if hour < 14 {
            messages = ["午间记得休息。", "中午可以放松一下。"]
        } else if hour >= 18 {
            messages = ["今天辛苦了。", "晚上记得休息。"]
        } else {
            messages = ["我会安静待着。", "需要时我会提醒你。", "我先不打扰。"]
        }

        if memory.energy < 35 {
            messages.append("我会少动一点。")
        }
        if memory.lastAppName != nil {
            messages.append("刚才的通知我已经提醒过了。")
        }
        return messages
    }

    private static func notificationSourceName(_ appName: String) -> String {
        if appName.localizedCaseInsensitiveContains("微信") || appName.localizedCaseInsensitiveContains("WeChat") {
            return "微信"
        }
        if appName.localizedCaseInsensitiveContains("飞书") || appName.localizedCaseInsensitiveContains("Lark") {
            return "飞书"
        }
        return appName
    }

    private static func pick(_ values: [String]) -> String {
        values.randomElement() ?? ""
    }

    private static func pick(_ values: [String], avoiding memory: PetMemory) -> String {
        let fresh = values.filter { !memory.recentMessages.contains($0) }
        return (fresh.isEmpty ? values : fresh).randomElement() ?? ""
    }
}
