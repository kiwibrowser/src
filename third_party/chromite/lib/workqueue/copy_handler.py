# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Handler for copying Chrome OS payloads to DUTs."""

from __future__ import print_function

from chromite.lib import cros_logging as logging
from chromite.lib import remote_access


_LOGGING_FORMAT = '%(asctime)s %(levelname)-10s - %(message)s'
_LOGGING_DATE_FORMAT = '%Y-%m-%d %H:%M:%S'

LOG_FORMATTER = logging.Formatter(_LOGGING_FORMAT,
                                  _LOGGING_DATE_FORMAT)


def CopyPayload(request_data):
  """Copy a payload file to a target DUT.

  This constructs a `RemoteDevice`, and calls its `CopyToDevice()`
  method to copy a payload file to the target device.  A payload
  file is either the `stateful.tgz` tarball, or a Chrome OS AU
  payload (typically a full payload).

  The `request_data` argument has the following fields:
    * hostname:  Name of the target DUT where the payload will
      be copied.
    * localpath:  Path on this system to the payload file.
    * remotepath:  Path on the DUT where the payload will be copied.
    * kwargs:  Keyword arguments dictionanry to be passed
      to `RemoteDevice.CopyToDevice()`.
    * kwargs['log_stdout_to_file']:  If present, a file to which logger
      output will be written.  The output will typically include the
      command used for the copy, and standard input/output of that copy
      command.
  """
  log_handler = None
  log_file = request_data.kwargs.get('log_stdout_to_file')
  if log_file:
    log_handler = logging.FileHandler(log_file)
    log_handler.setFormatter(LOG_FORMATTER)
    logging.getLogger().addHandler(log_handler)
  try:
    device = remote_access.RemoteDevice(request_data.hostname)
    device.CopyToDevice(request_data.localpath,
                        request_data.remotepath,
                        mode='scp',
                        **request_data.kwargs)
  finally:
    if log_handler:
      logging.getLogger().removeHandler(log_handler)
