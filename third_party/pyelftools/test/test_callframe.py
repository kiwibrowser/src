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
from elftools.common.py3compat import BytesIO
from elftools.dwarf.callframe import (
    CallFrameInfo, CIE, FDE, instruction_name, CallFrameInstruction,
    RegisterRule)
from elftools.dwarf.structs import DWARFStructs
from elftools.dwarf.descriptions import (describe_CFI_instructions,
    set_global_machine_arch)


class TestCallFrame(unittest.TestCase):
    def assertInstruction(self, instr, name, args):
        self.assertIsInstance(instr, CallFrameInstruction)
        self.assertEqual(instruction_name(instr.opcode), name)
        self.assertEqual(instr.args, args)

    def test_spec_sample_d6(self):
        # D.6 sample in DWARFv3
        s = BytesIO()
        data = (b'' +
            # first comes the CIE
            b'\x20\x00\x00\x00' +        # length
            b'\xff\xff\xff\xff' +        # CIE_id
            b'\x03\x00\x04\x7c' +        # version, augmentation, caf, daf
            b'\x08' +                    # return address
            b'\x0c\x07\x00' +
            b'\x08\x00' +
            b'\x07\x01' +
            b'\x07\x02' +
            b'\x07\x03' +
            b'\x08\x04' +
            b'\x08\x05' +
            b'\x08\x06' +
            b'\x08\x07' +
            b'\x09\x08\x01' +
            b'\x00' +

            # then comes the FDE
            b'\x28\x00\x00\x00' +        # length
            b'\x00\x00\x00\x00' +        # CIE_pointer (to CIE at 0)
            b'\x44\x33\x22\x11' +        # initial_location
            b'\x54\x00\x00\x00' +        # address range
            b'\x41' +
            b'\x0e\x0c' + b'\x41' +
            b'\x88\x01' + b'\x41' +
            b'\x86\x02' + b'\x41' +
            b'\x0d\x06' + b'\x41' +
            b'\x84\x03' + b'\x4b' +
            b'\xc4' + b'\x41' +
            b'\xc6' +
            b'\x0d\x07' + b'\x41' +
            b'\xc8' + b'\x41' +
            b'\x0e\x00' +
            b'\x00\x00'
            )
        s.write(data)

        structs = DWARFStructs(little_endian=True, dwarf_format=32, address_size=4)
        cfi = CallFrameInfo(s, len(data), structs)
        entries = cfi.get_entries()

        self.assertEqual(len(entries), 2)
        self.assertIsInstance(entries[0], CIE)
        self.assertEqual(entries[0]['length'], 32)
        self.assertEqual(entries[0]['data_alignment_factor'], -4)
        self.assertEqual(entries[0]['return_address_register'], 8)
        self.assertEqual(len(entries[0].instructions), 11)
        self.assertInstruction(entries[0].instructions[0],
            'DW_CFA_def_cfa', [7, 0])
        self.assertInstruction(entries[0].instructions[8],
            'DW_CFA_same_value', [7])
        self.assertInstruction(entries[0].instructions[9],
            'DW_CFA_register', [8, 1])

        self.assertTrue(isinstance(entries[1], FDE))
        self.assertEqual(entries[1]['length'], 40)
        self.assertEqual(entries[1]['CIE_pointer'], 0)
        self.assertEqual(entries[1]['address_range'], 84)
        self.assertIs(entries[1].cie, entries[0])
        self.assertEqual(len(entries[1].instructions), 21)
        self.assertInstruction(entries[1].instructions[0],
            'DW_CFA_advance_loc', [1])
        self.assertInstruction(entries[1].instructions[1],
            'DW_CFA_def_cfa_offset', [12])
        self.assertInstruction(entries[1].instructions[9],
            'DW_CFA_offset', [4, 3])
        self.assertInstruction(entries[1].instructions[18],
            'DW_CFA_def_cfa_offset', [0])
        self.assertInstruction(entries[1].instructions[20],
            'DW_CFA_nop', [])

        # Now let's decode it...
        decoded_CIE = entries[0].get_decoded()
        self.assertEqual(decoded_CIE.reg_order, list(range(9)))
        self.assertEqual(len(decoded_CIE.table), 1)
        self.assertEqual(decoded_CIE.table[0]['cfa'].reg, 7)
        self.assertEqual(decoded_CIE.table[0]['pc'], 0)
        self.assertEqual(decoded_CIE.table[0]['cfa'].offset, 0)
        self.assertEqual(decoded_CIE.table[0][4].type, RegisterRule.SAME_VALUE)
        self.assertEqual(decoded_CIE.table[0][8].type, RegisterRule.REGISTER)
        self.assertEqual(decoded_CIE.table[0][8].arg, 1)

        decoded_FDE = entries[1].get_decoded()
        self.assertEqual(decoded_FDE.reg_order, list(range(9)))
        self.assertEqual(decoded_FDE.table[0]['cfa'].reg, 7)
        self.assertEqual(decoded_FDE.table[0]['cfa'].offset, 0)
        self.assertEqual(decoded_FDE.table[0]['pc'], 0x11223344)
        self.assertEqual(decoded_FDE.table[0][8].type, RegisterRule.REGISTER)
        self.assertEqual(decoded_FDE.table[0][8].arg, 1)
        self.assertEqual(decoded_FDE.table[1]['cfa'].reg, 7)
        self.assertEqual(decoded_FDE.table[1]['cfa'].offset, 12)
        self.assertEqual(decoded_FDE.table[2][8].type, RegisterRule.OFFSET)
        self.assertEqual(decoded_FDE.table[2][8].arg, -4)
        self.assertEqual(decoded_FDE.table[2][4].type, RegisterRule.SAME_VALUE)
        self.assertEqual(decoded_FDE.table[5]['pc'], 0x11223344 + 20)
        self.assertEqual(decoded_FDE.table[5][4].type, RegisterRule.OFFSET)
        self.assertEqual(decoded_FDE.table[5][4].arg, -12)
        self.assertEqual(decoded_FDE.table[6]['pc'], 0x11223344 + 64)
        self.assertEqual(decoded_FDE.table[9]['pc'], 0x11223344 + 76)

    def test_describe_CFI_instructions(self):
        # The data here represents a single CIE
        data = (b'' +
            b'\x16\x00\x00\x00' +        # length
            b'\xff\xff\xff\xff' +        # CIE_id
            b'\x03\x00\x04\x7c' +        # version, augmentation, caf, daf
            b'\x08' +                    # return address
            b'\x0c\x07\x02' +
            b'\x10\x02\x07\x03\x01\x02\x00\x00\x06\x06')
        s = BytesIO(data)

        structs = DWARFStructs(little_endian=True, dwarf_format=32, address_size=4)
        cfi = CallFrameInfo(s, len(data), structs)
        entries = cfi.get_entries()

        set_global_machine_arch('x86')
        self.assertEqual(describe_CFI_instructions(entries[0]),
            (   '  DW_CFA_def_cfa: r7 (edi) ofs 2\n' +
                '  DW_CFA_expression: r2 (edx) (DW_OP_addr: 201; DW_OP_deref; DW_OP_deref)\n'))


if __name__ == '__main__':
    unittest.main()


