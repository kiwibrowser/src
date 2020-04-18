#!/usr/bin/python
import doctest
import glob
import sys
import louis

if sys.version_info >= (3,):
  sys.stderr.write("The doctests have not been ported to python 3. Skipping...\n")
  sys.exit(0)

class TestHelper():

    def __init__(self, tables):
        self.tables = tables

    def braille(self, txt):
        return louis.translateString(self.tables, txt)

    def cursor(self, txt, cursorPos):
        return louis.translate(self.tables, txt, 
                               cursorPos=cursorPos, 
                               mode=louis.compbrlAtCursor)[0:4:3]


exit_value = 0
for test in glob.iglob('doctests/*_test.txt'):
    failure_count, ignore = doctest.testfile(
        test, report=True, encoding='utf-8',
        extraglobs={'TestHelper': TestHelper})
    if failure_count > 0: 
        exit_value = 1

sys.exit(exit_value)
