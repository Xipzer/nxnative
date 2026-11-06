// Key, mouse and controller bindings, shared by the Mac front end (front_mac.m), the iOS gamepad (DmGamepad.m) and the Controls screens that edit them (ControlsView.swift, through
// keymap_api.h). A map sends each physical input to one Switch output: a button, a stick direction or shaking the controller (DM_BTN_SHAKE).
//   Inputs: keyboard keys by USB HID usage (GCKeyCode values are HID usages; the Mac's virtual key codes go through KM_MAC_VK), the mouse's left, right and middle buttons (with
//   Mouse: Camera, mouse_look.h), and a controller's buttons, the face buttons by position as pad_apple.h reads them (FaceRight is the Switch's A, whatever the pad prints on it).
//   Text form (the profile's key_binds, DM_KEYMAP, the Controls screens' store), <input>=<output> tokens separated by spaces:
//     input   kb:<key> (kb:W, kb:LeftShift, kb:Space, kb:Return, kb:0x2c), mouse:left | right | middle, pad:<element> (KM_PAD_NAMES)
//     output  A B X Y L R ZL ZR PLUS MINUS L3 R3 LEFT UP RIGHT DOWN (+ is PLUS), LS_UP LS_DOWN LS_LEFT LS_RIGHT, RS_UP RS_DOWN RS_LEFT RS_RIGHT, SHAKE, or NONE (- too): unbound
//   Names are not case-sensitive. Layers, each over the one before: KM_BASE (the key map the fronts always had), the mouse's buttons (ml_buttons: DM_MOUSE_BUTTONS, else the
//   profile's mouse_buttons), the profile's key_binds (DM_GAME_KEY_BINDS), the player's DM_KEYMAP. An input has one output; an output may have several inputs.
//   Reserved: Esc frees a captured pointer, keys pressed with ⌘ held are the system's; the Controls screens do not bind them.
//   Shift + an arrow key is the right stick while that arrow is its D-pad direction and no Shift key is bound (km_shift_arrow), as before the map existed.
#pragma once
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "dm_input.h"

// Outputs: KM_BTN + n is the Switch button bit n (dm_input.h: A0 B1 X2 Y3 L3 4 R3 5 L6 R7 ZL8 ZR9 +10 −11 ←12 ↑13 →14 ↓15)
enum { KM_NONE = 0, KM_BTN = 1, KM_LS_UP = 17, KM_LS_DOWN, KM_LS_LEFT, KM_LS_RIGHT, KM_RS_UP, KM_RS_DOWN, KM_RS_LEFT, KM_RS_RIGHT, KM_SHAKE, KM_NOUT };
// Inputs, one flat index: keys 0..255 (their HID usage), then the mouse's buttons, then the controller's elements (bit i of a pad's elements is KM_PAD + i, the Switch button bit i by default)
enum { KM_KB = 0, KM_MOUSE = 256, KM_MOUSE_N = 3, KM_PAD = KM_MOUSE + KM_MOUSE_N, KM_PAD_N = 16, KM_NIN = KM_PAD + KM_PAD_N };
typedef struct { uint8_t out[KM_NIN]; } km_map;

static const char *const KM_OUT_NAMES[KM_NOUT] = {"NONE", "A", "B", "X", "Y", "L3", "R3", "L", "R", "ZL", "ZR", "PLUS", "MINUS", "LEFT", "UP", "RIGHT", "DOWN",
    "LS_UP", "LS_DOWN", "LS_LEFT", "LS_RIGHT", "RS_UP", "RS_DOWN", "RS_LEFT", "RS_RIGHT", "SHAKE"};
