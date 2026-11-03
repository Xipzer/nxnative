// In-game settings: the launcher's rows in a sheet over the running game (three-finger tap, front_ios.m). A setting the host reads again while it runs (liveKeys, dm_live.h)
// takes effect at once; any other is saved for the next start and says so. Choices are saved as the launcher saves them (only what differs from the game's default)
import SwiftUI
import UIKit

private let liveKeys: Set<String> = ["DM_ANISO", "DM_TRILINEAR", "DM_SHARPEN", "DM_PRESENT_UPSCALE", "DM_PRESENT_UPSCALE_OUT", "DM_QUALITY_AUTO",
                                      "DM_THERMAL_CAP", "DM_FRAME_WG", "DM_SHOW_FPS", "DM_RENDER_SCALE", "DM_FPS_TARGET"]
// Render scale goes down at once; above the scale the game started with only from the next start (its render buffers were sized at boot)
private let liveNotes: [String: String] = ["DM_RENDER_SCALE": "Lower takes effect now; higher than at start takes effect next time the game starts"]
// The same value written two ways ("1.0" and "1.00" from a slider) is not a change
private func same(_ s: Setting, _ a: String, _ b: String) -> Bool { s.kind == "range" ? abs((Double(a) ?? .nan) - (Double(b) ?? .nan)) < 1e-6 : a == b }

private struct InGameSettingsView: View {
    @ObservedObject var m: LauncherModel
    let start: [String: String]
    let resume: () -> Void

    private var pending: [Setting] { m.settings.filter { !liveKeys.contains($0.env) && !same($0, m.values[$0.env] ?? $0.def, start[$0.env] ?? $0.def) } }

    var body: some View {
        NavigationStack {
            List {
                ForEach(m.sections, id: \.self) { section in
                    let rows = m.rows(in: section)
                    if !rows.isEmpty {
                        Section {
                            ForEach(rows) { s in
                                VStack(alignment: .leading, spacing: 2) {
                                    SettingRow(s: s, m: m)
                                    if let note = liveNotes[s.env] {
                                        Text(note).font(.caption2).foregroundStyle(.secondary)
                                    } else if !liveKeys.contains(s.env) {
                                        Text("Takes effect next time the game starts").font(.caption2).foregroundStyle(.secondary)
                                    }
                                }
                            }
                        } header: { Text(section) }
                    }
                }
                if !pending.isEmpty {
                    Section {
                        Text("\(pending.map(\.label).joined(separator: ", ")) will change the next time the game starts.")
                            .font(.footnote).foregroundStyle(.secondary)
                    }
                }
            }
            .listStyle(.insetGrouped)
            .navigationTitle("Settings")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar { ToolbarItem(placement: .confirmationAction) { Button("Resume", action: resume).fontWeight(.semibold) } }
        }
        .preferredColorScheme(.dark)
    }
}

private var g_open = false

// Shows the sheet over the game; each change is applied when it closes (Resume, or swiped away)
@_cdecl("dm_show_ingame_settings")
public func dmShowInGameSettings(_ viewController: UnsafeMutableRawPointer) {
    guard !g_open, let url = Bundle.main.resourceURL?.appendingPathComponent("dm/launcher.json"), let data = try? Data(contentsOf: url),
          let file = try? JSONDecoder().decode(SettingsFile.self, from: data), !file.settings.isEmpty else { return }
    let parent = Unmanaged<UIViewController>.fromOpaque(viewController).takeUnretainedValue()
    let model = LauncherModel(file.settings)
    // What applies now: a live setting as last changed in this run (dm_live_getenv), any other as saved for the next start, else as this run started
    for s in file.settings where !model.fixed.contains(s.env) {
        if liveKeys.contains(s.env), let v = dm_live_getenv(s.env).map({ String(cString: $0) }) { model.values[s.env] = v; continue }
        if !model.ignoreSaved, let saved = UserDefaults.standard.object(forKey: "launcher." + s.env) as? String, s.accepts(saved) { model.values[s.env] = saved; continue }
        if let v = getenv(s.env).map({ String(cString: $0) }) { model.values[s.env] = v }
    }
    let start = model.values
    var host: UIViewController?
    let finish = {
        g_open = false
        for s in file.settings where !model.fixed.contains(s.env) {
            let v = model.values[s.env] ?? s.def
            guard !same(s, v, start[s.env] ?? s.def) else { continue }
            if !model.ignoreSaved { if s.isDefault(v) { UserDefaults.standard.removeObject(forKey: "launcher." + s.env) } else { UserDefaults.standard.set(v, forKey: "launcher." + s.env) } }
            if liveKeys.contains(s.env) {
                dm_live_set(s.env, v)
                if s.env == "DM_SHOW_FPS" { dm_front_set_fps(Int32(v) ?? 0) }
                fputs("in-game settings: \(s.env)=\(v) (now)\n", stderr)
            } else {
                fputs("in-game settings: \(s.env)=\(v) (next start)\n", stderr)
            }
        }
    }
    let view = InGameSettingsView(m: model, start: start) { SheetDismiss.shared.onDismiss = nil; host?.dismiss(animated: true); host = nil; finish() }
    let h = UIHostingController(rootView: view)
    h.modalPresentationStyle = .formSheet
    h.presentationController?.delegate = SheetDismiss.shared
    SheetDismiss.shared.onDismiss = { host = nil; finish() }
    host = h; g_open = true
    parent.present(h, animated: true)
    fputs("in-game settings: shown\n", stderr)
}

private final class SheetDismiss: NSObject, UIAdaptivePresentationControllerDelegate {
    static let shared = SheetDismiss()
    var onDismiss: (() -> Void)?
    func presentationControllerDidDismiss(_ presentationController: UIPresentationController) { onDismiss?(); onDismiss = nil }
}
