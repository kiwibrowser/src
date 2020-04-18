# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from infra_libs.ts_mon.common import errors

class ErrorsTest(unittest.TestCase):

  ERRORS = [
      (errors.MonitoringDecreasingValueError, ('test', 1, 0)),
      (errors.MonitoringDuplicateRegistrationError, ('test',)),
      (errors.MonitoringIncrementUnsetValueError, ('test',)),
      (errors.MonitoringInvalidValueTypeError, ('test', 'foo')),
      (errors.MonitoringInvalidFieldTypeError, ('test', 'foo', 'bar')),
      (errors.MonitoringTooManyFieldsError, ('test', {'foo': 'bar'})),
      (errors.MonitoringNoConfiguredMonitorError, ('test',)),
      (errors.MonitoringNoConfiguredMonitorError, (None,)),
      (errors.MonitoringNoConfiguredTargetError, ('test',)),
      (errors.MonitoringFailedToFlushAllMetricsError, (3,)),
      (errors.MetricDefinitionError, ('foo')),
      (errors.WrongFieldsError, ('foo', ['a'], ['a', 'b'])),
  ]

  def test_smoke(self):
    for error_class, args in self.ERRORS:
      with self.assertRaises(error_class) as e:
        raise error_class(*args)
      str(e.exception)