static const char *const KM_MOUSE_NAMES[KM_MOUSE_N] = {"left", "right", "middle"};
static const char *const KM_MOUSE_LABELS[KM_MOUSE_N] = {"Left click", "Right click", "Middle click"};
static const char *const KM_PAD_NAMES[KM_PAD_N] = {"FaceRight", "FaceBottom", "FaceTop", "FaceLeft", "LeftStick", "RightStick", "LeftShoulder", "RightShoulder",
    "LeftTrigger", "RightTrigger", "Menu", "Options", "DpadLeft", "DpadUp", "DpadRight", "DpadDown"};
static const char *const KM_PAD_LABELS[KM_PAD_N] = {"Right button", "Bottom button", "Top button", "Left button", "Left stick press", "Right stick press", "Left shoulder", "Right shoulder",
    "Left trigger", "Right trigger", "Menu", "Options", "D-pad ←", "D-pad ↑", "D-pad →", "D-pad ↓"};

// Keys other than letters, digits, F1..F24 and the keypad's digits (km_key_name makes those)
static const struct { uint8_t u; const char *name, *label; } KM_KEYS[] = {
    {0x28, "Return", "Return"}, {0x29, "Escape", "Esc"}, {0x2A, "Backspace", "Delete"}, {0x2B, "Tab", "Tab"}, {0x2C, "Space", "Space"}, {0x2D, "Minus", "-"}, {0x2E, "Equal", "="},
    {0x2F, "LeftBracket", "["}, {0x30, "RightBracket", "]"}, {0x31, "Backslash", "\\"}, {0x32, "NonUSPound", "#"}, {0x33, "Semicolon", ";"}, {0x34, "Quote", "'"}, {0x35, "Grave", "`"},
    {0x36, "Comma", ","}, {0x37, "Period", "."}, {0x38, "Slash", "/"}, {0x39, "CapsLock", "Caps Lock"}, {0x46, "PrintScreen", "Print Screen"}, {0x47, "ScrollLock", "Scroll Lock"},
    {0x48, "Pause", "Pause"}, {0x49, "Insert", "Insert"}, {0x4A, "Home", "Home"}, {0x4B, "PageUp", "Page Up"}, {0x4C, "Delete", "Forward Delete"}, {0x4D, "End", "End"},
    {0x4E, "PageDown", "Page Down"}, {0x4F, "Right", "→"}, {0x50, "Left", "←"}, {0x51, "Down", "↓"}, {0x52, "Up", "↑"}, {0x53, "NumLock", "Clear"}, {0x54, "KPDivide", "Num /"},
    {0x55, "KPMultiply", "Num *"}, {0x56, "KPMinus", "Num -"}, {0x57, "KPPlus", "Num +"}, {0x58, "KPEnter", "Enter"}, {0x63, "KPPeriod", "Num ."}, {0x64, "NonUSBackslash", "§"},
    {0x65, "Application", "Menu"}, {0x67, "KPEqual", "Num ="}, {0xE0, "LeftControl", "⌃ Control"}, {0xE1, "LeftShift", "⇧ Shift"}, {0xE2, "LeftAlt", "⌥ Option"},
    {0xE3, "LeftGUI", "⌘ Command"}, {0xE4, "RightControl", "Right ⌃"}, {0xE5, "RightShift", "Right ⇧"}, {0xE6, "RightAlt", "Right ⌥"}, {0xE7, "RightGUI", "Right ⌘"},
};
enum { KM_ESCAPE = 0x29, KM_GRAVE = 0x35, KM_LSHIFT = 0xE1, KM_RSHIFT = 0xE5 };

