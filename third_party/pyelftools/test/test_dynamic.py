#-------------------------------------------------------------------------------
# elftools tests
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import os

from utils import setup_syspath
setup_syspath()
from elftools.elf.elffile import ELFFile
from elftools.common.exceptions import ELFError
from elftools.elf.dynamic import DynamicTag


class TestDynamicTag(unittest.TestCase):
    """Tests for the DynamicTag class."""

    def test_requires_stringtable(self):
        with self.assertRaises(ELFError):
            dt = DynamicTag('', None)


class TestDynamic(unittest.TestCase):
    """Tests for the Dynamic class."""

    def test_missing_sections(self):
        """Verify we can get dynamic strings w/out section headers"""

        libs = []
        with open(os.path.join('test', 'testfiles_for_unittests',
                               'aarch64_super_stripped.elf'), 'rb') as f:
            elf = ELFFile(f)
            for segment in elf.iter_segments():
                if segment.header.p_type != 'PT_DYNAMIC':
                    continue

                for t in segment.iter_tags():
                    if t.entry.d_tag == 'DT_NEEDED':
                        libs.append(t.needed.decode('utf-8'))

        exp = ['libc.so.6']
        self.assertEqual(libs, exp)


if __name__ == '__main__':
    unittest.main()
