"""Execute the exact-build TOTK camera/viewport veneers without game files.

Optional execution dependency: unicorn (macOS JIT needs an unsandboxed run).
Original bytes below were checked against the extracted 1.4.3 main NSO.
"""
import importlib
import importlib.util
from pathlib import Path
import struct
import sys
import unittest
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
try:
    uc: Any = importlib.import_module('unicorn')
    arm: Any = importlib.import_module('unicorn.arm64_const')
except ImportError:
    uc = arm = None

spec = importlib.util.spec_from_file_location('totk_patches', Path(__file__).resolve().parents[2] / 'games/0100F2C0115B6000/patches.py')
assert spec is not None and spec.loader is not None
patch = importlib.util.module_from_spec(spec); spec.loader.exec_module(patch)
BUILD = '277178b7dba1b6d4949a84778d4abc58'
ORIGINAL = {
    0x628c74: bytes.fromhex('604241bd9522003980b200bd'),
    0xb43460: bytes.fromhex('c06a40bdf8220039e0b200bd'),
    0x1ff04f4: bytes.fromhex('0018211e00b100bd'),
    0x253fbb0: bytes.fromhex('0018211e00b100bd'),
}


# Where 1.4.3 has the six functions the frame step is matched by (patches.py MATCH_FRAME_STEP): the camera sites ride on that stub, for this build alone
MATCHED = {'base': 0x2db174, 'others': {'game step reader 1': 0x2daa20, 'game step reader 2': 0x3e55e0, 'menu fade': 0x1fbb50, 'swapchain argument': 0xbe1304, 'view render size': 0xd5c24}}


class Context:
    state, render_scale = 0x3100000, 0

    def __init__(self, originals=None, build=BUILD):
        self.cursor = 0x3000000
        self.originals = ORIGINAL if originals is None else originals
        self.checked, self.code, self.writes = {}, {}, {}
        self.base, self.others, self.build, self.capabilities, self.keys = MATCHED['base'], dict(MATCHED['others']), build, set(), {}

    def here(self): return self.cursor

    def emit(self, word):
        self.code[self.cursor] = word
        self.cursor += 4

    def site(self, offset, original):
        expected = bytes.fromhex(original)
        self.checked[offset] = expected
        if offset in self.originals:
            assert self.originals[offset] == expected, f'original bytes mismatch {offset:#x}'

    def put(self, offset, words, why): self.writes[offset] = words


