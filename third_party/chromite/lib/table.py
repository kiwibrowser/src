# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Support generic spreadsheet-like table information."""

from __future__ import print_function

import inspect
import re
import sys

from chromite.lib import cros_build_lib


class Table(object):
  """Class to represent column headers and rows of data."""

  __slots__ = (
      '_column_set',  # Set of column headers (for faster lookup)
      '_columns',     # List of column headers in order
      '_name',        # Name to associate with table
      '_rows',        # List of row dicts
  )

  EMPTY_CELL = ''

  CSV_BQ = '__BEGINQUOTE__'
  CSV_EQ = '__ENDQUOTE__'

  @staticmethod
  def _SplitCSVLine(line):
    r'''Split a single CSV line into separate values.

    Behavior illustrated by the following examples, with all but
    the last example taken from Google Docs spreadsheet behavior:
    'a,b,c,d':           ==> ['a', 'b', 'c', 'd'],
    'a, b, c, d':        ==> ['a', ' b', ' c', ' d'],
    'a,b,c,':            ==> ['a', 'b', 'c', ''],
    'a,"b c",d':         ==> ['a', 'b c', 'd'],
    'a,"b, c",d':        ==> ['a', 'b, c', 'd'],
    'a,"b, c, d",e':     ==> ['a', 'b, c, d', 'e'],
    'a,"""b, c""",d':    ==> ['a', '"b, c"', 'd'],
    'a,"""b, c"", d",e': ==> ['a', '"b, c", d', 'e'],
    'a,b\,c,d':          ==> ['a', 'b,c', 'd'],

    Return a list of values.
    '''
    # Split on commas, handling two special cases:
    # 1) Escaped commas are not separators.
    # 2) A quoted value can have non-separator commas in it.  Quotes
    #    should be removed.
    vals = []
    for val in re.split(r'(?<!\\),', line):
      if not val:
        vals.append(val)
        continue

      # Handle regular double quotes at beginning/end specially.
      if val[0] == '"':
        val = Table.CSV_BQ + val[1:]
      if val[-1] == '"' and (val[-2] != '"' or val[-3] == '"'):
        val = val[0:-1] + Table.CSV_EQ

      # Remove escape characters now.
      val = val.replace(r'\,', ',')  # \ before ,
      val = val.replace('""', '"')   # " before " (Google Spreadsheet syntax)

      prevval = vals[-1] if vals else None

      # If previous value started with quote and ended without one, then
      # the current value is just a continuation of the previous value.
      if prevval and prevval.startswith(Table.CSV_BQ):
        val = prevval + ',' + val
        # Once entire value is read, strip surrounding quotes
        if val.endswith(Table.CSV_EQ):
          vals[-1] = val[len(Table.CSV_BQ):-len(Table.CSV_EQ)]
        else:
          vals[-1] = val
      elif val.endswith(Table.CSV_EQ):
        vals.append(val[len(Table.CSV_BQ):-len(Table.CSV_EQ)])
      else:
        vals.append(val)

    # If an unpaired Table.CSV_BQ is still in vals, then replace with ".
    vals = [val.replace(Table.CSV_BQ, '"') for val in vals]

    return vals

  @staticmethod
  def LoadFromCSV(csv_file, name=None):
    """Create a new Table object by loading contents of |csv_file|."""
    if type(csv_file) is file:
      file_handle = csv_file
    else:
      file_handle = open(csv_file, 'r')
    table = None

    for line in file_handle:
      if line[-1] == '\n':
        line = line[0:-1]

      vals = Table._SplitCSVLine(line)

      if not table:
        # Read headers
        table = Table(vals, name=name)

      else:
        # Read data row
        table.AppendRow(vals)

    return table

  def __init__(self, columns, name=None):
    self._columns = columns
    self._column_set = set(columns)
    self._rows = []
    self._name = name

  def __str__(self):
    """Return a table-like string representation of this table."""
    cols = ['%10s' % col for col in self._columns]
    text = 'Columns: %s\n' % ', '.join(cols)

    ix = 0
    for row in self._rows:
      vals = ['%10s' % row[col] for col in self._columns]
      text += 'Row %3d: %s\n' % (ix, ', '.join(vals))
      ix += 1
    return text

  def __nonzero__(self):
    """Define boolean equivalent for this table."""
    return bool(self._columns)

  def __len__(self):
    """Length of table equals the number of rows."""
    return self.GetNumRows()

  def __eq__(self, other):
    """Return true if two tables are equal."""
    # pylint: disable=protected-access
    return self._columns == other._columns and self._rows == other._rows

  def __ne__(self, other):
    """Return true if two tables are not equal."""
    return not self == other

  def __getitem__(self, index):
    """Access one or more rows by index or slice."""
    return self.GetRowByIndex(index)

  def __delitem__(self, index):
    """Delete one or more rows by index or slice."""
    self.RemoveRowByIndex(index)

  def __iter__(self):
    """Declare that this class supports iteration (over rows)."""
    return self._rows.__iter__()

  def GetName(self):
    """Return name associated with table, None if not available."""
    return self._name

  def SetName(self, name):
    """Set the name associated with table."""
    self._name = name

  def Clear(self):
    """Remove all row data."""
    self._rows = []

  def GetNumRows(self):
    """Return the number of rows in the table."""
    return len(self._rows)

  def GetNumColumns(self):
    """Return the number of columns in the table."""
    return len(self._columns)

  def GetColumns(self):
    """Return list of column names in order."""
    return list(self._columns)

  def GetRowByIndex(self, index):
    """Access one or more rows by index or slice.

    If more than one row is returned they will be contained in a list.
    """
    return self._rows[index]

  def _GenRowFilter(self, id_values):
    """Return a method that returns true for rows matching |id_values|."""
    def Grep(row):
      """Filter function for rows with id_values."""
      for key in id_values:
        if id_values[key] != row.get(key, None):
          return False
      return True
    return Grep

  def GetRowsByValue(self, id_values):
    """Return list of rows matching key/value pairs in |id_values|."""
    # If row retrieval by value is heavily used for larger tables, then
    # the implementation should change to be more efficient, at the
    # expense of some pre-processing and extra storage.
    grep = self._GenRowFilter(id_values)
    return [r for r in self._rows if grep(r)]

  def GetRowIndicesByValue(self, id_values):
    """Return list of indices for rows matching k/v pairs in |id_values|."""
    grep = self._GenRowFilter(id_values)
    indices = []
    for ix, row in enumerate(self._rows):
      if grep(row):
        indices.append(ix)

    return indices

  def _PrepareValuesForAdd(self, values):
    """Prepare a |values| dict/list to be added as a row.

    If |values| is a dict, verify that only supported column
    values are included. Add empty string values for columns
    not seen in the row.  The original dict may be altered.

    If |values| is a list, translate it to a dict using known
    column order.  Append empty values as needed to match number
    of expected columns.

    Return prepared dict.
    """
    if isinstance(values, dict):
      for col in values:
        if not col in self._column_set:
          raise LookupError("Tried adding data to unknown column '%s'" % col)

      for col in self._columns:
        if not col in values:
          values[col] = self.EMPTY_CELL

    elif isinstance(values, list):
      if len(values) > len(self._columns):
        raise LookupError('Tried adding row with too many columns')
      if len(values) < len(self._columns):
        shortage = len(self._columns) - len(values)
        values.extend([self.EMPTY_CELL] * shortage)

      values = dict(zip(self._columns, values))

    return values

  def AppendRow(self, values):
    """Add a single row of data to the table, according to |values|.

    The |values| argument can be either a dict or list.
    """
    row = self._PrepareValuesForAdd(values)
    self._rows.append(row)

  def SetRowByIndex(self, index, values):
    """Replace the row at |index| with values from |values| dict."""
    row = self._PrepareValuesForAdd(values)
    self._rows[index] = row

  def RemoveRowByIndex(self, index):
    """Remove the row at |index|."""
    del self._rows[index]

  def HasColumn(self, name):
    """Return True if column |name| is in this table, False otherwise."""
    return name in self._column_set

  def GetColumnIndex(self, name):
    """Return the column index for column |name|, -1 if not found."""
    for ix, col in enumerate(self._columns):
      if name == col:
        return ix
    return -1

  def GetColumnByIndex(self, index):
    """Return the column name at |index|"""
    return self._columns[index]

  def InsertColumn(self, index, name, value=None):
    """Insert a new column |name| into table at index |index|.

    If |value| is specified, all rows will have |value| in the new column.
    Otherwise, they will have the EMPTY_CELL value.
    """
    if self.HasColumn(name):
      raise LookupError('Column %s already exists in table.' % name)

    self._columns.insert(index, name)
    self._column_set.add(name)

    for row in self._rows:
      row[name] = value if value is not None else self.EMPTY_CELL

  def AppendColumn(self, name, value=None):
    """Same as InsertColumn, but new column is appended after existing ones."""
    self.InsertColumn(self.GetNumColumns(), name, value)

  def ProcessRows(self, row_processor):
    """Invoke |row_processor| on each row in sequence."""
    for row in self._rows:
      row_processor(row)

  def MergeTable(self, other_table, id_columns, merge_rules=None,
                 allow_new_columns=False, key=None, reverse=False,
                 new_name=None):
    """Merge |other_table| into this table, identifying rows by |id_columns|.

    The |id_columns| argument can either be a list of identifying columns names
    or a single column name (string).  The values in these columns will be used
    to identify the existing row that each row in |other_table| should be
    merged into.

    The |merge_rules| specify what to do when there is a merge conflict.  Every
    column where a conflict is anticipated should have an entry in the
    |merge_rules| dict.  The value should be one of:
    'join_with:<text>| = Join the two conflicting values with <text>
    'accept_this_val' = Keep value in 'this' table and discard 'other' value.
    'accept_other_val' = Keep value in 'other' table and discard 'this' value.
    function = Keep return value from function(col_name, this_val, other_val)

    A default merge rule can be specified with the key '__DEFAULT__' in
    |merge_rules|.

    By default, the |other_table| must not have any columns that don't already
    exist in this table.  To allow new columns to be creating by virtue of their
    presence in |other_table| set |allow_new_columns| to true.

    To sort the final merged table, supply |key| and |reverse| arguments exactly
    as they work with the Sort method.
    """
    # If requested, allow columns in other_table to create new columns
    # in this table if this table does not already have them.
    if allow_new_columns:
      # pylint: disable=protected-access
      for ix, col in enumerate(other_table._columns):
        if not self.HasColumn(col):
          # Create a merge_rule on the fly for this new column.
          if not merge_rules:
            merge_rules = {}
          merge_rules[col] = 'accept_other_val'

          if ix == 0:
            self.InsertColumn(0, col)
          else:
            prevcol = other_table._columns[ix - 1]
            previx = self.GetColumnIndex(prevcol)
            self.InsertColumn(previx + 1, col)

    for other_row in other_table:
      self._MergeRow(other_row, id_columns, merge_rules=merge_rules)

    # Optionally re-sort the merged table.
    if key:
      self.Sort(key, reverse=reverse)

    if new_name:
      self.SetName(new_name)
    elif self.GetName() and other_table.GetName():
      self.SetName(self.GetName() + ' + ' + other_table.GetName())

  def _GetIdValuesForRow(self, row, id_columns):
    """Return a dict with values from |row| in |id_columns|."""
    id_values = dict((col, row[col]) for col in
                     cros_build_lib.iflatten_instance(id_columns))
    return id_values

  def _MergeRow(self, other_row, id_columns, merge_rules=None):
    """Merge |other_row| into this table.

    See MergeTables for description of |id_columns| and |merge_rules|.
    """
    id_values = self._GetIdValuesForRow(other_row, id_columns)

    row_indices = self.GetRowIndicesByValue(id_values)
    if row_indices:
      row_index = row_indices[0]
      row = self.GetRowByIndex(row_index)
      for col in other_row:
        if col in row:
          # Find the merge rule that applies to this column, if there is one.
          merge_rule = None
          if merge_rules:
            merge_rule = merge_rules.get(col, None)
            if not merge_rule and merge_rules:
              merge_rule = merge_rules.get('__DEFAULT__', None)

          try:
            val = self._MergeColValue(col, row[col], other_row[col],
                                      merge_rule=merge_rule)
          except ValueError:
            msg = "Failed to merge '%s' value in row %r" % (col, id_values)
            print(msg, file=sys.stderr)
            raise

          if val != row[col]:
            row[col] = val
        else:
          # Cannot add new columns to row this way.
          raise LookupError("Tried merging data to unknown column '%s'" % col)
      self.SetRowByIndex(row_index, row)
    else:
      self.AppendRow(other_row)

  def _MergeColValue(self, col, val, other_val, merge_rule):
    """Merge |col| values |val| and |other_val| according to |merge_rule|.

    See MergeTable method for explanation of option |merge_rule|.
    """
    if val == other_val:
      return val

    if not merge_rule:
      raise ValueError("Cannot merge column values without rule: '%s' vs '%s'" %
                       (val, other_val))
    elif inspect.isfunction(merge_rule):
      try:
        return merge_rule(col, val, other_val)
      except ValueError:
        pass  # Fall through to exception at end
    elif merge_rule == 'accept_this_val':
      return val
    elif merge_rule == 'accept_other_val':
      return other_val
    else:
      match = re.match(r'join_with:(.+)$', merge_rule)
      if match:
        return match.group(1).join(v for v in (val, other_val) if v)

    raise ValueError("Invalid merge rule (%s) for values '%s' and '%s'." %
                     (merge_rule, val, other_val))

  def Sort(self, key, reverse=False):
    """Sort the rows using the given |key| function."""
    self._rows.sort(key=key, reverse=reverse)

  def WriteCSV(self, filehandle, hiddencols=None):
    """Write this table out as comma-separated values to |filehandle|.

    To skip certain columns during the write, use the |hiddencols| set.
    """
    def ColFilter(col):
      """Filter function for columns not in hiddencols."""
      return not hiddencols or col not in hiddencols

    cols = [col for col in self._columns if ColFilter(col)]
    filehandle.write(','.join(cols) + '\n')
    for row in self._rows:
      vals = [row.get(col, self.EMPTY_CELL) for col in cols]
      filehandle.write(','.join(vals) + '\n')