// The Mac's virtual key codes (Carbon's kVK_*, NSEvent.keyCode) as HID usages; 0: none
static const uint8_t KM_MAC_VK[128] = {
    [0] = 0x04, [1] = 0x16, [2] = 0x07, [3] = 0x09, [4] = 0x0B, [5] = 0x0A, [6] = 0x1D, [7] = 0x1B, [8] = 0x06, [9] = 0x19, [10] = 0x64, [11] = 0x05, [12] = 0x14, [13] = 0x1A,
    [14] = 0x08, [15] = 0x15, [16] = 0x1C, [17] = 0x17, [18] = 0x1E, [19] = 0x1F, [20] = 0x20, [21] = 0x21, [22] = 0x23, [23] = 0x22, [24] = 0x2E, [25] = 0x26, [26] = 0x24,
    [27] = 0x2D, [28] = 0x25, [29] = 0x27, [30] = 0x30, [31] = 0x12, [32] = 0x18, [33] = 0x2F, [34] = 0x0C, [35] = 0x13, [36] = 0x28, [37] = 0x0F, [38] = 0x0D, [39] = 0x34,
    [40] = 0x0E, [41] = 0x33, [42] = 0x31, [43] = 0x36, [44] = 0x38, [45] = 0x11, [46] = 0x10, [47] = 0x37, [48] = 0x2B, [49] = 0x2C, [50] = 0x35, [51] = 0x2A, [53] = 0x29,
    [54] = 0xE7, [55] = 0xE3, [56] = 0xE1, [57] = 0x39, [58] = 0xE2, [59] = 0xE0, [60] = 0xE5, [61] = 0xE6, [62] = 0xE4, [64] = 0x6C, [65] = 0x63, [67] = 0x55, [69] = 0x57,
    [71] = 0x53, [75] = 0x54, [76] = 0x58, [78] = 0x56, [79] = 0x6D, [80] = 0x6E, [81] = 0x67, [82] = 0x62, [83] = 0x59, [84] = 0x5A, [85] = 0x5B, [86] = 0x5C, [87] = 0x5D,
    [88] = 0x5E, [89] = 0x5F, [90] = 0x6F, [91] = 0x60, [92] = 0x61, [96] = 0x3E, [97] = 0x3F, [98] = 0x40, [99] = 0x3C, [100] = 0x41, [101] = 0x42, [103] = 0x44, [105] = 0x68,
    [106] = 0x6B, [107] = 0x69, [109] = 0x43, [111] = 0x45, [113] = 0x6A, [114] = 0x49, [115] = 0x4A, [116] = 0x4B, [117] = 0x4C, [118] = 0x3D, [119] = 0x4D, [120] = 0x3B,
    [121] = 0x4E, [122] = 0x3A, [123] = 0x50, [124] = 0x4F, [125] = 0x51, [126] = 0x52,
};

// The base map: WASD the left stick, the arrow keys the D-pad, J K U I = A B X Y, Q E = L R, Z C = ZL ZR, F G = L3 R3, Return = +, Tab = −, Space shakes the controller;
// the mouse Y ZL R3 (Breath of the Wild's attack, lock on, scope; the profile's mouse_buttons replaces them); a controller's buttons as the Switch's
#define KM_BASE "kb:W=LS_UP kb:A=LS_LEFT kb:S=LS_DOWN kb:D=LS_RIGHT kb:Left=LEFT kb:Up=UP kb:Right=RIGHT kb:Down=DOWN kb:J=A kb:K=B kb:U=X kb:I=Y kb:Q=L kb:E=R " \
    "kb:Z=ZL kb:C=ZR kb:F=L3 kb:G=R3 kb:Return=PLUS kb:KPEnter=PLUS kb:Tab=MINUS kb:Space=SHAKE mouse:left=Y mouse:right=ZL mouse:middle=R3 " \
    "pad:FaceRight=A pad:FaceBottom=B pad:FaceTop=X pad:FaceLeft=Y pad:LeftStick=L3 pad:RightStick=R3 pad:LeftShoulder=L pad:RightShoulder=R pad:LeftTrigger=ZL " \
    "pad:RightTrigger=ZR pad:Menu=PLUS pad:Options=MINUS pad:DpadLeft=LEFT pad:DpadUp=UP pad:DpadRight=RIGHT pad:DpadDown=DOWN"

