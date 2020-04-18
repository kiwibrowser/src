# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Helper methods for dealing with a SQLite database with pandas.
"""
import pandas  # pylint: disable=import-error


def _EmptyFrame(column_types):
  df = pandas.DataFrame()
  for column, dtype in column_types:
    df[column] = pandas.Series(dtype=dtype)
  return df


def CreateTableIfNotExists(con, name, column_types, keys=None):
  """Create a new empty table, if it doesn't already exist.

  Args:
    con: A sqlite connection object.
    name: Name of SQL table to create.
    column_types: A sequence of (column, dtype) pairs for the table schema.
    keys: A sequence of column names to set as PRIMARY KEY of the table.
  """
  frame = _EmptyFrame(column_types)
  db = pandas.io.sql.SQLiteDatabase(con)
  table = pandas.io.sql.SQLiteTable(
      name, db, frame=frame, index=False, keys=keys, if_exists='append')
  table.create()


def _InsertOrReplaceStatement(name, keys):
  columns = ','.join(keys)
  values = ','.join('?' for _ in keys)
  return 'INSERT OR REPLACE INTO %s(%s) VALUES (%s)' % (name, columns, values)


def InsertOrReplaceRecords(con, name, frame):
  """Insert or replace records from a DataFrame into a SQLite database.

  Assumes that the table already exists. Any new records with a matching
  PRIMARY KEY, usually the frame.index, will replace existing records.

  Args:
    con: A sqlite connection object.
    name: Name of SQL table.
    frame: DataFrame with records to write.
  """
  db = pandas.io.sql.SQLiteDatabase(con)
  table = pandas.io.sql.SQLiteTable(
      name, db, frame=frame, index=True, if_exists='append')
  assert table.exists()
  keys, data = table.insert_data()
  insert_statement = _InsertOrReplaceStatement(name, keys)
  with db.run_transaction() as c:
    c.executemany(insert_statement, zip(*data))
