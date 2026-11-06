// The Controls screen: which key, mouse button or controller button presses each Switch control, shared by the iOS app (the in-game menu's Controls… and the settings screen's
// Controls section) and the Mac app's settings window (dm/mac/Launcher.swift). The bindings are dm/host/keymap.h's, read and written through its calls (keymap_api.h): the game's
// defaults (the base map, the profile's mouse_buttons and key_binds) and the player's overrides over them, which the screen keeps as controls.keymap in the app's UserDefaults (the
// host's DM_KEYMAP; iOS applies a change at once, the Mac at the next start). One list of Switch controls in groups, each row with the inputs bound to it as key caps, for the
// keyboard and mouse or for a controller (a segmented control). Tap a row, then press a key, click a mouse button (in the click box) or press a controller button: it replaces the
// row's bindings of that kind (a key replaces keys, a mouse button mouse buttons) and leaves the control it pressed before. Esc cancels. While the screen is open, no input reaches the game.
import SwiftUI
#if os(iOS)
import UIKit
#else
import AppKit
import GameController
#endif

struct SwitchControl: Identifiable { let out, glyph, label: String; var id: String { out } }
struct ControlGroup: Identifiable { let title, controllerNote, keyboardNote: String; let rows: [SwitchControl]; var id: String { title } }

let controlGroups: [ControlGroup] = [
    ControlGroup(title: "Face buttons", controllerNote: "By position, as on a Switch controller: the right button is A, the bottom one B.", keyboardNote: "",
                 rows: [SwitchControl(out: "A", glyph: "A", label: "A"), SwitchControl(out: "B", glyph: "B", label: "B"),
                        SwitchControl(out: "X", glyph: "X", label: "X"), SwitchControl(out: "Y", glyph: "Y", label: "Y")]),
    ControlGroup(title: "Shoulders", controllerNote: "", keyboardNote: "",
                 rows: [SwitchControl(out: "L", glyph: "L", label: "L"), SwitchControl(out: "R", glyph: "R", label: "R"),
                        SwitchControl(out: "ZL", glyph: "ZL", label: "ZL"), SwitchControl(out: "ZR", glyph: "ZR", label: "ZR")]),
    ControlGroup(title: "Sticks", controllerNote: "A controller's sticks stay its sticks; a button bound to a direction pushes the stick all the way.",
                 keyboardNote: "Shift + an arrow key turns the right stick while no Shift key is bound. With Mouse: Camera, moving the mouse turns it too.",
                 rows: [SwitchControl(out: "LS_UP", glyph: "L↑", label: "Left stick up"), SwitchControl(out: "LS_DOWN", glyph: "L↓", label: "Left stick down"),
                        SwitchControl(out: "LS_LEFT", glyph: "L←", label: "Left stick left"), SwitchControl(out: "LS_RIGHT", glyph: "L→", label: "Left stick right"),
                        SwitchControl(out: "L3", glyph: "L3", label: "Left stick press"),
                        SwitchControl(out: "RS_UP", glyph: "R↑", label: "Right stick up"), SwitchControl(out: "RS_DOWN", glyph: "R↓", label: "Right stick down"),
                        SwitchControl(out: "RS_LEFT", glyph: "R←", label: "Right stick left"), SwitchControl(out: "RS_RIGHT", glyph: "R→", label: "Right stick right"),
                        SwitchControl(out: "R3", glyph: "R3", label: "Right stick press")]),
    ControlGroup(title: "D-pad", controllerNote: "", keyboardNote: "",
                 rows: [SwitchControl(out: "UP", glyph: "↑", label: "Up"), SwitchControl(out: "DOWN", glyph: "↓", label: "Down"),
                        SwitchControl(out: "LEFT", glyph: "←", label: "Left"), SwitchControl(out: "RIGHT", glyph: "→", label: "Right")]),
    ControlGroup(title: "System", controllerNote: "", keyboardNote: "Shake moves the controller as if shaken (its motion sensor).",
                 rows: [SwitchControl(out: "PLUS", glyph: "+", label: "Plus"), SwitchControl(out: "MINUS", glyph: "−", label: "Minus"),
                        SwitchControl(out: "SHAKE", glyph: "≋", label: "Shake")]),
]