// A key's name (as the text form writes it) and its label (for a key cap on screen); a usage without a name is 0x<hex>
static inline void km_key_text(int u, int label, char *out, size_t n) {
    if (u >= 0x04 && u <= 0x1D) { snprintf(out, n, "%c", 'A' + u - 0x04); return; }
    if (u >= 0x1E && u <= 0x27) { snprintf(out, n, "%d", (u - 0x1E + 1) % 10); return; }
    if (u >= 0x3A && u <= 0x45) { snprintf(out, n, "F%d", u - 0x3A + 1); return; }
    if (u >= 0x68 && u <= 0x73) { snprintf(out, n, "F%d", u - 0x68 + 13); return; }
    if (u >= 0x59 && u <= 0x62) { snprintf(out, n, label ? "Num %d" : "KP%d", (u - 0x59 + 1) % 10); return; }
    for (size_t i = 0; i < sizeof KM_KEYS / sizeof *KM_KEYS; i++) if (KM_KEYS[i].u == u) { snprintf(out, n, "%s", label ? KM_KEYS[i].label : KM_KEYS[i].name); return; }
    snprintf(out, n, "0x%02x", u);
}
// An input as a token (kb:W) or as a label for the screen (W, Left click, Bottom button)
static inline void km_input_text(int in, int label, char *out, size_t n) {
    if (in < KM_MOUSE) { char k[32]; km_key_text(in, label, k, sizeof k); snprintf(out, n, "%s%s", label ? "" : "kb:", k); }
    else if (in < KM_PAD) snprintf(out, n, "%s%s", label ? "" : "mouse:", (label ? KM_MOUSE_LABELS : KM_MOUSE_NAMES)[in - KM_MOUSE]);
    else if (in < KM_NIN) snprintf(out, n, "%s%s", label ? "" : "pad:", (label ? KM_PAD_LABELS : KM_PAD_NAMES)[in - KM_PAD]);
    else snprintf(out, n, "?");
}
static inline int km_input(const char *s, size_t len) {   // a token's input, -1 if none
    char t[48]; if (len >= sizeof t) return -1;
    memcpy(t, s, len); t[len] = 0;
    if (!strncasecmp(t, "kb:", 3)) {
        const char *k = t + 3; char name[32];
        if (!strncasecmp(k, "0x", 2)) { char *end; long u = strtol(k + 2, &end, 16); return *end || end == k + 2 || u < 0 || u > 255 ? -1 : (int)u; }
        for (int u = 0; u < 256; u++) { km_key_text(u, 0, name, sizeof name); if (!strncmp(name, "0x", 2)) continue; if (!strcasecmp(name, k)) return u; }
        return -1;
    }
    if (!strncasecmp(t, "mouse:", 6)) { for (int i = 0; i < KM_MOUSE_N; i++) if (!strcasecmp(t + 6, KM_MOUSE_NAMES[i])) return KM_MOUSE + i; return -1; }
    if (!strncasecmp(t, "pad:", 4)) { for (int i = 0; i < KM_PAD_N; i++) if (!strcasecmp(t + 4, KM_PAD_NAMES[i])) return KM_PAD + i; return -1; }
    return -1;
}
static inline int km_output(const char *s, size_t len) {   // a token's output, -1 if none
    char t[16]; if (len >= sizeof t) return -1;
    memcpy(t, s, len); t[len] = 0;
    if (!strcmp(t, "+")) return KM_BTN + 10;
    if (!strcmp(t, "-")) return KM_NONE;
    for (int o = 0; o < KM_NOUT; o++) if (!strcasecmp(t, KM_OUT_NAMES[o])) return o;
    return -1;
}
// Applies text over m (separated by spaces or commas). Returns how many tokens it could not read, the first of them in bad (may be NULL); the others still apply
static inline int km_parse(km_map *m, const char *text, char *bad, size_t nbad) {
    int nb = 0;
    if (bad && nbad) *bad = 0;
    for (const char *p = text ? text : ""; *p;) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        const char *s = p;
        while (*p && !isspace((unsigned char)*p) && *p != ',') p++;
        if (p == s) break;
        const char *eq = memchr(s, '=', (size_t)(p - s));
        int in = eq ? km_input(s, (size_t)(eq - s)) : -1, out = eq ? km_output(eq + 1, (size_t)(p - eq - 1)) : -1;
        if (in < 0 || out < 0) { if (!nb++ && bad && nbad) snprintf(bad, nbad, "%.*s", (int)(p - s), s); continue; }
        m->out[in] = (uint8_t)out;
    }
    return nb;
}
// Writes m as text: with base, only the inputs whose output differs from base's (NONE for one m unbinds); without, every bound input. Returns the length, -1 when n is too small
static inline int km_write(const km_map *m, const km_map *base, char *out, size_t n) {
    size_t len = 0;
    if (n) *out = 0;
    for (int in = 0; in < KM_NIN; in++) {
        int o = m->out[in];
        if (base ? o == base->out[in] : !o) continue;
        char t[48]; km_input_text(in, 0, t, sizeof t);
        len += (size_t)snprintf(len < n ? out + len : NULL, len < n ? n - len : 0, "%s%s=%s", len ? " " : "", t, KM_OUT_NAMES[o < KM_NOUT ? o : 0]);
    }
    return len < n ? (int)len : -1;
}
static inline int km_bit_out(uint64_t bit) { return !bit ? KM_NONE : bit == DM_BTN_SHAKE ? KM_SHAKE : KM_BTN + __builtin_ctzll(bit); }   // a DM_BTN_* bit as an output
// The map a game starts with: the base, the mouse's buttons (ml_buttons' bits: left, right, middle), the profile's key_binds. The player's DM_KEYMAP goes over it (km_parse)
static inline void km_game(km_map *m, const uint64_t mouse_bits[3], const char *key_binds) {
    memset(m, 0, sizeof *m); km_parse(m, KM_BASE, NULL, 0);
    for (int i = 0; i < KM_MOUSE_N; i++) m->out[KM_MOUSE + i] = (uint8_t)km_bit_out(mouse_bits[i]);
    km_parse(m, key_binds, NULL, 0);
}
// Shift + an arrow key: the right stick's direction while the arrow is its D-pad direction and neither Shift key is bound; 0 otherwise
static inline int km_shift_arrow(const km_map *m, int u) {
    static const uint8_t arrows[4][3] = {{0x50, KM_BTN + 12, KM_RS_LEFT}, {0x52, KM_BTN + 13, KM_RS_UP}, {0x4F, KM_BTN + 14, KM_RS_RIGHT}, {0x51, KM_BTN + 15, KM_RS_DOWN}};
    if (u < 0 || u >= KM_MOUSE || m->out[KM_LSHIFT] || m->out[KM_RSHIFT]) return 0;
    for (int i = 0; i < 4; i++) if (u == arrows[i][0] && m->out[u] == arrows[i][1]) return arrows[i][2];
    return 0;
}

