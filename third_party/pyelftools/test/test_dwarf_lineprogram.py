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
from elftools.common.py3compat import BytesIO, iteritems
from elftools.dwarf.lineprogram import LineProgram, LineState, LineProgramEntry
from elftools.dwarf.structs import DWARFStructs
from elftools.dwarf.constants import *


class TestLineProgram(unittest.TestCase):
    def _make_program_in_stream(self, stream):
        """ Create a LineProgram from the given program encoded in a stream
        """
        ds = DWARFStructs(little_endian=True, dwarf_format=32, address_size=4)
        header = ds.Dwarf_lineprog_header.parse(
            b'\x04\x10\x00\x00' +    # initial lenght
            b'\x03\x00' +            # version
            b'\x20\x00\x00\x00' +    # header length
            b'\x01\x01\x01\x0F' +    # flags
            b'\x0A' +                # opcode_base
            b'\x00\x01\x04\x08\x0C\x01\x01\x01\x00' + # standard_opcode_lengths
            # 2 dir names followed by a NULL
            b'\x61\x62\x00\x70\x00\x00' +
            # a file entry
            b'\x61\x72\x00\x0C\x0D\x0F' +
            # and another entry
            b'\x45\x50\x51\x00\x86\x12\x07\x08' +
            # followed by NULL
            b'\x00')

        lp = LineProgram(header, stream, ds, 0, len(stream.getvalue()))
        return lp

    def assertLineState(self, state, **kwargs):
        """ Assert that the state attributes specified in kwargs have the given
            values (the rest are default).
        """
        for k, v in iteritems(kwargs):
            self.assertEqual(getattr(state, k), v)

    def test_spec_sample_59(self):
        # Sample in figure 59 of DWARFv3
        s = BytesIO()
        s.write(
            b'\x02\xb9\x04' +
            b'\x0b' +
            b'\x38' +
            b'\x82' +
            b'\x73' +
            b'\x02\x02' +
            b'\x00\x01\x01')

        lp = self._make_program_in_stream(s)
        linetable = lp.get_entries()

        self.assertEqual(len(linetable), 7)
        self.assertIs(linetable[0].state, None)  # doesn't modify state
        self.assertEqual(linetable[0].command, DW_LNS_advance_pc)
        self.assertEqual(linetable[0].args, [0x239])
        self.assertLineState(linetable[1].state, address=0x239, line=3)
        self.assertEqual(linetable[1].command, 0xb)
        self.assertEqual(linetable[1].args, [2, 0])
        self.assertLineState(linetable[2].state, address=0x23c, line=5)
        self.assertLineState(linetable[3].state, address=0x244, line=6)
        self.assertLineState(linetable[4].state, address=0x24b, line=7, end_sequence=False)
        self.assertEqual(linetable[5].command, DW_LNS_advance_pc)
        self.assertEqual(linetable[5].args, [2])
        self.assertLineState(linetable[6].state, address=0x24d, line=7, end_sequence=True)

    def test_spec_sample_60(self):
        # Sample in figure 60 of DWARFv3
        s = BytesIO()
        s.write(
            b'\x09\x39\x02' +
            b'\x0b' +
            b'\x09\x03\x00' +
            b'\x0b' +
            b'\x09\x08\x00' +
            b'\x0a' +
            b'\x09\x07\x00' +
            b'\x0a' +
            b'\x09\x02\x00' +
            b'\x00\x01\x01')

        lp = self._make_program_in_stream(s)
        linetable = lp.get_entries()

        self.assertEqual(len(linetable), 10)
        self.assertIs(linetable[0].state, None)  # doesn't modify state
        self.assertEqual(linetable[0].command, DW_LNS_fixed_advance_pc)
        self.assertEqual(linetable[0].args, [0x239])
        self.assertLineState(linetable[1].state, address=0x239, line=3)
        self.assertLineState(linetable[3].state, address=0x23c, line=5)
        self.assertLineState(linetable[5].state, address=0x244, line=6)
        self.assertLineState(linetable[7].state, address=0x24b, line=7, end_sequence=False)
        self.assertLineState(linetable[9].state, address=0x24d, line=7, end_sequence=True)


if __name__ == '__main__':
    unittest.main()

