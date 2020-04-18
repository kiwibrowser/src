# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Client to copy Chrome OS payloads with throttling."""

from __future__ import print_function

import collections

from chromite.lib.workqueue import service


_PAYLOAD_TIMEOUT = 5 * 60

_ThrottledCopyRequest = collections.namedtuple(
    '_ThrottledCopyRequest',
    ['hostname', 'localpath', 'remotepath', 'kwargs'])


DEFAULT_SPOOL_DIR = '/var/spool/devserver'

_copy_service = service.WorkQueueClient(DEFAULT_SPOOL_DIR)


def ThrottledCopy(hostname, localpath, remotepath, **kwargs):
  """Copy a Chrome OS payload to a remote host with throttling.

  This copies a given file to a target remote host.  The file is
  expected to be a large file like a Chrome OS payload; either a
  `stateful.tgz` tarball, or an AU payload (usually a full payload).
  The copy will be performed by the workqueue service to guarantee
  that the number of copies performed will be rate limited.

  Args:
    hostname: Name of the target DUT where the payload will be copied.
    localpath: Path name of the payload file on the local system.
    remotepath: Path name on the target where the payload will be
      copied.
    kwargs: Keyword arguments dictionary to be passed to
      `RemoteDevice.CopyToDevice()`.
  """
  task_request = _ThrottledCopyRequest(hostname, localpath,
                                       remotepath, kwargs)
  request_id = _copy_service.EnqueueRequest(task_request)
  return _copy_service.Wait(request_id, _PAYLOAD_TIMEOUT)