enum ControlsDevice: String, CaseIterable, Identifiable {
    case keyboard = "Keyboard & Mouse", controller = "Controller"
    var id: String { rawValue }
    func owns(_ token: String) -> Bool { (self == .controller) == token.hasPrefix("pad:") }
}

final class ControlsModel: ObservableObject, Identifiable {
    let defaults: [String: String]   // input token → output: the game's bindings
    let fixed: String?               // DM_KEYMAP set by an env file or at launch: shown, not changed
    @Published private(set) var map: [String: String]
    @Published var listening: String?   // the output a row waits for an input for
    private let defaultsText: String
    private let store: (String) -> Void   // the player's overrides: saved (and on iOS applied)

    init(gameBinds: String, player: String, fixed: String?, store: @escaping (String) -> Void) {
        defaultsText = String(cString: dm_km_apply(dm_km_base(), gameBinds))
        defaults = Self.parse(defaultsText)
        map = Self.parse(String(cString: dm_km_apply(defaultsText, fixed ?? player)))
        self.fixed = fixed; self.store = store
    }

    static func parse(_ text: String) -> [String: String] {   // keymap.h's text, canonical (dm_km_apply's): only bound inputs
        var m: [String: String] = [:]
        for w in text.split(separator: " ") {
            let p = w.split(separator: "=", maxSplits: 1)
            if p.count == 2, p[1] != "NONE" { m[String(p[0])] = String(p[1]) }
        }
        return m
    }
    static func label(_ token: String) -> String { String(cString: dm_km_label(token)) }
    private static func rank(_ t: String) -> Int { t.hasPrefix("kb:") ? 0 : t.hasPrefix("mouse:") ? 1 : 2 }
    private static func kind(_ t: String) -> Substring { t.prefix { $0 != ":" } }

    var overrides: String {   // what DM_KEYMAP holds: the inputs whose output differs from the game's
        let overlay = Set(defaults.keys).union(map.keys).sorted().map { "\($0)=\(map[$0] ?? "NONE")" }.joined(separator: " ")
        let full = String(cString: dm_km_apply("", overlay))
        return String(cString: dm_km_diff(full, defaultsText))
    }
    func inputs(_ out: String, _ d: ControlsDevice) -> [String] {
        map.filter { $0.value == out && d.owns($0.key) }.map(\.key).sorted { Self.rank($0) == Self.rank($1) ? $0 < $1 : Self.rank($0) < Self.rank($1) }
    }
    func changed(_ out: String, _ d: ControlsDevice) -> Bool {
        Set(inputs(out, d)) != Set(defaults.filter { $0.value == out && d.owns($0.key) }.map(\.key))
    }
    var changedAny: Bool { map != defaults }

    func bind(_ token: String, to out: String) {
        for (k, v) in map where v == out && Self.kind(k) == Self.kind(token) { map[k] = nil }
        map[token] = out; commit()
    }
    func clear(_ out: String, _ d: ControlsDevice) {
        for (k, v) in map where v == out && d.owns(k) { map[k] = nil }
        commit()
    }
    func reset(_ out: String, _ d: ControlsDevice) {
        for k in Set(defaults.keys).union(map.keys) where d.owns(k) && (defaults[k] == out || map[k] == out) { map[k] = defaults[k] }
        commit()
    }
    func resetAll() { map = defaults; commit() }
    private func commit() { if fixed == nil { store(overrides) } }

    // An input pressed while the screen is open: binds it to the listening row. Esc cancels; ⌘ is the system's
    func captured(_ token: String) {
        guard let out = listening, fixed == nil else { return }
        if token == "kb:Escape" { listening = nil; return }
        if token == "kb:LeftGUI" || token == "kb:RightGUI" { return }
        listening = nil
        bind(token, to: out)
    }
}

