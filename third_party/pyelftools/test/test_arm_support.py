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

from utils import setup_syspath; setup_syspath()
from elftools.elf.elffile import ELFFile

class TestARMSupport(unittest.TestCase):
    def test_hello(self):
        with open(os.path.join('test', 'testfiles_for_unittests',
                               'simple_gcc.elf.arm'), 'rb') as f:
            elf = ELFFile(f)
            self.assertEqual(elf.get_machine_arch(), 'ARM')

            # Check some other properties of this ELF file derived from readelf
            self.assertEqual(elf['e_entry'], 0x8018)
            self.assertEqual(elf.num_sections(), 14)
            self.assertEqual(elf.num_segments(), 2)

    def test_DWARF_indirect_forms(self):
        # This file uses a lot of DW_FORM_indirect, and is also an ARM ELF
        # with non-trivial DWARF info.
        # So this is a simple sanity check that we can successfully parse it
        # and extract the expected amount of CUs.
        with open(os.path.join('test', 'testfiles_for_unittests',
                               'arm_with_form_indirect.elf'), 'rb') as f:
            elffile = ELFFile(f)
            self.assertTrue(elffile.has_dwarf_info())

            dwarfinfo = elffile.get_dwarf_info()
            all_CUs = list(dwarfinfo.iter_CUs())
            self.assertEqual(len(all_CUs), 9)

if __name__ == '__main__':
    unittest.main()

