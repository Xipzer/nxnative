"""The binding model (dm/host/keymap.h), compiled with the system's cc: its text form reads back what it writes, the base map is the key map the fronts always had, the Mac's
virtual key codes land on the right HID usages, held keys release what they pressed, and every profile's key_binds reads without a bad token; game.py and settings.py pass
the profile's bindings on (DM_GAME_KEY_BINDS, launcher.json's key_binds). No game files needed."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / 'dm/games'))
import game   # noqa: E402

PROGRAM = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define KM_API
#include "keymap.h"
static km_map base(void) { km_map m; memset(&m, 0, sizeof m); assert(!km_parse(&m, KM_BASE, NULL, 0)); return m; }
int main(int argc, char **argv) {
    if (argc > 1) {   // each argument: a profile's key_binds, which must read without a bad token
        for (int i = 1; i < argc; i++) { km_map m = base(); char bad[64]; if (km_parse(&m, argv[i], bad, sizeof bad)) { printf("bad %s\n", bad); return 1; } }
        return 0;
    }
    km_map m = base();
    // the key map of old (front_mac.m key_bit, DmGamepad.m kb_bit)
    assert(m.out[0x1A] == KM_LS_UP && m.out[0x04] == KM_LS_LEFT && m.out[0x16] == KM_LS_DOWN && m.out[0x07] == KM_LS_RIGHT);
    assert(m.out[0x0D] == KM_BTN + 0 && m.out[0x0E] == KM_BTN + 1 && m.out[0x18] == KM_BTN + 2 && m.out[0x0C] == KM_BTN + 3);
    assert(m.out[0x09] == KM_BTN + 4 && m.out[0x0A] == KM_BTN + 5 && m.out[0x14] == KM_BTN + 6 && m.out[0x08] == KM_BTN + 7 && m.out[0x1D] == KM_BTN + 8 && m.out[0x06] == KM_BTN + 9);
    assert(m.out[0x28] == KM_BTN + 10 && m.out[0x58] == KM_BTN + 10 && m.out[0x2B] == KM_BTN + 11 && m.out[0x2C] == KM_SHAKE);
    assert(m.out[0x50] == KM_BTN + 12 && m.out[0x52] == KM_BTN + 13 && m.out[0x4F] == KM_BTN + 14 && m.out[0x51] == KM_BTN + 15);
    for (int i = 0; i < KM_PAD_N; i++) assert(m.out[KM_PAD + i] == KM_BTN + i);   // a controller as the Switch's
    // round trip: the full map, and overrides against the base
    char t[8192], u[8192];
    assert(km_write(&m, NULL, t, sizeof t) > 0);
    km_map r; memset(&r, 0, sizeof r); assert(!km_parse(&r, t, NULL, 0)); assert(!memcmp(&r, &m, sizeof m));
    km_map p = m; assert(!km_parse(&p, "kb:leftshift=b, KB:Space=x kb:h=SHAKE mouse:right=+ pad:FaceTop=RS_UP kb:0x2c=X kb:Q=- kb:E=none", NULL, 0));
    assert(km_write(&p, &m, u, sizeof u) > 0);
    assert(!strcmp(u, "kb:E=NONE kb:H=SHAKE kb:Q=NONE kb:Space=X kb:LeftShift=B mouse:right=PLUS pad:FaceTop=RS_UP"));
    km_map q = m; assert(!km_parse(&q, u, NULL, 0)); assert(!memcmp(&q, &p, sizeof p));
    assert(!strcmp(dm_km_diff(dm_km_apply(KM_BASE, u), KM_BASE), u));
    char bad[64]; km_map b = m; assert(km_parse(&b, "kb:Nope=A kb:W=JUMP mouse:left kb:X=Y", bad, sizeof bad) == 3 && !strcmp(bad, "kb:Nope=A") && b.out[0x1B] == KM_BTN + 3);
    assert(km_write(&m, NULL, t, 8) == -1);   // too small: no overflow, a failure
    // labels and the Mac's key codes
    assert(!strcmp(dm_km_label("kb:LeftShift"), "⇧ Shift") && !strcmp(dm_km_label("mouse:left"), "Left click") && !strcmp(dm_km_label("kb:w"), "W"));
    assert(!strcmp(dm_km_label("pad:FaceBottom"), "Bottom button") && !strcmp(dm_km_label("kb:KP1"), "Num 1") && !strcmp(dm_km_label("nonsense"), "nonsense"));
    assert(!strcmp(dm_km_mac_key(13), "kb:W") && !strcmp(dm_km_mac_key(56), "kb:LeftShift") && !strcmp(dm_km_mac_key(49), "kb:Space") && !strcmp(dm_km_mac_key(36), "kb:Return"));
    assert(!strcmp(dm_km_mac_key(123), "kb:Left") && !strcmp(dm_km_mac_key(29), "kb:0") && !strcmp(dm_km_mac_key(122), "kb:F1") && !dm_km_mac_key(52) && !dm_km_mac_key(200));
    assert(!strcmp(dm_km_hid_key(0x2C), "kb:Space") && !strcmp(dm_km_pad(2), "pad:FaceTop"));
    for (int vk = 0; vk < 128; vk++) if (KM_MAC_VK[vk]) { const char *k = dm_km_mac_key(vk); assert(km_input(k, strlen(k)) == KM_MAC_VK[vk]); }   // every one reads back
    for (int u = 1; u < 256; u++) { char n[32]; km_input_text(u, 0, n, sizeof n); assert(km_input(n, strlen(n)) == u); }
    // held keys: a change of the map while one is held releases what it pressed; Shift + an arrow is the right stick only while Shift is unbound
    km_held h; memset(&h, 0, sizeof h); int32_t ax[4];
    assert(km_press(&h, &m, 0x1A, 1) && !km_press(&h, &m, 0x1A, 1)); km_sticks(h.held, ax); assert(ax[1] == DM_STICK_MAX && !ax[0]);
    km_map n = m; n.out[0x1A] = KM_BTN; assert(km_press(&h, &n, 0x1A, 0)); km_sticks(h.held, ax); assert(!ax[1] && !km_buttons(h.held));
    assert(km_shift_arrow(&m, 0x52) == KM_RS_UP && !km_shift_arrow(&p, 0x52) && !km_shift_arrow(&m, 0x1A));
    assert(km_press(&h, &m, 0x2C, 1) && km_press(&h, &m, KM_MOUSE + 0, 1)); assert(km_buttons(h.held) == (DM_BTN_SHAKE | DM_BTN_Y));
    km_release(&h, KM_MOUSE, KM_PAD); assert(km_buttons(h.held) == DM_BTN_SHAKE);
    // a controller: bound elements, and a stick direction over the pad's own axis
    int32_t pa[4] = {100, -200, 0, 0};
    assert(km_pad(&m, DM_BTN_A | DM_BTN_DOWN, pa) == (DM_BTN_A | DM_BTN_DOWN) && pa[0] == 100 && pa[1] == -200);
    assert(km_pad(&p, DM_BTN_X, pa) == 0 && pa[3] == DM_STICK_MAX && pa[0] == 100);
    uint64_t mb[3] = {DM_BTN_ZR, 0, DM_BTN_SHAKE}; km_map g; km_game(&g, mb, "kb:LeftShift=B");
    assert(g.out[KM_MOUSE] == KM_BTN + 9 && g.out[KM_MOUSE + 1] == KM_NONE && g.out[KM_MOUSE + 2] == KM_SHAKE && g.out[0xE1] == KM_BTN + 1);
    puts("ok");
    return 0;
}
'''


class Keymap(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        src, cls.exe = Path(cls.tmp.name) / 't.c', Path(cls.tmp.name) / 't'
        src.write_text(PROGRAM)
        subprocess.run(['cc', '-std=gnu11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-I', str(REPO / 'dm/host'), str(src), '-o', str(cls.exe)], check=True)

    @classmethod
    def tearDownClass(cls): cls.tmp.cleanup()

    def test_model(self):
        self.assertEqual(subprocess.run([str(self.exe)], capture_output=True, text=True, check=True).stdout, 'ok\n')

    def test_every_profiles_bindings_read(self):
        binds = {g.id: game.key_binds_text(g) for g in game.all_games()}
        p = subprocess.run([str(self.exe), *binds.values()], capture_output=True, text=True)
        self.assertEqual((p.returncode, p.stdout), (0, ''), binds)

    def test_breath_of_the_wild_and_tears_of_the_kingdom(self):   # Shift sprints, Space jumps, the mouse attacks and locks on, shaking on H
        for gid in ('01007EF00011E000', '0100F2C0115B6000'):
            with self.subTest(gid=gid):
                g = game.load(gid)
                self.assertIn('#define DM_GAME_KEY_BINDS "kb:LeftShift=B kb:Space=X kb:H=SHAKE"', game.header_one(g))
                self.assertEqual(game.key_binds_text(g), 'mouse:left=Y mouse:right=ZL mouse:middle=R3 kb:LeftShift=B kb:Space=X kb:H=SHAKE')

    def test_profile_without_bindings_and_a_malformed_one(self):
        g = game.load('010015100B514000')
        self.assertIn('#define DM_GAME_KEY_BINDS ""', game.header_one(g))
        g.rows = g.rows + [('key_binds', ['kb:Space X'])]
        with self.assertRaisesRegex(SystemExit, 'key_binds'): game.header_one(g)

    def test_launcher_list_carries_the_games_bindings(self):
        spec = importlib.util.spec_from_file_location('launcher_settings', REPO / 'dm/games/settings.py')
        settings = importlib.util.module_from_spec(spec); spec.loader.exec_module(settings)
        with tempfile.TemporaryDirectory() as tmp, patch.object(settings.nacp, 'languages', return_value=['en-US']), patch.dict(os.environ, {'GFXAPI': 'nvn'}):
            listed = settings.build('01007EF00011E000', tmp, Path(tmp) / 'control.nacp')
        self.assertEqual(listed['key_binds'], 'mouse:left=Y mouse:right=ZL mouse:middle=R3 kb:LeftShift=B kb:Space=X kb:H=SHAKE')


if __name__ == '__main__':
    unittest.main()