struct ControlsView: View {
    @ObservedObject var m: ControlsModel
    let done: () -> Void
    @State private var device: ControlsDevice = .keyboard

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Controls").font(.headline)
                Spacer()
                Button("Done", action: done).fontWeight(.semibold)
            }
            .padding([.horizontal, .top], 16).padding(.bottom, 8)
            Picker("Bindings for", selection: $device) {
                ForEach(ControlsDevice.allCases) { Text($0.rawValue).tag($0) }
            }
            .pickerStyle(.segmented).labelsHidden().padding(.horizontal, 16).padding(.bottom, 8)
            Form {
                if let f = m.fixed {
                    Section { Text("DM_KEYMAP is set by an env file or at launch, so these bindings are shown, not changed: \(f.isEmpty ? "(empty)" : f)").font(.footnote) }
                }
                ForEach(controlGroups) { g in
                    let note = device == .keyboard ? g.keyboardNote : g.controllerNote
                    Section {
                        ForEach(g.rows) { row($0) }
                    } header: {
                        Text(g.title)
                    } footer: {
                        if !note.isEmpty { Text(note) }
                    }
                }
                Section {
                    Button("Reset to game defaults", role: .destructive) { m.resetAll() }.disabled(!m.changedAny || m.fixed != nil)
                } footer: {
                    Text(device == .keyboard ? "Tap a control, then press a key or click a mouse button for it. Esc cancels." : "Tap a control, then press a controller button for it.")
                }
            }
            .formStyle(.grouped)
            .disabled(m.fixed != nil)
        }
        #if os(macOS)
        .frame(minWidth: 520, idealWidth: 560, minHeight: 600, idealHeight: 680)
        #endif
        .overlay { if let out = m.listening { listeningCard(out) } }
        .onAppear { ControlsCapture.start(m) }
        .onDisappear { m.listening = nil; ControlsCapture.stop() }
    }

    private func row(_ c: SwitchControl) -> some View {
        let inputs = m.inputs(c.out, device)
        return HStack(spacing: 10) {
            ControlGlyph(text: c.glyph)
            Text(c.label)
            Spacer(minLength: 8)
            if inputs.isEmpty {
                Text("Not bound").font(.callout).foregroundStyle(.secondary)
            } else {
                HStack(spacing: 4) { ForEach(inputs, id: \.self) { KeyCap(text: ControlsModel.label($0)) } }
            }
            if m.changed(c.out, device) {
                Button { m.reset(c.out, device) } label: { Image(systemName: "arrow.uturn.backward") }
                    .buttonStyle(.borderless).help("Back to the game's default").accessibilityLabel("Reset \(c.label)")
            }
        }
        .contentShape(Rectangle())
        .onTapGesture { m.listening = c.out }
        .contextMenu {
            Button("Change…") { m.listening = c.out }
            Button("Clear") { m.clear(c.out, device) }
            Button("Reset to game default") { m.reset(c.out, device) }
        }
        .accessibilityElement(children: .combine)
        .accessibilityAddTraits(.isButton)
        .accessibilityHint("Then press a key or button to bind it")
    }

    private func listeningCard(_ out: String) -> some View {
        let c = controlGroups.flatMap(\.rows).first { $0.out == out }
        return ZStack {
            Color.black.opacity(0.35).ignoresSafeArea()
            VStack(spacing: 14) {
                ControlGlyph(text: c?.glyph ?? out).scaleEffect(1.4).padding(.top, 4)
                Text("Press a key…").font(.title3.weight(.semibold))
                Text("…or a controller button for \(c?.label ?? out). Esc cancels.").multilineTextAlignment(.center).foregroundStyle(.secondary)
                ClickCatcher { m.captured($0) }
                    .frame(height: 64)
                    .overlay { Text("Click here with a mouse button").font(.callout).foregroundStyle(.secondary).allowsHitTesting(false) }
                    .overlay { RoundedRectangle(cornerRadius: 10).strokeBorder(.secondary, style: StrokeStyle(lineWidth: 1, dash: [5])).allowsHitTesting(false) }
                Button("Cancel") { m.listening = nil }
            }
            .padding(24).frame(maxWidth: 360)
            .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 16))
        }
    }
}

struct ControlGlyph: View {   // a Switch control as a small badge
    let text: String
    var body: some View {
        Text(text).font(.system(.caption, design: .rounded).weight(.bold))
            .frame(minWidth: 30, minHeight: 24).padding(.horizontal, 2)
            .background(Capsule().fill(Color.accentColor.opacity(0.22)))
            .accessibilityHidden(true)
    }
}

struct KeyCap: View {   // a bound input as a key cap
    let text: String
    var body: some View {
        Text(text).font(.caption.weight(.medium)).lineLimit(1)
            .padding(.horizontal, 7).padding(.vertical, 3)
            .background(RoundedRectangle(cornerRadius: 5).fill(.quaternary))
            .overlay(RoundedRectangle(cornerRadius: 5).strokeBorder(.secondary.opacity(0.5)))
    }
}

