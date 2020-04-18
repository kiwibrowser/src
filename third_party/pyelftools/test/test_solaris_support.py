#-------------------------------------------------------------------------------
# elftools tests
#
# Yann Rouillard (yann@pleiades.fr.eu.org)
# This code is in the public domain
#-------------------------------------------------------------------------------
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import os
import copy

from utils import setup_syspath; setup_syspath()
from elftools.elf.elffile import ELFFile
from elftools.elf.constants import SUNW_SYMINFO_FLAGS


class TestSolarisSupport(unittest.TestCase):

    def _test_SUNW_syminfo_section_generic(self, testfile):
        with open(os.path.join('test', 'testfiles_for_unittests',
                               testfile), 'rb') as f:
            elf = ELFFile(f)
            syminfo_section = elf.get_section_by_name(b'.SUNW_syminfo')
            self.assertIsNotNone(syminfo_section)

            # The test files were compiled against libc.so.1 with
            # direct binding, hence the libc symbols used
            # (exit, atexit and _exit) have the direct binding flags
            # in the syminfo table.
            # We check that this is properly detected.
            exit_symbols = [s for s in syminfo_section.iter_symbols()
                            if b'exit' in s.name]
            self.assertNotEqual(len(exit_symbols), 0)

            for symbol in exit_symbols:
                # libc.so.1 has the index 0 in the dynamic table
                self.assertEqual(symbol['si_boundto'], 0)
                self.assertEqual(symbol['si_flags'],
                                 SUNW_SYMINFO_FLAGS.SYMINFO_FLG_DIRECT |
                                 SUNW_SYMINFO_FLAGS.SYMINFO_FLG_DIRECTBIND)

    def test_SUNW_syminfo_section_x86(self):
        self._test_SUNW_syminfo_section_generic('exe_solaris32_cc.elf')

    def test_SUNW_syminfo_section_x64(self):
        self._test_SUNW_syminfo_section_generic('exe_solaris64_cc.elf')

    def test_SUNW_syminfo_section_sparc32(self):
        self._test_SUNW_syminfo_section_generic('exe_solaris32_cc.sparc.elf')

    def test_SUNW_syminfo_section_sparc64(self):
        self._test_SUNW_syminfo_section_generic('exe_solaris64_cc.sparc.elf')

    ldsynsym_reference_data = [b'', b'exe_solaris32.elf', b'crti.s', b'crt1.o',
                               b'crt1.s', b'fsr.s', b'values-Xa.c',
                               b'exe_solaris64.elf.c', b'crtn.s']

    def _test_SUNW_ldynsym_section_generic(self, testfile, reference_data):
        with open(os.path.join('test', 'testfiles_for_unittests',
                               testfile), 'rb') as f:
            elf = ELFFile(f)
            ldynsym_section = elf.get_section_by_name(b'.SUNW_ldynsym')
            self.assertIsNotNone(ldynsym_section)

            for symbol, ref_symbol_name in zip(
                    ldynsym_section.iter_symbols(), reference_data):

                self.assertEqual(symbol.name, ref_symbol_name)

    def test_SUNW_ldynsym_section_x86(self):
        reference_data = TestSolarisSupport.ldsynsym_reference_data
        self._test_SUNW_ldynsym_section_generic('exe_solaris32_cc.elf',
                                                reference_data)

    def test_SUNW_ldynsym_section_x64(self):
        reference_data = copy.deepcopy(
            TestSolarisSupport.ldsynsym_reference_data)
        reference_data[1] = b'exe_solaris64.elf'
        reference_data[3] = b'crt1x.o'
        reference_data[5] = b'fsrx.s'
        self._test_SUNW_ldynsym_section_generic('exe_solaris64_cc.elf',
                                                reference_data)

if __name__ == '__main__':
    unittest.main()