// What is held: the output each pressed input pressed (so a change of the map while it is held releases the right one), and how many inputs hold each output
typedef struct { uint8_t on[KM_NIN]; uint16_t held[KM_NOUT]; } km_held;
static inline int km_press_as(km_held *h, int in, int out, int down) {   // returns whether anything changed
    if (in < 0 || in >= KM_NIN) return 0;
    if (down) { if (h->on[in] || out <= KM_NONE || out >= KM_NOUT) return 0; h->on[in] = (uint8_t)out; h->held[out]++; return 1; }
    int o = h->on[in]; if (!o) return 0;
    h->on[in] = 0; h->held[o]--; return 1;
}
static inline int km_press(km_held *h, const km_map *m, int in, int down) { return km_press_as(h, in, down && in >= 0 && in < KM_NIN ? m->out[in] : 0, down); }
static inline void km_release(km_held *h, int from, int to) { for (int in = from; in < to; in++) km_press_as(h, in, 0, 0); }   // inputs from..to-1 let go
static inline uint64_t km_buttons(const uint16_t held[KM_NOUT]) {
    uint64_t b = held[KM_SHAKE] ? DM_BTN_SHAKE : 0;
    for (int i = 0; i < 16; i++) if (held[KM_BTN + i]) b |= 1ull << i;
    return b;
}
static inline void km_sticks(const uint16_t held[KM_NOUT], int32_t ax[4]) {   // lx ly rx ry, full deflection, up positive
    ax[0] = ((held[KM_LS_RIGHT] > 0) - (held[KM_LS_LEFT] > 0)) * DM_STICK_MAX; ax[1] = ((held[KM_LS_UP] > 0) - (held[KM_LS_DOWN] > 0)) * DM_STICK_MAX;
    ax[2] = ((held[KM_RS_RIGHT] > 0) - (held[KM_RS_LEFT] > 0)) * DM_STICK_MAX; ax[3] = ((held[KM_RS_UP] > 0) - (held[KM_RS_DOWN] > 0)) * DM_STICK_MAX;
}
// A controller's pressed elements (bit i: KM_PAD + i) through the map: the Switch buttons, and in ax (the pad's own sticks on entry) the stick directions bound elements hold, which take
// the axis over the pad's own. A controller reports its whole state each time, so nothing is kept
static inline uint64_t km_pad(const km_map *m, uint32_t elems, int32_t ax[4]) {
    uint16_t held[KM_NOUT] = {0};
    for (int i = 0; i < KM_PAD_N; i++) if (elems >> i & 1) held[m->out[KM_PAD + i]]++;
    int32_t d[4]; km_sticks(held, d);
    for (int k = 0; k < 4; k++) if (d[k]) ax[k] = d[k];
    return km_buttons(held);
}

