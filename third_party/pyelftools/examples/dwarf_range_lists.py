#-------------------------------------------------------------------------------
# elftools example: dwarf_range_lists.py
#
# Examine DIE entries which have range list values, and decode these range
# lists.
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
from __future__ import print_function
import sys

# If pyelftools is not installed, the example can also run from the root or
# examples/ dir of the source distribution.
sys.path[0:0] = ['.', '..']

from elftools.common.py3compat import itervalues
from elftools.elf.elffile import ELFFile
from elftools.dwarf.descriptions import (
    describe_DWARF_expr, set_global_machine_arch)
from elftools.dwarf.ranges import RangeEntry


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

        # The range lists are extracted by DWARFInfo from the .debug_ranges
        # section, and returned here as a RangeLists object.
        range_lists = dwarfinfo.range_lists()
        if range_lists is None:
            print('  file has no .debug_ranges section')
            return

        for CU in dwarfinfo.iter_CUs():
            # DWARFInfo allows to iterate over the compile units contained in
            # the .debug_info section. CU is a CompileUnit object, with some
            # computed attributes (such as its offset in the section) and
            # a header which conforms to the DWARF standard. The access to
            # header elements is, as usual, via item-lookup.
            print('  Found a compile unit at offset %s, length %s' % (
                CU.cu_offset, CU['unit_length']))

            # A CU provides a simple API to iterate over all the DIEs in it.
            for DIE in CU.iter_DIEs():
                # Go over all attributes of the DIE. Each attribute is an
                # AttributeValue object (from elftools.dwarf.die), which we
                # can examine.
                for attr in itervalues(DIE.attributes):
                    if attribute_has_range_list(attr):
                        # This is a range list. Its value is an offset into
                        # the .debug_ranges section, so we can use the range
                        # lists object to decode it.
                        rangelist = range_lists.get_range_list_at_offset(
                            attr.value)

                        print('   DIE %s. attr %s.\n%s' % (
                            DIE.tag,
                            attr.name,
                            rangelist))


def attribute_has_range_list(attr):
    """ Only some attributes can have range list values, if they have the
        required DW_FORM (rangelistptr "class" in DWARF spec v3)
    """
    if attr.name == 'DW_AT_ranges':
        if attr.form in ('DW_FORM_data4', 'DW_FORM_data8'):
            return True
    return False


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        process_file(filename)







