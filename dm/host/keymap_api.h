// The binding model (keymap.h) as the Controls screens (ControlsView.swift) call it: the iOS app through its bridging header (defined in DmGamepad.m), the Mac app's settings window
// through dm/mac/LauncherBridge.h (defined in dm/mac/keymap_api.c). Each returns a string the next call replaces (main thread); NULL where there is none.
// A full map's text lists every bound input (km_write without a base); overrides list what differs from a base, NONE for an input unbound.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
const char *dm_km_base(void);   // KM_BASE: the base map's text
const char *dm_km_apply(const char *full, const char *overlay);   // the full map of full with overlay applied over it, in the canonical form (token names as keymap.h writes them)
const char *dm_km_diff(const char *full, const char *base);   // the overrides that make base into full (what DM_KEYMAP holds)
const char *dm_km_label(const char *token);   // an input's label for a key cap (kb:LeftShift → ⇧ Shift, mouse:left → Left click); an unknown token as it is
const char *dm_km_hid_key(int usage);   // a key's token by HID usage (GCKeyCode, UIKeyboardHIDUsage)
const char *dm_km_mac_key(int vk);   // a key's token by the Mac's virtual key code (NSEvent.keyCode)
const char *dm_km_pad(int element);   // a controller element's token (bit i of km_pad's elements)
#ifdef __cplusplus
}
#endif
