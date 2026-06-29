import AppKit
import Combine
import QuartzCore
import SwiftUI

@MainActor
final class IslandWindowManager {
    private let model: PetModel
    private let settings: SettingsStore
    private var panel: NSPanel?
    private var panelDelegate: PanelMoveObserver?
    private weak var roundedContentView: NSView?
    private var outsideClickMonitor: Any?
    private var screenChangeObserver: NSObjectProtocol?
    private var cancellables = Set<AnyCancellable>()
    private let transitionDuration: TimeInterval = 0.36

    init(model: PetModel, settings: SettingsStore) {
        self.model = model
        self.settings = settings
        observeScreenChanges()
        observeModel()
    }

    func show() {
        if panel == nil {
            panel = makePanel()
        }

        guard let panel else { return }
        position(panel)
        panel.orderFrontRegardless()
    }

    func toggleVisibility() {
        guard let panel else {
            show()
            return
        }

        if panel.isVisible {
            panel.orderOut(nil)
        } else {
            show()
        }
    }

    private func makePanel() -> NSPanel {
        let content = PetRootView(model: model, settings: settings)
        let hostingView = NSHostingView(rootView: content)
        hostingView.wantsLayer = true
        hostingView.layer?.backgroundColor = NSColor.clear.cgColor
        hostingView.layer?.isOpaque = false
        hostingView.translatesAutoresizingMaskIntoConstraints = false

        let containerView = NSView()
        containerView.wantsLayer = true
        containerView.layer?.backgroundColor = NSColor.clear.cgColor
        containerView.layer?.isOpaque = false
        containerView.layer?.masksToBounds = true
        containerView.addSubview(hostingView)

        NSLayoutConstraint.activate([
            hostingView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
            hostingView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
            hostingView.topAnchor.constraint(equalTo: containerView.topAnchor),
            hostingView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor)
        ])
        roundedContentView = containerView

        let panel = NSPanel(
            contentRect: NSRect(origin: .zero, size: settings.compactSize),
            styleMask: [.borderless, .nonactivatingPanel],
            backing: .buffered,
            defer: false
        )