// The settings screens' row that opens the Controls screen
struct ControlsEntry: View {
    let make: () -> ControlsModel
    @State private var model: ControlsModel?
    var body: some View {
        Button { model = make() } label: {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Keyboard, mouse and controller")
                    Text("Which key or button presses each Switch control").font(.footnote).foregroundStyle(.secondary)
                }
                Spacer()
                Image(systemName: "chevron.right").foregroundStyle(.tertiary)
            }
            .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
        .sheet(item: $model) { m in ControlsView(m: m) { model = nil } }   // on the button, not the Section (Launcher.swift's Cloud saves: a Section hands its modifiers to every row)
    }
}

#if os(iOS)
// The keys, the mouse's right and middle buttons and the controllers come from the gamepad's GameController handlers (DmGamepad.m dm_keymap_capture), which keep them from the game
// while the screen is open; a left click is the click box's, so a click on the screen's own buttons binds nothing
private weak var captureTarget: ControlsModel?
enum ControlsCapture {
    static func start(_ m: ControlsModel) {
        captureTarget = m
        dm_keymap_capture { t in
            guard let t else { return }
            let s = String(cString: t)
            DispatchQueue.main.async { captureTarget?.captured(s) }
        }
    }
    static func stop() { captureTarget = nil; dm_keymap_capture(nil) }
}

private final class ClickCatcherView: UIView {
    var onClick: ((String) -> Void)?
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let t = touches.first, t.type == .indirectPointer else { return }   // a mouse or trackpad, not a finger
        let mask = event?.buttonMask ?? .primary
        onClick?(mask.contains(.secondary) ? "mouse:right" : mask.isEmpty || mask.contains(.primary) ? "mouse:left" : "mouse:middle")
    }
}
private struct ClickCatcher: UIViewRepresentable {
    let onClick: (String) -> Void
    func makeUIView(context: Context) -> ClickCatcherView { let v = ClickCatcherView(); v.backgroundColor = .clear; v.onClick = onClick; return v }
    func updateUIView(_ v: ClickCatcherView, context: Context) { v.onClick = onClick }
}

extension ControlsModel {
    // This game's: its bindings compiled in (game_profile.h), the player's from controls.keymap (or DM_KEYMAP); a change is saved and the running game takes it at once
    static func forThisGame() -> ControlsModel {
        ControlsModel(gameBinds: String(cString: dm_keymap_game()), player: String(cString: dm_keymap_player()), fixed: ProcessInfo.processInfo.environment["DM_KEYMAP"]) { text in
            if text.isEmpty { UserDefaults.standard.removeObject(forKey: "controls.keymap") } else { UserDefaults.standard.set(text, forKey: "controls.keymap") }
            dm_keymap_set(text)
        }
    }
}

// The in-game menu's Controls…: the screen as a sheet over the game
func presentControls() {
    let window = UIApplication.shared.connectedScenes.compactMap { ($0 as? UIWindowScene)?.keyWindow }.first
    guard var top = window?.rootViewController else { return }
    while let p = top.presentedViewController { top = p }
    weak var host: UIViewController?
    let h = DmHostingController(rootView: ControlsView(m: .forThisGame()) { host?.dismiss(animated: true) })
    host = h
    h.modalPresentationStyle = .formSheet
    top.present(h, animated: true)
}
#else
// The keyboard (a local event monitor), the controllers (GameController) and the click box's mouse buttons, in the settings window before the game starts
enum ControlsCapture {
    private static var monitor: Any?
    private static weak var target: ControlsModel?
    private static var last: [ObjectIdentifier: UInt32] = [:]
    private static var observer: NSObjectProtocol?

