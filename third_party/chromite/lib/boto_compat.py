# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Boto compatibility functions for cbuildbot.

This module provides compatibility functions to help manage old versions of
boto/gsutil.

NOTE: This should eventually be removed as part of crbug.com/845304.
"""

from __future__ import print_function

import ConfigParser
import contextlib
import os
import tempfile

from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import osutils

# Path to an updated cacerts.txt file, which will override the cacerts.txt file
# embedded in boto in the wrapped FixBotoCerts context. Relative to chromite/.
BOTO_CACERTS_PATH = 'third_party/boto/boto/cacerts/cacerts.txt'

BOTO_CACERTS_ABS_PATH = os.path.join(
    constants.CHROMITE_DIR,
    os.path.normpath(BOTO_CACERTS_PATH))


@contextlib.contextmanager
def FixBotoCerts(activate=True, strict=False):
  """Fix for outdated cacerts.txt file in old versions of boto/gsutil."""

  if not activate:
    logging.info('FixBotoCerts skipped')
    yield
    return

  logging.info('FixBotoCerts started')

  orig_env = os.environ.copy()

  boto_cfg_path = None
  try:
    config = ConfigParser.SafeConfigParser()

    # Read existing boto config file(s); this mimics what boto itself does.
    if 'BOTO_CONFIG' in os.environ:
      config.read(os.environ['BOTO_CONFIG'])
    else:
      boto_path = os.environ.get('BOTO_PATH', '/etc/boto.cfg:~/.boto')
      config.read(boto_path.split(':'))

    # Set [Boto] ca_certificates_file = <path to cacerts.txt>.
    if not config.has_section('Boto'):
      config.add_section('Boto')
    config.set('Boto', 'ca_certificates_file', BOTO_CACERTS_ABS_PATH)

    # Write updated boto config to a tempfile.
    fd, boto_cfg_path = tempfile.mkstemp(prefix='fix_certs', suffix='boto.cfg')
    os.close(fd)
    with open(boto_cfg_path, 'w') as f:
      config.write(f)
    os.chmod(boto_cfg_path, 0o644)

    # Update env to use only our generated boto config.
    os.environ['BOTO_CONFIG'] = boto_cfg_path
    os.environ.pop('BOTO_PATH', None)

  except Exception, e:
    if strict:
      raise e
    logging.warning('FixBotoCerts init failed: %s', e)
    # Don't make things worse; let the build continue.

  try:
    yield
  finally:
    # Restore env.
    osutils.SetEnvironment(orig_env)

    # Clean up the boto.cfg file.
    if boto_cfg_path:
      try:
        os.remove(boto_cfg_path)
      except Exception, e:
        if strict:
          raise e
        logging.warning('FixBotoCerts failed removing %s: %s', boto_cfg_path, e)