        panel.isOpaque = false
        panel.backgroundColor = .clear
        panel.hasShadow = false
        panel.level = .floating
        panel.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary, .stationary]
        panel.hidesOnDeactivate = false
        panel.isMovableByWindowBackground = true
        panel.acceptsMouseMovedEvents = true
        panel.contentView = containerView
        panel.contentView?.wantsLayer = true
        panel.contentView?.layer?.backgroundColor = NSColor.clear.cgColor
        panel.contentView?.layer?.isOpaque = false
        let panelDelegate = PanelMoveObserver(settings: settings)
        panel.delegate = panelDelegate
        self.panelDelegate = panelDelegate
        applyWindowMask(to: panel)
        return panel
    }

    private func position(_ panel: NSPanel) {
        let targetSize = targetSize
        let screen = NSScreen.main ?? NSScreen.screens.first
        guard let visibleFrame = screen?.visibleFrame else { return }

        let savedAnchor = settings.savedWindowAnchor
        let anchorX = savedAnchor?.x ?? visibleFrame.midX
        let anchorY = savedAnchor?.y ?? (visibleFrame.maxY - settings.topInset)
        let x = anchorX - targetSize.width / 2
        let y = anchorY - targetSize.height
        let nextFrame = NSRect(x: x, y: y, width: targetSize.width, height: targetSize.height)
        if panel.frame != nextFrame {
            animate(panel: panel, to: nextFrame)
        } else {
            applyWindowMask(to: panel)
        }
    }

    private func applyWindowMask(to panel: NSPanel) {
        let cornerRadius = targetCornerRadius
        panel.contentView?.layer?.cornerRadius = cornerRadius
        panel.contentView?.layer?.cornerCurve = .continuous
        panel.contentView?.layer?.masksToBounds = false
        roundedContentView?.layer?.cornerRadius = cornerRadius
        roundedContentView?.layer?.cornerCurve = .continuous
        roundedContentView?.layer?.masksToBounds = false
    }

    private var targetSize: CGSize {
        let baseSize = model.phase == .expanded ? settings.expandedSize : settings.compactSize
        let scaleOverflow = max(0, settings.petScale - 1)
        return CGSize(
            width: baseSize.width + scaleOverflow * 160,
            height: baseSize.height + scaleOverflow * 90
        )
    }

    private var targetCornerRadius: CGFloat {
        0
    }

    private func animate(panel: NSPanel, to frame: NSRect) {
        let cornerRadius = targetCornerRadius

        NSAnimationContext.runAnimationGroup { context in
            context.duration = transitionDuration
            context.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
            panel.animator().setFrame(frame, display: true)
            animateCornerRadius(on: panel.contentView?.layer, to: cornerRadius)
            animateCornerRadius(on: roundedContentView?.layer, to: cornerRadius)
        } completionHandler: { [weak self, weak panel] in
            Task { @MainActor in
                guard let self, let panel else { return }
                self.applyWindowMask(to: panel)
            }
        }
    }

    private func animateCornerRadius(on layer: CALayer?, to cornerRadius: CGFloat) {
        guard let layer else { return }

        let animation = CABasicAnimation(keyPath: "cornerRadius")
        animation.fromValue = layer.presentation()?.cornerRadius ?? layer.cornerRadius
        animation.toValue = cornerRadius
        animation.duration = transitionDuration
        animation.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
        layer.cornerRadius = cornerRadius
        layer.add(animation, forKey: "cornerRadius")
    }

    private func installOutsideClickMonitor() {
        guard outsideClickMonitor == nil else { return }

        outsideClickMonitor = NSEvent.addGlobalMonitorForEvents(matching: [.leftMouseDown, .rightMouseDown]) { [weak model] _ in
            Task { @MainActor in
                model?.collapse()
            }
        }
    }

    private func removeOutsideClickMonitor() {
        if let outsideClickMonitor {
            NSEvent.removeMonitor(outsideClickMonitor)
            self.outsideClickMonitor = nil
        }
    }

    private func observeScreenChanges() {
        screenChangeObserver = NotificationCenter.default.addObserver(
            forName: NSApplication.didChangeScreenParametersNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in
            Task { @MainActor in
                guard let self, let panel = self.panel else { return }
                self.position(panel)
            }
        }
    }

    private func observeModel() {
        model.$phase
            .dropFirst()
            .sink { [weak self] _ in
                guard let self, let panel = self.panel else { return }
                self.position(panel)
                if self.model.phase == .expanded {
                    self.installOutsideClickMonitor()
                } else {
                    self.removeOutsideClickMonitor()
                }
            }
            .store(in: &cancellables)

        settings.$petScale
            .dropFirst()
            .sink { [weak self] _ in
                guard let self, let panel = self.panel else { return }
                self.resizeKeepingCurrentAnchor(panel)
            }
            .store(in: &cancellables)
    }

    private func resizeKeepingCurrentAnchor(_ panel: NSPanel) {
        let targetSize = targetSize
        let frame = panel.frame
        let nextFrame = NSRect(
            x: frame.midX - targetSize.width / 2,
            y: frame.maxY - targetSize.height,
            width: targetSize.width,
            height: targetSize.height
        )
        if panel.frame != nextFrame {
            animate(panel: panel, to: nextFrame)
        } else {
            applyWindowMask(to: panel)
        }
    }
}

@MainActor
private final class PanelMoveObserver: NSObject, NSWindowDelegate {
    private let settings: SettingsStore

    init(settings: SettingsStore) {
        self.settings = settings
    }

    func windowDidMove(_ notification: Notification) {
        guard let window = notification.object as? NSWindow else { return }
        settings.savedWindowAnchor = CGPoint(x: window.frame.midX, y: window.frame.maxY)
    }
}
