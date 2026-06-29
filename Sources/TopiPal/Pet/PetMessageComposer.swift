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

        let seed = DialogueSeed(
            opener: pick(timeOpeners(hour: hour) + moodOpeners(memory.mood)),
            subject: pick(ambientSubjects(memory: memory)),
            ending: pick(softEndings(memory: memory))
        )
        return polish(seed.joined(maxLength: 24))
    }

    static func interactionMessage(context: PetInteractionContext) -> String {
        if let familiarLine = familiarInteractionMessage(context: context) {
            return familiarLine
        }

        if context.memory.tapStreak >= 7 {
            return pick([
                "你今天真的很爱戳我。",
                "再点下去，我要申请休息了。",
                "连续召唤成功，我已经在你面前了。",
                "这不是互动，这是连击测试吧？",
                "我懂了，你是在确认我还活着。"
            ], avoiding: context.memory)
        }

        if context.memory.tapStreak >= 4 {
            return pick([
                "我在，别把我点晕。",
                "收到，注意力已经给你了。",
                "你这节奏有点急，我跟上了。",
                "我刚站稳，你又来了。",
                "好好好，我已经转向你了。"
            ], avoiding: context.memory)
        }

        let areaLine = areaSubject(context.area)
        return compose(
            pick(moodOpeners(context.memory.mood)),
            areaLine,
            pick(softEndings(memory: context.memory))
        )
    }

    private static func activityAmbientMessage(activity: ActivityContext?, memory: PetMemory) -> String? {
        guard let activity, activity.confidence >= 65 else { return nil }

        switch activity.mode {
        case .coding:
            return pick([
                "这段代码先别急着推翻。",
                "你在和逻辑较劲，我陪你盯着。",
                "编辑器安静下来时，答案会露头。",
                "我看见你在拆一团线。",
                "先别急着重写，可能只差一小块。"
            ], avoiding: memory)
        case .terminal:
            return pick([
                "终端在办正事，我先不吵。",
                "命令行这会儿挺认真。",
                "敲回车前，我替你紧张一下。",
                "这行命令看起来有点关键。",
                "我先屏住呼吸，看它跑完。"
            ], avoiding: memory)
        case .chatting:
            return pick([
                "对话那边有点热闹。",
                "你先聊，我在旁边守着。",
                "消息来回跑，我不插嘴。",
                "这像是有新剧情了。",
                "你回消息，我帮你看门。"
            ], avoiding: memory)
        case .watchingVideo:
            return pick([
                "观影模式，我把声音放轻。",
                "你先看，通知来了我再叫你。",
                "这一段我也安静看着。"
            ], avoiding: memory)
        case .meeting:
            return pick([
                "会议中，我先把存在感收小。",
                "你先说正事，我只轻轻提醒。",
                "通话时间，我在旁边待命。"
            ], avoiding: memory)
        case .reading:
            return pick([
                "你看得挺稳，我先不打断。",
                "这页内容可以慢慢消化。",
                "眼睛也要休息一下。",
                "我把声音放低，陪你看完。"
            ], avoiding: memory)
        case .designing:
            return pick([
                "这个细节被你盯上了。",
                "画布正在变顺眼。",
                "你刚才那一下调整有点意思。",
                "别急，感觉是慢慢调出来的。"
            ], avoiding: memory)
        case .browsing:
            return pick([
                "这个页面像是有线索。",
                "信息有点多，我帮你按住一角。",
                "你在翻找，我先不催。"
            ], avoiding: memory)
        case .idle:
            return nil
        case .unknown:
            return nil
        }
    }

    static func autoActionMessage(area: PetHitArea, memory: PetMemory, hour: Int = Calendar.current.component(.hour, from: Date())) -> String {
        let movement: String
        switch area {
        case .head:
            movement = pick(["脑袋刚冒出一个小想法", "灵感从我头顶路过", "我刚把想法理顺一点", "我好像想起一件小事"])
        case .body:
            movement = pick(["我换了个更舒服的姿势", "这会儿适合安静陪着", "我把桌面气氛稳住了", "我刚刚偷偷伸了个懒腰"])
        case .left:
            movement = pick(["我往左边看了一眼", "左边没什么要紧事", "左边暂时归我看着"])
        case .right:
            movement = pick(["我往右边看了一眼", "右边也先帮你留意着", "右边有我盯一会儿"])
        case .feet:
            movement = pick(["该换个坐姿了", "身体也该醒一下", "坐太久会被我发现"])
        }
        return compose(pick(timeOpeners(hour: hour) + moodOpeners(memory.mood)), movement, pick(["。", "，继续。", "，我在。"]))
    }

    static func lifeMomentMessage(moment: LifeMoment, memory: PetMemory, activity: ActivityContext?) -> String {
        let isWorking = activity.map { [.coding, .terminal, .designing, .reading].contains($0.mode) } ?? false

        switch moment {
        case .morningStart:
            return pick([
                "早呀，我已经在桌面就位了。",
                "今天开工，我先帮你守住节奏。",
                "新的一天开始，我坐好啦。",
                "先慢慢启动，别一上来就满电冲刺。"
            ], avoiding: memory)
        case .lunch:
            if isWorking {
                return pick([
                    "饭点到了，代码可以等一口饭。",
                    "先吃饭，问题不会趁你夹菜逃跑。",
                    "午饭时间，我替你按住这段思路。",
                    "该补能量了，不然脑袋会降帧。"
                ], avoiding: memory)
            }
            return pick([
                "饭点到了，去补点能量吧。",
                "午饭时间，我先不催你。",
                "吃点东西，下午会好过一点。",
                "桌面我看着，你去吃饭。"
            ], avoiding: memory)
        case .afternoonReset:
            return pick([
                "下午容易卡住，先喝口水。",
                "三点附近，脑袋可以重启一下。",
                "起来动一动，思路可能就回来了。",
                "我建议你给眼睛放个小假。"
            ], avoiding: memory)
        case .offWork:
            if isWorking {
                return pick([
                    "到下班点了，别把自己也编译进去。",
                    "可以收个尾了，剩下的明天也认路。",
                    "今天推进不少，准备慢慢降速吧。",
                    "下班信号到了，我轻轻提醒一下。"
                ], avoiding: memory)
            }
            return pick([
                "下班点到了，今天辛苦。",
                "可以把工作音量调小一点了。",
                "今天的桌面任务快收队吧。",
                "该从工作模式里出来了。"
            ], avoiding: memory)
        case .dinner:
            return pick([
                "晚饭时间到了，别只喂电脑。",
                "去吃晚饭吧，我帮你看着桌面。",
                "晚上也要认真吃点东西。",
                "先吃饭，回来我还在。"
            ], avoiding: memory)
        case .lateNight:
            if isWorking {
                return pick([
                    "有点晚了，别把今天熬成明天。",
                    "夜深了，这段可以先留个记号。",
                    "我小声提醒：该收一下了。",
                    "再推进一点可以，但别太硬撑。"
                ], avoiding: memory)
            }
            return pick([
                "夜深了，我把声音放轻。",
                "今天差不多该慢下来了。",
                "晚一点了，眼睛也该下班。",
                "我还在，但你可以休息了。"
            ], avoiding: memory)
        }
    }

    static func pointerFollowMessage(memory: PetMemory) -> String {
        switch memory.mood {
        case .sleepy:
            return pick([
                "慢一点，我要被你晃醒了。",
                "你这样绕，我困意都转飞了。",
                "轻点转，我眼睛要打结了。"
            ], avoiding: memory)
        case .focused:
            return pick([
                "你要把我转晕了哟。",
                "我在跟，你别突然变向。",
                "等等，我的视线快追不上了。"
            ], avoiding: memory)
        case .curious, .alert:
            return pick([
                "你要把我转晕了哟。",
                "我看见你在逗我了。",
                "别绕太快，我真的在跟。"
            ], avoiding: memory)
        case .clingy:
            return pick([
                "又逗我，你要把我转晕了哟。",
                "我跟着呢，但别太快。",
                "你再绕一圈，我就装晕。"
            ], avoiding: memory)
        case .calm:
            return pick([
                "你要把我转晕了哟。",
                "我跟着看了，慢一点。",
                "鼠标跑这么快，是在逗我吗？"
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
                "\(source)又来了，我帮你先记着。",
                "\(source)今天消息有点密。",
                "\(source)还在响，先别漏掉。",
                "\(source)又亮了一下，我看到了。"
            ], avoiding: memory)
        }

        return pick([
            "\(source)有新消息。",
            "\(source)刚刚提醒你。",
            "\(source)亮了一下，我看到了。",
            "\(source)那边有动静。"
        ], avoiding: memory)
    }

    static func relationshipLevelUpMessage(level: PetRelationshipLevel, memory: PetMemory) -> String {
        switch level {
        case .new:
            return pick(["初次见面，我先记住你。", "我刚搬来桌面，请多指教。"], avoiding: memory)
        case .familiar:
            return pick([
                "我们好像开始熟起来了。",
                "我已经有点认得你的节奏了。",
                "以后我会更懂你一点。"
            ], avoiding: memory)
        case .close:
            return pick([
                "现在算老熟人了吧。",
                "你的桌面习惯，我记住不少了。",
                "我开始知道什么时候该安静了。"
            ], avoiding: memory)
        case .trusted:
            return pick([
                "我们已经很有默契了。",
                "你不用多说，我大概懂了。",
                "我会继续替你守着桌面。"
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

    private static func areaSubject(_ area: PetHitArea) -> String {
        switch area {
        case .head:
            return pick(["想法被你碰醒了", "脑袋开始转圈", "灵感差点掉出来", "你碰到我的想法开关了"])
        case .body:
            return pick(["这一下很安心", "我把抱抱收好", "我往你这边靠一点", "收到，我把陪伴模式打开"])
        case .left:
            return pick(["我往左边看了一眼", "左边暂时没事", "你点到左边啦", "左边这一下我收到了"])
        case .right:
            return pick(["我往右边看了一眼", "右边我也看到了", "你点到右边啦", "右边这下有点突然"])
        case .feet:
            return pick(["该起身换换气", "坐久了，动一下吧", "脚边提醒你休息一下", "腿也需要一点存在感"])
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
            if isFavoriteArea {
                return pick([
                    "又是这里，我记住了。",
                    "你还挺喜欢点这里的。",
                    "这个位置我已经有印象了。"
                ], avoiding: context.memory)
            }
            return nil
        case .close:
            if isFavoriteArea {
                return pick([
                    "我就知道你会点这里。",
                    "熟悉的位置，熟悉的你。",
                    "这里已经快变成暗号了。"
                ], avoiding: context.memory)
            }
            return pick([
                "今天换地方逗我了？",
                "这一下有点突然，但我接住了。",
                "我懂，你是在叫我。"
            ], avoiding: context.memory)
        case .trusted:
            if isFavoriteArea {
                return pick([
                    "不用猜，又是我们的老地方。",
                    "收到，老地方的信号。",
                    "你一点这里，我就知道你在叫我。"
                ], avoiding: context.memory)
            }
            return pick([
                "换个位置我也懂。",
                "你的小动作，我已经很熟了。",
                "我在，不用点太重。"
            ], avoiding: context.memory)
        }
    }

    private static func relationshipAmbientMessage(memory: PetMemory) -> String? {
        guard Int.random(in: 0..<100) < 22 else { return nil }

        switch memory.relationship.level {
        case .new:
            return nil
        case .familiar:
            return pick([
                "我开始摸到你的桌面节奏了。",
                "这几天你常来，我记着呢。",
                "我还在学习怎么陪你。"
            ], avoiding: memory)
        case .close:
            if let appName = memory.relationship.favoriteAppName {
                return pick([
                    "\(appName)那边我会多留意一点。",
                    "我知道你常被\(appName)叫走。",
                    "\(appName)一动，我会帮你看着。"
                ], avoiding: memory)
            }
            return pick([
                "你忙的时候，我会少打扰。",
                "我们现在挺有节奏了。",
                "我知道什么时候该安静一点。"
            ], avoiding: memory)
        case .trusted:
            if let appName = memory.relationship.favoriteAppName {
                return pick([
                    "\(appName)交给我盯一会儿。",
                    "你先忙，\(appName)有动静我叫你。",
                    "你的常用消息口，我守着呢。"
                ], avoiding: memory)
            }
            return pick([
                "你先忙，我在老位置。",
                "不用管我，我会自己找点事做。",
                "今天也按我们的节奏来。"
            ], avoiding: memory)
        }
    }

    private static func wechatNotificationMessage(memory: PetMemory) -> String {
        let streak = memory.notificationStreak
        if streak >= 5 {
            return pick([
                "微信又响了，今天你很受欢迎。",
                "微信消息排队了，我先帮你守着。",
                "微信还在找你，要不要先看一眼？",
                "聊天那边很热闹，我已经提醒过你啦。",
                "微信又来了，我都快认识它了。"
            ], avoiding: memory)
        }

        if streak >= 3 {
            return pick([
                "微信又来一条，别让它等太久。",
                "有人在微信连续敲门。",
                "微信今天有点黏你。",
                "聊天那边又亮了，我看见了。",
                "微信消息有点密，我帮你盯着。"
            ], avoiding: memory)
        }

        return pick([
            "微信来消息了。",
            "有人在微信找你。",
            "微信亮了一下，我看到了。",
            "聊天那边敲了敲门。",
            "微信有新动静。",
            "有人把消息递到门口了。",
            "微信提醒到了，我轻轻叫你。",
            "那边有人找你，我先记着。"
        ], avoiding: memory)
    }

    private static func larkNotificationMessage(memory: PetMemory) -> String {
        let streak = memory.notificationStreak
        if streak >= 5 {
            return pick([
                "飞书又来了，工作频道有点忙。",
                "飞书消息排队了，我先帮你盯着。",
                "飞书还在闪，可能有事等你处理。",
                "工作那边持续有动静，我提醒你一下。",
                "飞书今天很勤快，我已经看见了。"
            ], avoiding: memory)
        }

        if streak >= 3 {
            return pick([
                "飞书又来一条，像是正事。",
                "工作那边连续敲了两下。",
                "飞书消息有点密，先别漏掉。",
                "办公频道又动了，我帮你记着。",
                "飞书又亮了，可能要看一眼。"
            ], avoiding: memory)
        }

        return pick([
            "飞书来消息了。",
            "工作那边有新动静。",
            "飞书提醒到了。",
            "办公频道敲了敲门。",
            "飞书亮了一下，我看到了。",
            "工作消息到了，我轻轻叫你。",
            "飞书那边像有事找你。",
            "有条工作消息停在门口。"
        ], avoiding: memory)
    }

    private static func timeOpeners(hour: Int) -> [String] {
        if hour < 6 { return ["小声说", "夜班模式", "灯还亮着"] }
        if hour < 11 { return ["早呀", "开机完成", "今天启动"] }
        if hour < 14 { return ["午间检查", "中场提示", "饭点附近"] }
        if hour >= 18 { return ["收尾时间", "晚一点了", "今天辛苦"] }
        return ["现在", "这会儿", "我看着呢"]
    }

    private static func moodOpeners(_ mood: PetMood) -> [String] {
        switch mood {
        case .calm:
            return ["慢慢来", "我在旁边", "这会儿不急"]
        case .curious:
            return ["我看看", "有点意思", "我凑近一点"]
        case .clingy:
            return ["我知道啦", "你又来了", "这次我听见了"]
        case .focused:
            return ["我盯着呢", "先稳住", "节奏别断"]
        case .sleepy:
            return ["困意路过", "小声提醒", "半梦半醒"]
        case .alert:
            return ["有动静", "我听见了", "我提醒你"]
        }
    }

    private static func ambientSubjects(memory: PetMemory) -> [String] {
        var subjects = [
            "屏幕这边很安静",
            "我在桌面边上坐好",
            "今天的节奏还在",
            "我把小事先放轻"
        ]
        if memory.energy < 35 {
            subjects.append(contentsOf: ["我有点犯困", "我先省着点精神"])
        }
        if memory.lastAppName != nil {
            subjects.append("刚才的提醒我记着")
        }
        return subjects
    }

    private static func softEndings(memory: PetMemory) -> [String] {
        switch memory.mood {
        case .sleepy:
            return ["。", "，声音放轻。", "，别太晚。"]
        case .focused:
            return ["。", "，继续推进。", "，我帮你看着。"]
        case .alert:
            return ["。", "，我没漏掉。", "，有事再叫你。"]
        default:
            return ["。", "，我在。", "，别急。"]
        }
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

    private static func compose(_ first: String, _ second: String, _ third: String) -> String {
        polish(first + second + third)
    }

    private static func polish(_ value: String) -> String {
        value
            .replacingOccurrences(of: "。。", with: "。")
            .replacingOccurrences(of: "，。", with: "。")
            .trimmingCharacters(in: .whitespacesAndNewlines)
    }

    private static func pick(_ values: [String]) -> String {
        values.randomElement() ?? ""
    }

    private static func pick(_ values: [String], avoiding memory: PetMemory) -> String {
        let fresh = values.filter { !memory.recentMessages.contains($0) }
        return (fresh.isEmpty ? values : fresh).randomElement() ?? ""
    }
}

private struct DialogueSeed {
    let opener: String
    let subject: String
    let ending: String

    func joined(maxLength: Int) -> String {
        let value = opener + subject + ending
        guard value.count > maxLength else { return value }
        return subject + ending
    }
}
