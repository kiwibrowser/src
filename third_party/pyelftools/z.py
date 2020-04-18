#-------------------------------------------------------------------------------
# elftools
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------

# Just a script for playing around with pyelftools during testing
# please ignore it!
#
from __future__ import print_function

import sys, pprint
from elftools.elf.structs import ELFStructs
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import *

from elftools.elf.relocation import *


stream = open('test/testfiles/exe_simple64.elf', 'rb')

efile = ELFFile(stream)
print('elfclass', efile.elfclass)
print('===> %s sections!' % efile.num_sections())
print(efile.header)

dinfo = efile.get_dwarf_info()
from elftools.dwarf.locationlists import LocationLists
from elftools.dwarf.descriptions import describe_DWARF_expr
llists = LocationLists(dinfo.debug_loc_sec.stream, dinfo.structs)
for loclist in llists.iter_location_lists():
    print('----> loclist!')
    for li in loclist:
        print(li)
        print(describe_DWARF_expr(li.loc_expr, dinfo.structs))


