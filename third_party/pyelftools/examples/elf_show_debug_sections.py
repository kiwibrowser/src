#-------------------------------------------------------------------------------
# elftools example: elf_show_debug_sections.py
#
# Show the names of all .debug_* sections in ELF files.
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
from __future__ import print_function
import sys

# If pyelftools is not installed, the example can also run from the root or
# examples/ dir of the source distribution.
sys.path[0:0] = ['.', '..']

from elftools.common.py3compat import bytes2str
from elftools.elf.elffile import ELFFile


def process_file(filename):
    print('In file:', filename)
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)

        for section in elffile.iter_sections():
            # Section names are bytes objects
            if section.name.startswith(b'.debug'):
                print('  ' + bytes2str(section.name))


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        process_file(filename)

