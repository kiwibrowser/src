#-------------------------------------------------------------------------------
# elftools example: elf_relocations.py
#
# An example of obtaining a relocation section from an ELF file and examining
# the relocation entries it contains.
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
from elftools.elf.relocation import RelocationSection


def process_file(filename):
    print('Processing file:', filename)
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)

        # Read the .rela.dyn section from the file, by explicitly asking
        # ELFFile for this section
        # Recall that section names are bytes objects
        reladyn_name = b'.rela.dyn'
        reladyn = elffile.get_section_by_name(reladyn_name)

        if not isinstance(reladyn, RelocationSection):
            print('  The file has no %s section' % bytes2str(reladyn_name))

        print('  %s section with %s relocations' % (
            bytes2str(reladyn_name), reladyn.num_relocations()))

        for reloc in reladyn.iter_relocations():
            print('    Relocation (%s)' % 'RELA' if reloc.is_RELA() else 'REL')
            # Relocation entry attributes are available through item lookup
            print('      offset = %s' % reloc['r_offset'])


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        process_file(filename)

