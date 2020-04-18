# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for export_to_cloud_trace."""

from __future__ import print_function

import contextlib
import mock
import threading

from chromite.lib import cros_test_lib
from chromite.scripts import export_to_cloud_trace


_SPAN = """{
    "name": "foo",
    "startTime": "2017-08-23T23:28:46.484326Z",
    "endTime": "2017-08-23T23:29:46.484326Z"
}"""

# pylint: disable=protected-access
class ExportToCloudTraceTest(cros_test_lib.MockTempDirTestCase):
  """Test that various functions in export_to_cloud_trace work."""

  def setUp(self):
    # Put a limit on how many times "time" can be called; this prevents the
    # thread from spinning forever if there is a bug.
    self.PatchObject(
        export_to_cloud_trace, 'time',
        mock.Mock(time=mock.Mock(
            side_effect=range(42))))

    self.PatchObject(export_to_cloud_trace, '_ReadAndDeletePreexisting')

  @contextlib.contextmanager
  def _SendSpansThread(self):
    client = mock.Mock()
    thread = threading.Thread(
        target=lambda: export_to_cloud_trace._WatchAndSendSpans(
            'example-project', client))
    thread.start()
    yield thread, client
    thread.join()

  def _PatchDirWatcher(self, spans):
    dir_watcher = mock.MagicMock(Batches=lambda: iter(spans))
    dir_watcher.__enter__ = mock.Mock(return_value=dir_watcher)
    self.PatchObject(
        export_to_cloud_trace, '_DirWatcher', mock.Mock(
            return_value=dir_watcher))

  def testSendSingleEvent(self):
    """Test that the script waits for a larger batch before sending."""
    self.PatchObject(export_to_cloud_trace, 'MIN_BATCH_SIZE', 2)
    self._PatchDirWatcher(spans=[
        [_SPAN]
    ])
    with self._SendSpansThread() as (_, client):
      pass

    # Even though MIN_BATCH_SIZE is 2, we will send a smaller batch if we run
    # out of time.
    self.assertEqual(client.projects().patchTraces.call_count, 1)

  def testSendBatches(self):
    """Test that the script waits for a larger batch before sending."""
    self.PatchObject(export_to_cloud_trace, 'MIN_BATCH_SIZE', 2)
    error_log = self.PatchObject(export_to_cloud_trace.log, 'error')
    self._PatchDirWatcher(spans=[
        [_SPAN, _SPAN], [_SPAN, _SPAN, _SPAN]
    ])
    with self._SendSpansThread() as (_, client):
      pass

    # MIN_BATCH_SIZE=2, but the second batch has 3 elements, and gets sent as
    # one batch.
    self.assertEqual(client.projects().patchTraces.call_count, 2)
    # We log an error on metrics emission when the span does not have required
    # fields (endTime, startTime and name).
    self.assertEqual(0, error_log.call_count)