// The Controls screens' calls (keymap_api.h): one file of each program that has a screen defines KM_API before including this header (DmGamepad.m, dm/mac/keymap_api.c)
#ifdef KM_API
#include "keymap_api.h"
static char km_api_buf[8192];
const char *dm_km_base(void) { return KM_BASE; }
const char *dm_km_apply(const char *full, const char *overlay) {
    km_map m; memset(&m, 0, sizeof m); km_parse(&m, full, NULL, 0); km_parse(&m, overlay, NULL, 0);
    if (km_write(&m, NULL, km_api_buf, sizeof km_api_buf) < 0) *km_api_buf = 0;
    return km_api_buf;
}
const char *dm_km_diff(const char *full, const char *base) {
    km_map m, b; memset(&m, 0, sizeof m); memset(&b, 0, sizeof b); km_parse(&m, full, NULL, 0); km_parse(&b, base, NULL, 0);
    if (km_write(&m, &b, km_api_buf, sizeof km_api_buf) < 0) *km_api_buf = 0;
    return km_api_buf;
}
const char *dm_km_label(const char *token) {
    int in = token ? km_input(token, strlen(token)) : -1;
    if (in < 0) snprintf(km_api_buf, sizeof km_api_buf, "%s", token ? token : "");
    else km_input_text(in, 1, km_api_buf, sizeof km_api_buf);
    return km_api_buf;
}
const char *dm_km_hid_key(int usage) {
    if (usage <= 0 || usage >= KM_MOUSE) return NULL;
    km_input_text(usage, 0, km_api_buf, sizeof km_api_buf); return km_api_buf;
}
const char *dm_km_mac_key(int vk) { return vk >= 0 && vk < 128 && KM_MAC_VK[vk] ? dm_km_hid_key(KM_MAC_VK[vk]) : NULL; }
const char *dm_km_pad(int element) {
    if (element < 0 || element >= KM_PAD_N) return NULL;
    km_input_text(KM_PAD + element, 0, km_api_buf, sizeof km_api_buf); return km_api_buf;
}
#endif