class BuildGate(unittest.TestCase):
    def test_registry_advertises_only_verified_camera_build(self):
        self.assertEqual(patch.CAPABILITIES, {BUILD: ('CAMERA_ASPECT',)})
        self.assertFalse(getattr(patch, 'FRAME_STEP', {}))   # the frame step is placed by its functions (MATCH_FRAME_STEP), on this build too
        rule, = patch.MATCH_FRAME_STEP['main']
        self.assertIs(rule['stub'], patch.matched_frame_step); self.assertEqual(set(rule['others']), set(MATCHED['others']))
        ctx = Context()
        patch.matched_frame_step(ctx)
        for site, original in ORIGINAL.items():
            self.assertEqual(ctx.checked[site], original)
            self.assertEqual(len(ctx.writes[site]) * 4, len(original))
            self.assertEqual(ctx.writes[site][0] & 0xfc000000, 0x14000000)
            self.assertTrue(all(word == 0xd503201f for word in ctx.writes[site][1:]))
        timing = set(ctx.writes) - set(ORIGINAL)
        self.assertEqual(len(timing), 9)   # the matched stub's own sites: the step, two readers with their scvtf, the fade, the swapchain argument, two in the view size

    def test_camera_sites_are_this_builds_alone(self):
        """The camera sites are fixed addresses of 1.4.3: the matched stub writes them on that build only, and its timing sites on any"""
        for build in ('0' * 32, None):
            with self.subTest(build=build):
                ctx = Context(build=build)
                if build is None: del ctx.build   # a dry run that names no build (test_patches.py)
                patch.matched_frame_step(ctx)
                self.assertFalse(set(ctx.writes) & set(ORIGINAL)); self.assertFalse(set(ctx.checked) & set(ORIGINAL))
                self.assertEqual(len(ctx.writes), 9)
        ctx = Context(); ctx.others['menu fade'] = None   # one of the six not found: no timing stub, and the capability's camera sites all the same
        patch.matched_frame_step(ctx)
        self.assertEqual(set(ctx.writes), set(ORIGINAL))

    def test_each_original_word_mismatch_rejected_before_camera_writes(self):
        for site, original in ORIGINAL.items():
            for offset in range(0, len(original), 4):
                with self.subTest(site=hex(site), word=offset // 4):
                    changed = dict(ORIGINAL)
                    damaged = bytearray(original); damaged[offset] ^= 1
                    changed[site] = bytes(damaged)
                    ctx = Context(changed)
                    with self.assertRaisesRegex(AssertionError, 'original bytes mismatch'):
                        patch.camera_aspect(ctx)
                    self.assertFalse(ctx.code)
                    self.assertFalse(ctx.writes)


@unittest.skipIf(uc is None, 'optional ARM64 execution dependency: unicorn')
class ProjectionWriters(unittest.TestCase):
    base, obj, stack = 0x4000000, 0x10000000, 0x20000000

    def fixture(self, site):
        self.u = uc.Uc(uc.UC_ARCH_ARM64, uc.UC_MODE_ARM)
        self.u.mem_map(self.base, 0x4000000)
        self.u.mem_map(self.obj, 0x4000)
        self.u.mem_map(self.stack, 0x4000)
        self.ctx = Context()
        # Exercise the production stub, including its timing/scale hooks.
        patch.matched_frame_step(self.ctx)
        for offset, word in self.ctx.code.items():
            self.u.mem_write(self.base + offset, struct.pack('<I', word))
        for offset, words in self.ctx.writes.items():
            self.u.mem_write(self.base + offset, struct.pack('<' + 'I' * len(words), *words))
        self.site = site
        self.end = self.base + site + len(ORIGINAL[site])
        self.u.mem_write(self.obj, bytes([0xa5]) * 0x4000)
        for i in range(31): self.u.reg_write(getattr(arm, f'UC_ARM64_REG_X{i}'), 0x12340000 + i)
        for i in range(32): self.u.reg_write(getattr(arm, f'UC_ARM64_REG_Q{i}'), (i + 1) * 0x123456789abcdef)
        self.u.reg_write(arm.UC_ARM64_REG_SP, self.stack + 0x3000)
        self.u.reg_write(arm.UC_ARM64_REG_NZCV, 0xa0000000)
        self.u.reg_write(arm.UC_ARM64_REG_FPSR, 0)
        if site == 0x628c74:
            self.u.reg_write(arm.UC_ARM64_REG_X19, self.obj + 0x1000)
            self.u.reg_write(arm.UC_ARM64_REG_X20, self.obj)
            self.u.reg_write(arm.UC_ARM64_REG_W21, 1)
            self.u.mem_write(self.obj + 0x1140, struct.pack('<f', 16 / 9))
        elif site == 0xb43460:
            self.u.reg_write(arm.UC_ARM64_REG_X22, self.obj + 0x1000)
            self.u.reg_write(arm.UC_ARM64_REG_X23, self.obj)
            self.u.reg_write(arm.UC_ARM64_REG_W24, 1)
            self.u.mem_write(self.obj + 0x1068, struct.pack('<f', 16 / 9))
        else:
            self.u.reg_write(arm.UC_ARM64_REG_X8, self.obj)
            self.u.reg_write(arm.UC_ARM64_REG_S0, struct.unpack('<I', struct.pack('<f', 1280))[0])
            self.u.reg_write(arm.UC_ARM64_REG_S1, struct.unpack('<I', struct.pack('<f', 720))[0])

    def value(self, address): return struct.unpack('<f', self.u.mem_read(address, 4))[0]

    def desired(self, ratio):
        self.u.mem_write(self.base + self.ctx.state + 0x320, struct.pack('<f', ratio))

    def observable_registers(self):
        regs = [getattr(arm, f'UC_ARM64_REG_X{i}') for i in range(31)]
        regs += [getattr(arm, f'UC_ARM64_REG_Q{i}') for i in range(32)]
        regs += [arm.UC_ARM64_REG_SP, arm.UC_ARM64_REG_NZCV, arm.UC_ARM64_REG_FPSR]
        return [self.u.reg_read(reg) for reg in regs]

    def execute(self):
        self.u.emu_start(self.base + self.site, self.end, count=100)
        self.assertEqual(self.u.reg_read(arm.UC_ARM64_REG_PC), self.end)

    def test_disabled_matches_original_fields_registers_and_flags(self):
        for site, original in ORIGINAL.items():
            with self.subTest(site=hex(site)):
                self.fixture(site); self.desired(0)
                initial = self.u.context_save()
                image = bytes(self.u.mem_read(self.obj, 0x4000))
                self.execute()
                patched_regs = self.observable_registers()
                patched_image = bytes(self.u.mem_read(self.obj, 0x4000))
                self.u.context_restore(initial)
                self.u.mem_write(self.obj, image)
                baseline = self.base + 0x3200000
                self.u.mem_write(baseline, original)
                self.u.emu_start(baseline, baseline + len(original), count=len(original) // 4)
                self.assertEqual(self.observable_registers(), patched_regs)
                self.assertEqual(bytes(self.u.mem_read(self.obj, 0x4000)), patched_image)

    def test_dynamic_override_disable_and_acknowledgement_order(self):
        for site in ORIGINAL:
            with self.subTest(site=hex(site)):
                self.fixture(site)
                acknowledgement = self.base + self.ctx.state + 0x324
                self.u.mem_write(acknowledgement, struct.pack('<f', -7))
                observed = []
                def write(u, access, address, size, value, data, acknowledgement=acknowledgement, observed=observed):
                    if address == acknowledgement:
                        observed.append(self.value(self.obj + 0xb0))
                self.u.hook_add(uc.UC_HOOK_MEM_WRITE, write)
                for ratio in (21 / 9, 32 / 9, 0, 2.16, 0):
                    # The viewport's original producer divides these each time.
                    if site in (0x1ff04f4, 0x253fbb0):
                        self.u.reg_write(arm.UC_ARM64_REG_S0, struct.unpack('<I', struct.pack('<f', 1280))[0])
                    before = self.observable_registers()
                    initial = self.u.context_save()
                    baseline = self.base + 0x3200000
                    original = ORIGINAL[site]
                    self.u.mem_write(baseline, original)
                    self.u.emu_start(baseline, baseline + len(original), count=len(original) // 4)
                    expected_fpsr = self.u.reg_read(arm.UC_ARM64_REG_FPSR)
                    self.u.context_restore(initial)
                    self.desired(ratio); self.execute()
                    self.assertAlmostEqual(self.value(self.obj + 0xb0), ratio or 16 / 9, places=6)
                    after = self.observable_registers()
                    self.assertEqual(after[:31], before[:31])
                    self.assertEqual(after[32:-1], before[32:-1])  # only q0's result may change
                    self.assertEqual(after[-1], expected_fpsr)  # original fdiv can set inexact
                    if site in (0x628c74, 0xb43460):
                        self.assertEqual(self.u.mem_read(self.obj + 8, 1), b'\x01')
                        self.assertAlmostEqual(self.value(acknowledgement), ratio, places=6)
                        self.assertAlmostEqual(observed[-1], ratio or 16 / 9, places=6)
                    else:
                        self.assertEqual(self.value(acknowledgement), -7)
                        self.assertFalse(observed)  # viewport cannot authorize presentation
                self.assertEqual(len(observed), 5 if site in (0x628c74, 0xb43460) else 0)


if __name__ == '__main__': unittest.main()
