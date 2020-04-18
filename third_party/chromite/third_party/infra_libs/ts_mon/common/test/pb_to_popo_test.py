# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from infra_libs.ts_mon.common import monitors
from infra_libs.ts_mon.common import pb_to_popo
from infra_libs.ts_mon.protos import metrics_pb2

class PbToPopoTest(unittest.TestCase):

  def test_convert(self):
    data_set = metrics_pb2.MetricsDataSet()
    data_set.metric_name = 'foo'
    data = data_set.data.add()
    data.bool_value = True
    data = data_set.data.add()
    data.bool_value = False
    data = data_set.data.add()
    data.int64_value = 200
    data = data_set.data.add()
    data.double_value = 123.456

    popo = pb_to_popo.convert(data_set)
    expected = {
      'metric_name': 'foo',
      'data': [
        {'bool_value': True},
        {'bool_value': False},
        {'int64_value': 200L},
        {'double_value': 123.456},
      ],
    }
    self.assertDictEqual(expected, popo)

