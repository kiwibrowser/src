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
from elftools.dwarf.structs import DWARFStructs


class TestDWARFStructs(unittest.TestCase):
    def test_lineprog_header(self):
        ds = DWARFStructs(little_endian=True, dwarf_format=32, address_size=4)

        c = ds.Dwarf_lineprog_header.parse(
            b'\x04\x10\x00\x00' +    # initial lenght
            b'\x05\x02' +            # version
            b'\x20\x00\x00\x00' +    # header length
            b'\x05\x10\x40\x50' +    # until and including line_range
            b'\x06' +                # opcode_base
            b'\x00\x01\x04\x08\x0C' + # standard_opcode_lengths
            # 2 dir names followed by a NULL
            b'\x61\x62\x00\x70\x00\x00' +
            # a file entry
            b'\x61\x72\x00\x0C\x0D\x0F' +
            # and another entry
            b'\x45\x50\x51\x00\x86\x12\x07\x08' +
            # followed by NULL
            b'\x00')

        self.assertEqual(c.version, 0x205)
        self.assertEqual(c.opcode_base, 6)
        self.assertEqual(c.standard_opcode_lengths, [0, 1, 4, 8, 12])
        self.assertEqual(c.include_directory, [b'ab', b'p'])
        self.assertEqual(len(c.file_entry), 2)
        self.assertEqual(c.file_entry[0].name, b'ar')
        self.assertEqual(c.file_entry[1].name, b'EPQ')
        self.assertEqual(c.file_entry[1].dir_index, 0x12 * 128 + 6)


if __name__ == '__main__':
    unittest.main()

