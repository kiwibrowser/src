#------------------------------------------------------------------------------
# elftools tests
#
# Yann Rouillard (yann@pleiades.fr.eu.org)
# This code is in the public domain
#------------------------------------------------------------------------------
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import os

from utils import setup_syspath; setup_syspath()
from elftools.elf.elffile import ELFFile
from elftools.elf.dynamic import DynamicSection, DynamicTag


class TestDoubleDynstrSections(unittest.TestCase):
    """ This test make sure than dynamic tags
        are properly analyzed when two .dynstr
        sections are present in an elf file
    """

    reference_data = [
        b'libz.so.1',
        b'libc.so.6',
        b'lib_versioned.so.1',
    ]

    def _test_double_dynstr_section_generic(self, testfile):

        with open(os.path.join('test', 'testfiles_for_unittests', testfile),
                  'rb') as f:
            elf = ELFFile(f)
            for section in elf.iter_sections():
                if isinstance(section, DynamicSection):
                    d_tags = [getattr(x, x.entry.d_tag[3:].lower())
                              for x in section.iter_tags()
                              if x.entry.d_tag in DynamicTag._HANDLED_TAGS]
                    self.assertListEqual(
                            TestDoubleDynstrSections.reference_data,
                            d_tags)
                    return
            self.fail('No dynamic section found !!')


    def test_double_dynstr_section(self):
        """ First test with the good dynstr section first
        """
        self._test_double_dynstr_section_generic(
                'lib_with_two_dynstr_sections.so.1.elf')

    def test_double_dynstr_section_reverse(self):
        """ Second test with the good dynstr section last
        """
        self._test_double_dynstr_section_generic(
                'lib_with_two_dynstr_sections_reversed.so.1.elf')


if __name__ == '__main__':
    unittest.main()
