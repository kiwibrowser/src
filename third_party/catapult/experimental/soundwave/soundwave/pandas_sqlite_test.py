# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import pandas  # pylint: disable=import-error
import sqlite3
import unittest

from soundwave import pandas_sqlite


class TestPandasSQLite(unittest.TestCase):
  def testCreateTableIfNotExists_newTable(self):
    column_types = (('bug_id', int), ('summary', str), ('status', str))
    index = 'bug_id'
    con = sqlite3.connect(':memory:')
    try:
      self.assertFalse(pandas.io.sql.has_table('bugs', con))
      pandas_sqlite.CreateTableIfNotExists(con, 'bugs', column_types, index)
      self.assertTrue(pandas.io.sql.has_table('bugs', con))
    finally:
      con.close()

  def testCreateTableIfNotExists_alreadyExists(self):
    column_types = (('bug_id', int), ('summary', str), ('status', str))
    index = 'bug_id'
    con = sqlite3.connect(':memory:')
    try:
      self.assertFalse(pandas.io.sql.has_table('bugs', con))
      pandas_sqlite.CreateTableIfNotExists(con, 'bugs', column_types, index)
      self.assertTrue(pandas.io.sql.has_table('bugs', con))
      # It's fine to call a second time.
      pandas_sqlite.CreateTableIfNotExists(con, 'bugs', column_types, index)
      self.assertTrue(pandas.io.sql.has_table('bugs', con))
    finally:
      con.close()

  def testInsertOrReplaceRecords_tableNotExistsRaises(self):
    column_types = (('bug_id', int), ('summary', str), ('status', str))
    columns = tuple(c for c, _ in column_types)
    index = columns[0]
    df1 = pandas.DataFrame.from_records(
        [(123, 'Some bug', 'Started'), (456, 'Another bug', 'Assigned')],
        columns=columns, index=index)
    con = sqlite3.connect(':memory:')
    try:
      with self.assertRaises(AssertionError):
        pandas_sqlite.InsertOrReplaceRecords(con, 'bugs', df1)
    finally:
      con.close()

  def testInsertOrReplaceRecords_existingRecords(self):
    column_types = (('bug_id', int), ('summary', str), ('status', str))
    columns = tuple(c for c, _ in column_types)
    index = columns[0]
    df1 = pandas.DataFrame.from_records(
        [(123, 'Some bug', 'Started'), (456, 'Another bug', 'Assigned')],
        columns=columns, index=index)
    df2 = pandas.DataFrame.from_records(
        [(123, 'Some bug', 'Fixed'), (789, 'A new bug', 'Untriaged')],
        columns=columns, index=index)
    con = sqlite3.connect(':memory:')
    try:
      pandas_sqlite.CreateTableIfNotExists(con, 'bugs', column_types, index)

      # Write first data frame to database.
      pandas_sqlite.InsertOrReplaceRecords(con, 'bugs', df1)
      df = pandas.read_sql('SELECT * FROM bugs', con, index_col=index)
      self.assertEqual(len(df), 2)
      self.assertEqual(df.loc[123]['status'], 'Started')

      # Write second data frame to database.
      pandas_sqlite.InsertOrReplaceRecords(con, 'bugs', df2)
      df = pandas.read_sql('SELECT * FROM bugs', con, index_col=index)
      self.assertEqual(len(df), 3)  # Only one extra record added.
      self.assertEqual(df.loc[123]['status'], 'Fixed')  # Bug is now fixed.
      self.assertItemsEqual(df.index, (123, 456, 789))
    finally:
      con.close()


if __name__ == '__main__':
  unittest.main()
