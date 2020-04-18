# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Custom middlewares applicable to all apps on this site."""

from __future__ import print_function

from django.db import connection


class SqlPrintingMiddleware(object):
  """Middleware to print SQL stats for each page load."""

  # We hard code the terminal width because appengine SDK does not support the
  # fcntl python module. Without that, there's no reliable way to obtain the
  # terminal width.
  TERMINAL_WIDTH = 80
  INDENTATION = 2
  SQL_WIDTH = TERMINAL_WIDTH - INDENTATION
  INDENTATION_SPACE = ' ' * INDENTATION

  def _DisplayRed(self, value):
    return '\033[1;31m%s\033[0m' % value

  def _DisplayGreen(self, value):
    return '\033[1;32m%s\033[0m' % value

  def _PrintWithIndentation(self, value):
    print ('%s%s' % (self.INDENTATION_SPACE, value))

  def process_response(self, _, response):
    """Log SQL stats before forwarding response to the user."""
    if len(connection.queries) > 0:
      total_time = 0.0
      for query in connection.queries:
        total_time = total_time + float(query['time'])

        nice_sql = query['sql']
        sql = '[%s] %s' % (self._DisplayRed(query['time']), nice_sql)

        while len(sql) > self.SQL_WIDTH:
          self._PrintWithIndentation(sql[:self.SQL_WIDTH])
          sql = sql[self.SQL_WIDTH:]
        self._PrintWithIndentation(sql)
      self._PrintWithIndentation(self._DisplayGreen(
          '[TOTAL QUERIES: %s]' % len(connection.queries)))
      self._PrintWithIndentation(self._DisplayGreen(
          '[TOTAL TIME: %s seconds]' % total_time))
    return response
