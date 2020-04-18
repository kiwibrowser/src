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

from utils import setup_syspath
setup_syspath()
from elftools.elf.elffile import ELFFile
from elftools.elf.constants import VER_FLAGS
from elftools.elf.gnuversions import (
        GNUVerNeedSection, GNUVerDefSection,
        GNUVerSymSection)


class TestSymbolVersioning(unittest.TestCase):

    versym_reference_data = [
        {'name': b'', 'ndx': 'VER_NDX_LOCAL'},
        {'name': b'', 'ndx': 'VER_NDX_LOCAL'},
        {'name': b'_ITM_deregisterTMCloneTable', 'ndx': 'VER_NDX_LOCAL'},
        {'name': b'puts', 'ndx': 5},
        {'name': b'strlcat', 'ndx': 'VER_NDX_LOCAL'},
        {'name': b'__stack_chk_fail', 'ndx': 6},
        {'name': b'__gmon_start__', 'ndx': 'VER_NDX_LOCAL'},
        {'name': b'gzoffset', 'ndx': 7},
        {'name': b'_Jv_RegisterClasses', 'ndx': 'VER_NDX_LOCAL'},
        {'name': b'_ITM_registerTMCloneTable', 'ndx': 'VER_NDX_LOCAL'},
        {'name': b'__cxa_finalize', 'ndx': 5},
        {'name': b'_edata', 'ndx': 'VER_NDX_GLOBAL'},
        {'name': b'VER_1.0', 'ndx': 2},
        {'name': b'function1_ver1_1', 'ndx': 'VER_NDX_GLOBAL'},
        {'name': b'_end', 'ndx': 'VER_NDX_GLOBAL'},
        {'name': b'function1', 'ndx': 4 | 0x8000},
        {'name': b'__bss_start', 'ndx': 'VER_NDX_GLOBAL'},
        {'name': b'function1', 'ndx': 2},
        {'name': b'VER_1.1', 'ndx': 3},
        {'name': b'_init', 'ndx': 'VER_NDX_GLOBAL'},
        {'name': b'function1_ver1_0', 'ndx': 'VER_NDX_GLOBAL'},
        {'name': b'_fini', 'ndx': 'VER_NDX_GLOBAL'},
        {'name': b'VER_1.2', 'ndx': 4},
        {'name': b'function2', 'ndx': 3},
    ]

    def test_versym_section(self):

        reference_data = TestSymbolVersioning.versym_reference_data

        with open(os.path.join('test', 'testfiles_for_unittests',
                               'lib_versioned64.so.1.elf'), 'rb') as f:
            elf = ELFFile(f)
            versym_section = None
            for section in elf.iter_sections():
                if isinstance(section, GNUVerSymSection):
                    versym_section = section
                    break

            self.assertIsNotNone(versym_section)

            for versym, ref_versym in zip(section.iter_symbols(),
                                                   reference_data):
                self.assertEqual(versym.name, ref_versym['name'])
                self.assertEqual(versym['ndx'], ref_versym['ndx'])

    verneed_reference_data = [
        {'name': b'libz.so.1', 'vn_version': 1, 'vn_cnt': 1,
         'vernaux': [
            {'name': b'ZLIB_1.2.3.5', 'vna_flags': 0, 'vna_other': 7}]},
        {'name': b'libc.so.6', 'vn_version': 1, 'vn_cnt': 2,
         'vernaux': [
            {'name': b'GLIBC_2.4', 'vna_flags': 0, 'vna_other': 6},
            {'name': b'GLIBC_2.2.5', 'vna_flags': 0, 'vna_other': 5}]},
        ]

    def test_verneed_section(self):

        reference_data = TestSymbolVersioning.verneed_reference_data

        with open(os.path.join('test', 'testfiles_for_unittests',
                               'lib_versioned64.so.1.elf'), 'rb') as f:
            elf = ELFFile(f)
            verneed_section = None
            for section in elf.iter_sections():
                if isinstance(section, GNUVerNeedSection):
                    verneed_section = section
                    break

            self.assertIsNotNone(verneed_section)

            for (verneed, vernaux_iter), ref_verneed in zip(
                    section.iter_versions(), reference_data):

                self.assertEqual(verneed.name, ref_verneed['name'])
                self.assertEqual(verneed['vn_cnt'], ref_verneed['vn_cnt'])
                self.assertEqual(verneed['vn_version'],
                                 ref_verneed['vn_version'])

                for vernaux, ref_vernaux in zip(
                        vernaux_iter, ref_verneed['vernaux']):

                    self.assertEqual(vernaux.name, ref_vernaux['name'])
                    self.assertEqual(vernaux['vna_flags'],
                                     ref_vernaux['vna_flags'])
                    self.assertEqual(vernaux['vna_other'],
                                     ref_vernaux['vna_other'])

    verdef_reference_data = [
        {'vd_ndx': 1, 'vd_version': 1, 'vd_flags': VER_FLAGS.VER_FLG_BASE,
         'vd_cnt': 1,
         'verdaux': [
            {'name': b'lib_versioned.so.1'}]},
        {'vd_ndx': 2, 'vd_version': 1, 'vd_flags': 0, 'vd_cnt': 1,
         'verdaux': [
            {'name': b'VER_1.0'}]},
        {'vd_ndx': 3, 'vd_version': 1, 'vd_flags': 0, 'vd_cnt': 2,
         'verdaux': [
            {'name': b'VER_1.1'},
            {'name': b'VER_1.0'}]},
        {'vd_ndx': 4, 'vd_version': 1, 'vd_flags': 0, 'vd_cnt': 2,
         'verdaux': [
            {'name': b'VER_1.2'},
            {'name': b'VER_1.1'}]},
        ]

    def test_verdef_section(self):

        reference_data = TestSymbolVersioning.verdef_reference_data

        with open(os.path.join('test', 'testfiles_for_unittests',
                               'lib_versioned64.so.1.elf'), 'rb') as f:
            elf = ELFFile(f)
            verneed_section = None
            for section in elf.iter_sections():
                if isinstance(section, GNUVerDefSection):
                    verdef_section = section
                    break

            self.assertIsNotNone(verdef_section)

            for (verdef, verdaux_iter), ref_verdef in zip(
                    section.iter_versions(), reference_data):

                self.assertEqual(verdef['vd_ndx'], ref_verdef['vd_ndx'])
                self.assertEqual(verdef['vd_version'],
                                 ref_verdef['vd_version'])
                self.assertEqual(verdef['vd_flags'], ref_verdef['vd_flags'])
                self.assertEqual(verdef['vd_cnt'], ref_verdef['vd_cnt'])

                for verdaux, ref_verdaux in zip(
                        verdaux_iter, ref_verdef['verdaux']):
                    self.assertEqual(verdaux.name, ref_verdaux['name'])

if __name__ == '__main__':
    unittest.main()
