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

from utils import setup_syspath; setup_syspath()
from elftools.dwarf.descriptions import ExprDumper, set_global_machine_arch
from elftools.dwarf.structs import DWARFStructs


class TestExprDumper(unittest.TestCase):
    structs32 = DWARFStructs(
            little_endian=True,
            dwarf_format=32,
            address_size=4)

    def setUp(self):
        self.visitor = ExprDumper(self.structs32)
        set_global_machine_arch('x64')

    def test_basic_single(self):
        self.visitor.process_expr([0x1b])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_div')

        self.setUp()
        self.visitor.process_expr([0x74, 0x82, 0x01])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_breg4 (rsi): 130')

        self.setUp()
        self.visitor.process_expr([0x91, 0x82, 0x01])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_fbreg: 130')

        self.setUp()
        self.visitor.process_expr([0x51])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_reg1 (rdx)')

        self.setUp()
        self.visitor.process_expr([0x90, 16])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_regx: 16 (rip)')

        self.setUp()
        self.visitor.process_expr([0x9d, 0x8f, 0x0A, 0x90, 0x01])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_bit_piece: 1295 144')

    def test_basic_sequence(self):
        self.visitor.process_expr([0x03, 0x01, 0x02, 0, 0, 0x06, 0x06])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_addr: 201; DW_OP_deref; DW_OP_deref')

        self.setUp()
        self.visitor.process_expr([0x15, 0xFF, 0x0b, 0xf1, 0xff])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_pick: 255; DW_OP_const2s: -15')

        self.setUp()
        self.visitor.process_expr([0x1d, 0x1e, 0x1d, 0x1e, 0x1d, 0x1e])
        self.assertEqual(self.visitor.get_str(),
            'DW_OP_mod; DW_OP_mul; DW_OP_mod; DW_OP_mul; DW_OP_mod; DW_OP_mul')


if __name__ == '__main__':
    unittest.main()


