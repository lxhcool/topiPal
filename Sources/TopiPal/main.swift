import AppKit
import SwiftUI

let application = NSApplication.shared
let delegate = AppDelegate()
application.delegate = delegate
application.setActivationPolicy(.regular)
application.run()

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    private var windowManager: IslandWindowManager?
    private var statusBarController: StatusBarController?
    private var notificationMonitor: AppNotificationMonitor?
    private let settings = SettingsStore()
    private let petModel = PetModel()

    func applicationDidFinishLaunching(_ notification: Notification) {
        let windowManager = IslandWindowManager(model: petModel, settings: settings)
        self.windowManager = windowManager
        self.statusBarController = StatusBarController(
            model: petModel,
            settings: settings,
            windowManager: windowManager
        )
        let notificationMonitor = AppNotificationMonitor(model: petModel, settings: petModel.smartSettings)
        self.notificationMonitor = notificationMonitor

        windowManager.show()
        notificationMonitor.start()
        petModel.startActivityAwareness()
        petModel.startAmbientMessages()
        petModel.startAutoActions()

        Task { @MainActor in
            try? await Task.sleep(nanoseconds: 1_500_000_000)
            petModel.showWelcomeHint()
        }
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        false
    }

    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        NSApp.unhide(nil)
        NSApp.activate(ignoringOtherApps: true)
        Task { @MainActor in
            self.statusBarController?.showSettings()
        }
        return true
    }
}
