# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from dashboard.common import testing_common
from dashboard.models import graph_data


class GraphDataTest(testing_common.TestCase):

  def testPutTestTruncatesDescription(self):
    master = graph_data.Master(id='M').put()
    graph_data.Bot(parent=master, id='b').put()
    long_string = 500 * 'x'
    too_long = long_string + 'y'
    key = graph_data.TestMetadata(id='M/b/a', description=too_long).put()
    self.assertEqual(long_string, key.get().description)


if __name__ == '__main__':
  unittest.main()
