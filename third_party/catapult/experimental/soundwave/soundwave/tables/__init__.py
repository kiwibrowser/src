# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from soundwave import pandas_sqlite
from soundwave.tables import alerts
from soundwave.tables import bugs
from soundwave.tables import timeseries


def CreateIfNeeded(con):
  """Creates soundwave tables in the database, if they don't already exist."""
  for m in (alerts, bugs, timeseries):
    pandas_sqlite.CreateTableIfNotExists(
        con, m.TABLE_NAME, m.COLUMN_TYPES, m.INDEX)
