#-------------------------------------------------------------------------------
# elftools example: elfclass_address_size.py
#
# This example explores the ELF class (32 or 64-bit) and address size in each
# of the CUs in the DWARF information.
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
from __future__ import print_function
import sys

# If pyelftools is not installed, the example can also run from the root or
# examples/ dir of the source distribution.
sys.path[0:0] = ['.', '..']

from elftools.elf.elffile import ELFFile


def process_file(filename):
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)
        # elfclass is a public attribute of ELFFile, read from its header
        print('%s: elfclass is %s' % (filename, elffile.elfclass))

        if elffile.has_dwarf_info():
            dwarfinfo = elffile.get_dwarf_info()
            for CU in dwarfinfo.iter_CUs():
                # cu_offset is a public attribute of CU
                # address_size is part of the CU header
                print('  CU at offset 0x%x. address_size is %s' % (
                    CU.cu_offset, CU['address_size']))


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        process_file(filename)

