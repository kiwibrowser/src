#-------------------------------------------------------------------------------
# elftools tests
#
# Eli Bendersky (eliben@gmail.com), Santhosh Kumar Mani (santhoshmani@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import os

from utils import setup_syspath; setup_syspath()
from elftools.elf.elffile import ELFFile

class TestRangeLists(unittest.TestCase):
    # Test the absence of .debug_ranges section
    def test_range_list_absence(self):
        with open(os.path.join('test', 'testfiles_for_unittests',
                               'arm_with_form_indirect.elf'), 'rb') as f:
            elffile = ELFFile(f)
            self.assertTrue(elffile.has_dwarf_info())
            self.assertIsNone(elffile.get_dwarf_info().range_lists())

    # Test the presence of .debug_ranges section
    def test_range_list_presence(self):
        with open(os.path.join('test', 'testfiles_for_unittests',
                               'sample_exe64.elf'), 'rb') as f:
            elffile = ELFFile(f)
            self.assertTrue(elffile.has_dwarf_info())
            self.assertIsNotNone(elffile.get_dwarf_info().range_lists())

if __name__ == '__main__':
    unittest.main()
