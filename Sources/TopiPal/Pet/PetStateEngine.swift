import Foundation

struct PetRuntimeState: Sendable, Equatable {
    let mood: PetMood
    let activity: UserActivityMode
    let shouldSpeak: Bool
    let speakInterval: ClosedRange<Int>
    let reason: String
}

enum PetStateEngine {
    static func runtimeState(
        activity: ActivityContext?,
        memory: PetMemory,
        now: Date = Date()
    ) -> PetRuntimeState {
        guard let activity else {
            return PetRuntimeState(
                mood: memory.mood,
                activity: .unknown,
                shouldSpeak: true,
                speakInterval: 18...35,
                reason: "no activity context"
            )
        }

        switch activity.mode {
        case .coding:
            return PetRuntimeState(
                mood: .focused,
                activity: .coding,
                shouldSpeak: true,
                speakInterval: 45...90,
                reason: "coding"
            )
        case .terminal:
            return PetRuntimeState(
                mood: .focused,
                activity: .terminal,
                shouldSpeak: true,
                speakInterval: 40...80,
                reason: "terminal"
            )
        case .chatting:
            return PetRuntimeState(
                mood: .curious,
                activity: .chatting,
                shouldSpeak: true,
                speakInterval: 20...45,
                reason: "chatting"
            )
        case .meeting:
            return PetRuntimeState(
                mood: .calm,
                activity: .meeting,
                shouldSpeak: false,
                speakInterval: 90...150,
                reason: "meeting quiet"
            )
        case .watchingVideo:
            return PetRuntimeState(
                mood: .calm,
                activity: .watchingVideo,
                shouldSpeak: false,
                speakInterval: 80...140,
                reason: "watching video quiet"
            )
        case .browsing, .reading:
            return PetRuntimeState(
                mood: .curious,
                activity: activity.mode,
                shouldSpeak: true,
                speakInterval: 35...70,
                reason: activity.mode.rawValue
            )
        case .designing:
            return PetRuntimeState(
                mood: .curious,
                activity: .designing,
                shouldSpeak: true,
                speakInterval: 40...75,
                reason: "designing"
            )
        case .idle:
            return PetRuntimeState(
                mood: .sleepy,
                activity: .idle,
                shouldSpeak: false,
                speakInterval: 90...160,
                reason: "idle"
            )
        case .unknown:
            return PetRuntimeState(
                mood: memory.mood,
                activity: .unknown,
                shouldSpeak: true,
                speakInterval: 25...50,
                reason: "unknown"
            )
        }
    }
}
