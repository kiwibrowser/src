#-------------------------------------------------------------------------------
# elftools example: dwarf_location_lists.py
#
# Examine DIE entries which have location list values, and decode these
# location lists.
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
from elftools.dwarf.locationlists import LocationEntry


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

        # The location lists are extracted by DWARFInfo from the .debug_loc
        # section, and returned here as a LocationLists object.
        location_lists = dwarfinfo.location_lists()

        # This is required for the descriptions module to correctly decode
        # register names contained in DWARF expressions.
        set_global_machine_arch(elffile.get_machine_arch())

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
                    if attribute_has_location_list(attr):
                        # This is a location list. Its value is an offset into
                        # the .debug_loc section, so we can use the location
                        # lists object to decode it.
                        loclist = location_lists.get_location_list_at_offset(
                            attr.value)

                        print('   DIE %s. attr %s.\n%s' % (
                            DIE.tag,
                            attr.name,
                            show_loclist(loclist, dwarfinfo, indent='      ')))


def show_loclist(loclist, dwarfinfo, indent):
    """ Display a location list nicely, decoding the DWARF expressions
        contained within.
    """
    d = []
    for loc_entity in loclist:
        if isinstance(loc_entity, LocationEntry):
            d.append('%s <<%s>>' % (
                loc_entity,
                describe_DWARF_expr(loc_entity.loc_expr, dwarfinfo.structs)))
        else:
            d.append(str(loc_entity))
    return '\n'.join(indent + s for s in d)


def attribute_has_location_list(attr):
    """ Only some attributes can have location list values, if they have the
        required DW_FORM (loclistptr "class" in DWARF spec v3)
    """
    if (attr.name in (  'DW_AT_location', 'DW_AT_string_length',
                        'DW_AT_const_value', 'DW_AT_return_addr',
                        'DW_AT_data_member_location', 'DW_AT_frame_base',
                        'DW_AT_segment', 'DW_AT_static_link',
                        'DW_AT_use_location', 'DW_AT_vtable_elem_location')):
        if attr.form in ('DW_FORM_data4', 'DW_FORM_data8'):
            return True
    return False


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        process_file(filename)