    static func start(_ m: ControlsModel) {
        target = m
        monitor = NSEvent.addLocalMonitorForEvents(matching: [.keyDown, .flagsChanged]) { e in
            guard let m = target, m.listening != nil else { return e }
            if e.type == .keyDown {
                if e.modifierFlags.contains(.command) { return e }
                if let t = dm_km_mac_key(Int32(e.keyCode)) { m.captured(String(cString: t)) }
                return nil
            }
            if let t = modifierDown(e) { m.captured(t) }
            return e
        }
        for c in GCController.controllers() { attach(c) }
        observer = NotificationCenter.default.addObserver(forName: .GCControllerDidConnect, object: nil, queue: .main) { n in if let c = n.object as? GCController { attach(c) } }
    }
    static func stop() {
        if let mo = monitor { NSEvent.removeMonitor(mo) }
        monitor = nil; target = nil; last = [:]
        for c in GCController.controllers() { c.extendedGamepad?.valueChangedHandler = nil }
        if let o = observer { NotificationCenter.default.removeObserver(o) }
        observer = nil
    }
    // A modifier pressed (flagsChanged with its device-specific flag now set: NX_DEVICE{L,R}{SHIFT,CTL,ALT}KEYMASK), as front_mac.m on_modifier reads it
    private static func modifierDown(_ e: NSEvent) -> String? {
        let masks: [UInt16: UInt] = [56: 0x2, 60: 0x4, 59: 0x1, 62: 0x2000, 58: 0x20, 61: 0x40]
        guard let mask = masks[e.keyCode], e.modifierFlags.rawValue & mask != 0, let t = dm_km_mac_key(Int32(e.keyCode)) else { return nil }
        return String(cString: t)
    }
    // A controller's pressed elements as keymap.h's bits, as dm/host/pad_apple.h dm_pad_elements reads them: face buttons by position, a Nintendo pad by its labels
    static func elements(_ g: GCExtendedGamepad) -> UInt32 {
        let cat = g.controller?.productCategory ?? ""
        let labelled = cat == "Switch Pro Controller" || cat.contains("Joy-Con")
        let face = labelled ? [g.buttonA, g.buttonB, g.buttonX, g.buttonY] : [g.buttonB, g.buttonA, g.buttonY, g.buttonX]
        let rest: [GCControllerButtonInput?] = [g.leftThumbstickButton, g.rightThumbstickButton, g.leftShoulder, g.rightShoulder, g.leftTrigger, g.rightTrigger,
                                                g.buttonMenu, g.buttonOptions, g.dpad.left, g.dpad.up, g.dpad.right, g.dpad.down]
        var m: UInt32 = 0
        for (i, b) in (face.map { Optional($0) } + rest).enumerated() where b?.isPressed == true { m |= 1 << UInt32(i) }
        return m
    }
    private static func attach(_ c: GCController) {
        guard let g = c.extendedGamepad else { return }
        last[ObjectIdentifier(c)] = elements(g)   // what is held as the screen opens is not a press
        g.valueChangedHandler = { g, _ in
            guard let c = g.controller else { return }
            let now = elements(g), fresh = now & ~(last[ObjectIdentifier(c)] ?? 0)
            last[ObjectIdentifier(c)] = now
            if fresh != 0, let t = dm_km_pad(Int32(fresh.trailingZeroBitCount)) { target?.captured(String(cString: t)) }
        }
    }
}

private final class ClickCatcherView: NSView {
    var onClick: ((String) -> Void)?
    override func acceptsFirstMouse(for event: NSEvent?) -> Bool { true }
    override func mouseDown(with e: NSEvent) { onClick?("mouse:left") }
    override func rightMouseDown(with e: NSEvent) { onClick?("mouse:right") }
    override func otherMouseDown(with e: NSEvent) { if e.buttonNumber == 2 { onClick?("mouse:middle") } }
}
private struct ClickCatcher: NSViewRepresentable {
    let onClick: (String) -> Void
    func makeNSView(context: Context) -> ClickCatcherView { let v = ClickCatcherView(); v.onClick = onClick; return v }
    func updateNSView(_ v: ClickCatcherView, context: Context) { v.onClick = onClick }
}

extension ControlsModel {
    // The Mac app's: the game's bindings from launcher.json (settings.py key_binds), the player's from controls.keymap, which launcher.c passes on as DM_KEYMAP at every start;
    // a DM_KEYMAP an env file or the launch environment set wins, and is shown
    static func mac(gameBinds: String) -> ControlsModel {
        let fixed = ProcessInfo.processInfo.environment["DM_KEYMAP"]
        return ControlsModel(gameBinds: gameBinds, player: UserDefaults.standard.string(forKey: "controls.keymap") ?? "", fixed: fixed) { text in
            if text.isEmpty { UserDefaults.standard.removeObject(forKey: "controls.keymap") } else { UserDefaults.standard.set(text, forKey: "controls.keymap") }
        }
    }
}
#endif
