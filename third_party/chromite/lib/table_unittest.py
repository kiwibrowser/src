# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the table module."""

from __future__ import print_function

import cStringIO
import sys
import tempfile

from chromite.lib import cros_test_lib
from chromite.lib import table


# pylint: disable=protected-access


class TableTest(cros_test_lib.TempDirTestCase):
  """Unit tests for the Table class."""

  COL0 = 'Column1'
  COL1 = 'Column2'
  COL2 = 'Column3'
  COL3 = 'Column4'
  COLUMNS = [COL0, COL1, COL2, COL3]

  ROW0 = {COL0: 'Xyz', COL1: 'Bcd', COL2: 'Cde'}
  ROW1 = {COL0: 'Abc', COL1: 'Bcd', COL2: 'Opq', COL3: 'Foo'}
  ROW2 = {COL0: 'Abc', COL1: 'Nop', COL2: 'Wxy', COL3: 'Bar'}

  EXTRAROW = {COL1: 'Walk', COL2: 'The', COL3: 'Line'}
  EXTRAROWOUT = {COL0: '', COL1: 'Walk', COL2: 'The', COL3: 'Line'}

  ROW0a = {COL0: 'Xyz', COL1: 'Bcd', COL2: 'Cde', COL3: 'Yay'}
  ROW0b = {COL0: 'Xyz', COL1: 'Bcd', COL2: 'Cde', COL3: 'Boo'}
  ROW1a = {COL0: 'Abc', COL1: 'Bcd', COL2: 'Opq', COL3: 'Blu'}

  EXTRACOL = 'ExtraCol'
  EXTRACOLUMNS = [COL0, EXTRACOL, COL1, COL2]

  EROW0 = {COL0: 'Xyz', EXTRACOL: 'Yay', COL1: 'Bcd', COL2: 'Cde'}
  EROW1 = {COL0: 'Abc', EXTRACOL: 'Hip', COL1: 'Bcd', COL2: 'Opq'}
  EROW2 = {COL0: 'Abc', EXTRACOL: 'Yay', COL1: 'Nop', COL2: 'Wxy'}

  def _GetRowValsInOrder(self, row):
    """Take |row| dict and return correctly ordered values in a list."""
    vals = []
    for col in self.COLUMNS:
      vals.append(row.get(col, ''))

    return vals

  def _GetFullRowFor(self, row, cols):
    return dict((col, row.get(col, '')) for col in cols)

  def assertRowsEqual(self, row1, row2):
    # Determine column superset
    cols = set(row1.keys() + row2.keys())
    self.assertEquals(self._GetFullRowFor(row1, cols),
                      self._GetFullRowFor(row2, cols))

  def assertRowListsEqual(self, rows1, rows2):
    for (row1, row2) in zip(rows1, rows2):
      self.assertRowsEqual(row1, row2)

  def setUp(self):
    self._table = self._CreateTableWithRows(self.COLUMNS,
                                            [self.ROW0, self.ROW1, self.ROW2])

  def _CreateTableWithRows(self, cols, rows):
    mytable = table.Table(list(cols))
    if rows:
      for row in rows:
        mytable.AppendRow(dict(row))
    return mytable

  def testLen(self):
    self.assertEquals(3, len(self._table))

  def testGetNumRows(self):
    self.assertEquals(3, self._table.GetNumRows())

  def testGetNumColumns(self):
    self.assertEquals(4, self._table.GetNumColumns())

  def testGetColumns(self):
    self.assertEquals(self.COLUMNS, self._table.GetColumns())

  def testGetColumnIndex(self):
    self.assertEquals(0, self._table.GetColumnIndex(self.COL0))
    self.assertEquals(1, self._table.GetColumnIndex(self.COL1))
    self.assertEquals(2, self._table.GetColumnIndex(self.COL2))

  def testGetColumnByIndex(self):
    self.assertEquals(self.COL0, self._table.GetColumnByIndex(0))
    self.assertEquals(self.COL1, self._table.GetColumnByIndex(1))
    self.assertEquals(self.COL2, self._table.GetColumnByIndex(2))

  def testGetByIndex(self):
    self.assertRowsEqual(self.ROW0, self._table.GetRowByIndex(0))
    self.assertRowsEqual(self.ROW0, self._table[0])

    self.assertRowsEqual(self.ROW2, self._table.GetRowByIndex(2))
    self.assertRowsEqual(self.ROW2, self._table[2])

  def testSlice(self):
    self.assertRowListsEqual([self.ROW0, self.ROW1], self._table[0:2])
    self.assertRowListsEqual([self.ROW2], self._table[-1:])

  def testGetByValue(self):
    rows = self._table.GetRowsByValue({self.COL0: 'Abc'})
    self.assertEquals([self.ROW1, self.ROW2], rows)
    rows = self._table.GetRowsByValue({self.COL2: 'Opq'})
    self.assertEquals([self.ROW1], rows)
    rows = self._table.GetRowsByValue({self.COL3: 'Foo'})
    self.assertEquals([self.ROW1], rows)

  def testGetIndicesByValue(self):
    indices = self._table.GetRowIndicesByValue({self.COL0: 'Abc'})
    self.assertEquals([1, 2], indices)
    indices = self._table.GetRowIndicesByValue({self.COL2: 'Opq'})
    self.assertEquals([1], indices)
    indices = self._table.GetRowIndicesByValue({self.COL3: 'Foo'})
    self.assertEquals([1], indices)

  def testAppendRowDict(self):
    self._table.AppendRow(dict(self.EXTRAROW))
    self.assertEquals(4, self._table.GetNumRows())
    self.assertEquals(self.EXTRAROWOUT, self._table[len(self._table) - 1])

  def testAppendRowList(self):
    self._table.AppendRow(self._GetRowValsInOrder(self.EXTRAROW))
    self.assertEquals(4, self._table.GetNumRows())
    self.assertEquals(self.EXTRAROWOUT, self._table[len(self._table) - 1])

  def testSetRowDictByIndex(self):
    self._table.SetRowByIndex(1, dict(self.EXTRAROW))
    self.assertEquals(3, self._table.GetNumRows())
    self.assertEquals(self.EXTRAROWOUT, self._table[1])

  def testSetRowListByIndex(self):
    self._table.SetRowByIndex(1, self._GetRowValsInOrder(self.EXTRAROW))
    self.assertEquals(3, self._table.GetNumRows())
    self.assertEquals(self.EXTRAROWOUT, self._table[1])

  def testRemoveRowByIndex(self):
    self._table.RemoveRowByIndex(1)
    self.assertEquals(2, self._table.GetNumRows())
    self.assertEquals(self.ROW2, self._table[1])

  def testRemoveRowBySlice(self):
    del self._table[0:2]
    self.assertEquals(1, self._table.GetNumRows())
    self.assertEquals(self.ROW2, self._table[0])

  def testIteration(self):
    ix = 0
    for row in self._table:
      self.assertEquals(row, self._table[ix])
      ix += 1

  def testClear(self):
    self._table.Clear()
    self.assertEquals(0, len(self._table))

  def testMergeRows(self):
    # This merge should fail without a merge rule.  Capture stderr to avoid
    # scary error message in test output.
    stderr = sys.stderr
    sys.stderr = cStringIO.StringIO()
    self.assertRaises(ValueError, self._table._MergeRow, self.ROW0a, self.COL0)
    sys.stderr = stderr

    # Merge but stick with current row where different.
    self._table._MergeRow(self.ROW0a, self.COL0,
                          merge_rules={self.COL3: 'accept_this_val'})
    self.assertEquals(3, len(self._table))
    self.assertRowsEqual(self.ROW0, self._table[0])

    # Merge and use new row where different.
    self._table._MergeRow(self.ROW0a, self.COL0,
                          merge_rules={self.COL3: 'accept_other_val'})
    self.assertEquals(3, len(self._table))
    self.assertRowsEqual(self.ROW0a, self._table[0])

    # Merge and combine column values where different
    self._table._MergeRow(self.ROW1a, self.COL2,
                          merge_rules={self.COL3: 'join_with: '})
    self.assertEquals(3, len(self._table))
    final_row = dict(self.ROW1a)
    final_row[self.COL3] = self.ROW1[self.COL3] + ' ' + self.ROW1a[self.COL3]
    self.assertRowsEqual(final_row, self._table[1])

  def testMergeTablesSameCols(self):
    other_table = self._CreateTableWithRows(self.COLUMNS,
                                            [self.ROW0b, self.ROW1a, self.ROW2])

    self._table.MergeTable(other_table, self.COL2,
                           merge_rules={self.COL3: 'join_with: '})

    final_row0 = self.ROW0b
    final_row1 = dict(self.ROW1a)
    final_row1[self.COL3] = self.ROW1[self.COL3] + ' ' + self.ROW1a[self.COL3]
    final_row2 = self.ROW2
    self.assertRowsEqual(final_row0, self._table[0])
    self.assertRowsEqual(final_row1, self._table[1])
    self.assertRowsEqual(final_row2, self._table[2])

  def testMergeTablesNewCols(self):
    self.assertFalse(self._table.HasColumn(self.EXTRACOL))

    other_rows = [self.EROW0, self.EROW1, self.EROW2]
    other_table = self._CreateTableWithRows(self.EXTRACOLUMNS, other_rows)

    self._table.MergeTable(other_table, self.COL2,
                           allow_new_columns=True,
                           merge_rules={self.COL3: 'join_by_space'})

    self.assertTrue(self._table.HasColumn(self.EXTRACOL))
    self.assertEquals(5, self._table.GetNumColumns())
    self.assertEquals(1, self._table.GetColumnIndex(self.EXTRACOL))

    final_row0 = dict(self.ROW0)
    final_row0[self.EXTRACOL] = self.EROW0[self.EXTRACOL]
    final_row1 = dict(self.ROW1)
    final_row1[self.EXTRACOL] = self.EROW1[self.EXTRACOL]
    final_row2 = dict(self.ROW2)
    final_row2[self.EXTRACOL] = self.EROW2[self.EXTRACOL]
    self.assertRowsEqual(final_row0, self._table[0])
    self.assertRowsEqual(final_row1, self._table[1])
    self.assertRowsEqual(final_row2, self._table[2])

  def testSort1(self):
    self.assertRowsEqual(self.ROW0, self._table[0])
    self.assertRowsEqual(self.ROW1, self._table[1])
    self.assertRowsEqual(self.ROW2, self._table[2])

    # Sort by COL3
    self._table.Sort(lambda row: row[self.COL3])

    self.assertEquals(3, len(self._table))
    self.assertRowsEqual(self.ROW0, self._table[0])
    self.assertRowsEqual(self.ROW2, self._table[1])
    self.assertRowsEqual(self.ROW1, self._table[2])

    # Reverse sort by COL3
    self._table.Sort(lambda row: row[self.COL3], reverse=True)

    self.assertEquals(3, len(self._table))
    self.assertRowsEqual(self.ROW1, self._table[0])
    self.assertRowsEqual(self.ROW2, self._table[1])
    self.assertRowsEqual(self.ROW0, self._table[2])

  def testSort2(self):
    """Test multiple key sort."""
    self.assertRowsEqual(self.ROW0, self._table[0])
    self.assertRowsEqual(self.ROW1, self._table[1])
    self.assertRowsEqual(self.ROW2, self._table[2])

    # Sort by COL0 then COL1
    def sorter(row):
      return (row[self.COL0], row[self.COL1])
    self._table.Sort(sorter)

    self.assertEquals(3, len(self._table))
    self.assertRowsEqual(self.ROW1, self._table[0])
    self.assertRowsEqual(self.ROW2, self._table[1])
    self.assertRowsEqual(self.ROW0, self._table[2])

    # Reverse the sort
    self._table.Sort(sorter, reverse=True)

    self.assertEquals(3, len(self._table))
    self.assertRowsEqual(self.ROW0, self._table[0])
    self.assertRowsEqual(self.ROW2, self._table[1])
    self.assertRowsEqual(self.ROW1, self._table[2])

  def testSplitCSVLine(self):
    """Test splitting of csv line."""
    tests = {
        'a,b,c,d':           ['a', 'b', 'c', 'd'],
        'a, b, c, d':        ['a', ' b', ' c', ' d'],
        'a,b,c,':            ['a', 'b', 'c', ''],
        'a,"b c",d':         ['a', 'b c', 'd'],
        'a,"b, c",d':        ['a', 'b, c', 'd'],
        'a,"b, c, d",e':     ['a', 'b, c, d', 'e'],
        'a,"""b, c""",d':    ['a', '"b, c"', 'd'],
        'a,"""b, c"", d",e': ['a', '"b, c", d', 'e'],

        # Following not real Google Spreadsheet cases.
        r'a,b\,c,d':         ['a', 'b,c', 'd'],
        'a,",c':             ['a', '",c'],
        'a,"",c':            ['a', '', 'c'],
    }
    for line in tests:
      vals = table.Table._SplitCSVLine(line)
      self.assertEquals(vals, tests[line])

  def testWriteReadCSV(self):
    """Write and Read CSV and verify contents preserved."""
    # This also tests the Table == and != operators.
    _, path = tempfile.mkstemp(text=True)
    tmpfile = open(path, 'w')
    self._table.WriteCSV(tmpfile)
    tmpfile.close()
    mytable = table.Table.LoadFromCSV(path)
    self.assertEquals(mytable, self._table)
    self.assertFalse(mytable != self._table)

  def testInsertColumn(self):
    self._table.InsertColumn(1, self.EXTRACOL, 'blah')
    goldenrow = dict(self.ROW1)
    goldenrow[self.EXTRACOL] = 'blah'
    self.assertRowsEqual(goldenrow, self._table.GetRowByIndex(1))
    self.assertEquals(self.EXTRACOL, self._table.GetColumnByIndex(1))

  def testAppendColumn(self):
    self._table.AppendColumn(self.EXTRACOL, 'blah')
    goldenrow = dict(self.ROW1)
    goldenrow[self.EXTRACOL] = 'blah'
    self.assertRowsEqual(goldenrow, self._table.GetRowByIndex(1))
    col_size = self._table.GetNumColumns()
    self.assertEquals(self.EXTRACOL, self._table.GetColumnByIndex(col_size - 1))

  def testProcessRows(self):
    def Processor(row):
      row[self.COL0] = row[self.COL0] + ' processed'
    self._table.ProcessRows(Processor)

    final_row0 = dict(self.ROW0)
    final_row0[self.COL0] += ' processed'
    final_row1 = dict(self.ROW1)
    final_row1[self.COL0] += ' processed'
    final_row2 = dict(self.ROW2)
    final_row2[self.COL0] += ' processed'
    self.assertRowsEqual(final_row0, self._table[0])
    self.assertRowsEqual(final_row1, self._table[1])
    self.assertRowsEqual(final_row2, self._table[2])
