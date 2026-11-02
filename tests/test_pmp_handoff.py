# SPDX-License-Identifier: MIT
"""Execute the actual PMP handoff against real libfdt and an ADT API fixture."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class PmpHandoff(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix="m1n1-pmp-handoff-")
        cls.addClassCleanup(cls.tmp.cleanup)
        work = Path(cls.tmp.name)
        source = (ROOT / "src/kboot.c").read_text()
        start = source.index("static const char *excluded_pmp_props[]")
        end = source.index("static int dt_set_sep(void)", start)
        (work / "pmp_under_test.inc").write_text(source[start:end])
        library = ROOT / "src/libfdt"
        files = ["fdt.c", "fdt_ro.c", "fdt_rw.c", "fdt_wip.c", "fdt_sw.c",
                 "fdt_empty_tree.c", "fdt_strerror.c"]
        cls.binary = work / "pmp-handoff"
        subprocess.run([os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-Wall", "-Wextra",
                        "-Werror", "-Wno-sign-compare", "-I" + str(work), "-I" + str(ROOT / "src"),
                        str(ROOT / "tests/pmp_handoff_host.c"),
                        *[str(library / name) for name in files], "-o", str(cls.binary)],
                       check=True, timeout=30)

    def test_machine_calibration_and_node_selection(self):
        for case in ("alias", "missing-alias", "another-unit", "ambiguous", "other-chip", "absent"):
            with self.subTest(case=case):
                result = subprocess.run([str(self.binary), case], capture_output=True, text=True,
                                        timeout=5, check=False)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
