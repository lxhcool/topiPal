import AppKit
import SwiftUI

@MainActor
final class StatusBarController {
    private let item: NSStatusItem
    private weak var model: PetModel?
    private weak var settings: SettingsStore?
    private weak var windowManager: IslandWindowManager?
    private var settingsPanel: NSWindow?

    init(model: PetModel, settings: SettingsStore, windowManager: IslandWindowManager) {
        self.model = model
        self.settings = settings
        self.windowManager = windowManager
        self.item = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)

        if let button = item.button {
            button.image = Self.statusBarImage()
        }

        let menu = NSMenu()
        menu.addItem(NSMenuItem(title: "展开/收起", action: #selector(toggleExpanded), keyEquivalent: ""))
        menu.addItem(NSMenuItem(title: "隐藏/显示", action: #selector(toggleVisibility), keyEquivalent: ""))
        menu.addItem(NSMenuItem(title: "设置...", action: #selector(openSettings), keyEquivalent: ","))
        menu.addItem(.separator())
        menu.addItem(NSMenuItem(title: "退出 TopiPal", action: #selector(quit), keyEquivalent: "q"))

        assignTargets(in: menu)
        item.menu = menu
    }

    @objc private func toggleExpanded() {
        model?.toggle()
        windowManager?.show()
    }

    @objc private func toggleVisibility() {
        windowManager?.toggleVisibility()
    }

    func showSettings() {
        NSApp.unhide(nil)
        NSApp.activate(ignoringOtherApps: true)

        if let existing = settingsPanel {
            existing.orderFrontRegardless()
            existing.makeKeyAndOrderFront(nil)
            return
        }

        guard let model, let settings else { return }

        let contentView = SettingsPanelView(model: model, settings: settings) { [weak self] in
            self?.settingsPanel?.close()
            self?.settingsPanel = nil
        }
        let hostingView = NSHostingView(rootView: contentView)
        hostingView.wantsLayer = true
        hostingView.layer?.backgroundColor = NSColor.clear.cgColor

        let panel = TopiSettingsPanel(
            contentRect: NSRect(x: 0, y: 0, width: 810, height: 610),
            styleMask: [.titled, .closable, .miniaturizable, .fullSizeContentView],
            backing: .buffered,
            defer: false
        )
        panel.title = ""
        panel.titleVisibility = .hidden
        panel.titlebarAppearsTransparent = true
        panel.backgroundColor = .clear
        panel.isOpaque = false
        panel.hasShadow = true
        panel.isMovableByWindowBackground = false
        panel.contentView = hostingView
        panel.isReleasedWhenClosed = false
        panel.standardWindowButton(.closeButton)?.isHidden = true
        panel.standardWindowButton(.miniaturizeButton)?.isHidden = true
        panel.standardWindowButton(.zoomButton)?.isHidden = true
        panel.center()
        panel.orderFrontRegardless()
        panel.makeKeyAndOrderFront(nil)

        settingsPanel = panel
    }

    @objc private func openSettings() {
        showSettings()
    }

    @objc private func quit() {
        NSApp.terminate(nil)
    }

    private func characterMenuItem() -> NSMenuItem {
        let item = NSMenuItem(title: "切换人物", action: nil, keyEquivalent: "")
        let submenu = NSMenu()

        model?.characters.forEach { character in
            let menuItem = NSMenuItem(title: character.name, action: #selector(selectCharacter(_:)), keyEquivalent: "")
            menuItem.representedObject = character.id
            menuItem.state = character.id == model?.selectedCharacterID ? .on : .off
            submenu.addItem(menuItem)
        }

        item.submenu = submenu
        return item
    }

    private func assignTargets(in menu: NSMenu) {
        for item in menu.items {
            item.target = self
            if let submenu = item.submenu {
                assignTargets(in: submenu)
            }
        }
    }

    private static func statusBarImage() -> NSImage? {
        let image = Bundle.module.url(forResource: "MenuBarIcon", withExtension: "png", subdirectory: "App")
            .flatMap(NSImage.init(contentsOf:))
            ?? NSImage(systemSymbolName: "sparkle", accessibilityDescription: "TopiPal")
        image?.size = NSSize(width: 18, height: 18)
        image?.isTemplate = false
        return image
    }

    @objc private func selectCharacter(_ sender: NSMenuItem) {
        guard let id = sender.representedObject as? String,
              let character = model?.characters.first(where: { $0.id == id }) else {
            return
        }

        model?.selectCharacter(character)
    }
}

private final class TopiSettingsPanel: NSPanel {
    override var canBecomeKey: Bool { true }
    override var canBecomeMain: Bool { true }
}
