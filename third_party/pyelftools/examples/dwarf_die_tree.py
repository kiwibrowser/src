#-------------------------------------------------------------------------------
# elftools example: dwarf_die_tree.py
#
# In the .debug_info section, Dwarf Information Entries (DIEs) form a tree.
# pyelftools provides easy access to this tree, as demonstrated here.
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
    print('Processing file:', filename)
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)

        if not elffile.has_dwarf_info():
            print('  file has no DWARF info')
            return

        # get_dwarf_info returns a DWARFInfo context object, which is the
        # starting point for all DWARF-based processing in pyelftools.
        dwarfinfo = elffile.get_dwarf_info()

        for CU in dwarfinfo.iter_CUs():
            # DWARFInfo allows to iterate over the compile units contained in
            # the .debug_info section. CU is a CompileUnit object, with some
            # computed attributes (such as its offset in the section) and
            # a header which conforms to the DWARF standard. The access to
            # header elements is, as usual, via item-lookup.
            print('  Found a compile unit at offset %s, length %s' % (
                CU.cu_offset, CU['unit_length']))

            # Start with the top DIE, the root for this CU's DIE tree
            top_DIE = CU.get_top_DIE()
            print('    Top DIE with tag=%s' % top_DIE.tag)

            # We're interested in the filename...
            print('    name=%s' % top_DIE.get_full_path())

            # Display DIEs recursively starting with top_DIE
            die_info_rec(top_DIE)


def die_info_rec(die, indent_level='    '):
    """ A recursive function for showing information about a DIE and its
        children.
    """
    print(indent_level + 'DIE tag=%s' % die.tag)
    child_indent = indent_level + '  '
    for child in die.iter_children():
        die_info_rec(child, child_indent)


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        process_file(filename)






