#-------------------------------------------------------------------------------
# elftools example: elf_low_high_api.py
#
# A simple example that shows some usage of the low-level API pyelftools
# provides versus the high-level API while inspecting an ELF file's symbol
# table.
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
from elftools.elf.sections import SymbolTableSection


def process_file(filename):
    print('Processing file:', filename)
    with open(filename, 'rb') as f:
        section_info_lowlevel(f)
        f.seek(0)
        section_info_highlevel(f)


def section_info_lowlevel(stream):
    print('Low level API...')
    # We'll still be using the ELFFile context object. It's just too
    # convenient to give up, even in the low-level API demonstation :-)
    elffile = ELFFile(stream)

    # The e_shnum ELF header field says how many sections there are in a file
    print('  %s sections' % elffile['e_shnum'])

    # Try to find the symbol table
    for i in range(elffile['e_shnum']):
        section_offset = elffile['e_shoff'] + i * elffile['e_shentsize']
        # Parse the section header using structs.Elf_Shdr
        stream.seek(section_offset)
        section_header = elffile.structs.Elf_Shdr.parse_stream(stream)
        if section_header['sh_type'] == 'SHT_SYMTAB':
            # Some details about the section. Note that the section name is a
            # pointer to the object's string table, so it's only a number
            # here. To get to the actual name one would need to parse the string
            # table section and extract the name from there (or use the
            # high-level API!)
            print('  Section name: %s, type: %s' % (
                    section_header['sh_name'], section_header['sh_type']))
            break
    else:
        print('  No symbol table found. Perhaps this ELF has been stripped?')


def section_info_highlevel(stream):
    print('High level API...')
    elffile = ELFFile(stream)

    # Just use the public methods of ELFFile to get what we need
    # Note that section names, like everything read from the file, are bytes
    # objects.
    print('  %s sections' % elffile.num_sections())
    section = elffile.get_section_by_name(b'.symtab')

    if not section:
        print('  No symbol table found. Perhaps this ELF has been stripped?')
        return

    # A section type is in its header, but the name was decoded and placed in
    # a public attribute.
    # bytes2str is used to print the name of the section for consistency of
    # output between Python 2 and 3. The section name is a bytes object.
    print('  Section name: %s, type: %s' %(
        bytes2str(section.name), section['sh_type']))

    # But there's more... If this section is a symbol table section (which is
    # the case in the sample ELF file that comes with the examples), we can
    # get some more information about it.
    if isinstance(section, SymbolTableSection):
        num_symbols = section.num_symbols()
        print("  It's a symbol section with %s symbols" % num_symbols)
        print("  The name of the last symbol in the section is: %s" % (
            bytes2str(section.get_symbol(num_symbols - 1).name)))


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        process_file(filename)


