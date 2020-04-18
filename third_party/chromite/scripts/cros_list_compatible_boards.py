# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""List compatible boards that we can pull prebuilts from."""

from __future__ import print_function

import os

from chromite.cbuildbot import binhost
from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib


def _ParseArguments(argv):
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--buildroot', default=constants.SOURCE_ROOT,
                      help='Root directory where source is checked out to.')
  parser.add_argument('--prebuilt-type', required=True,
                      help='Type of prebuilt we want to look at.')
  parser.add_argument('--board', required=True,
                      help='Board to request prebuilts for.')
  opts = parser.parse_args(argv)
  opts.Freeze()
  return opts


def main(argv):
  cros_build_lib.AssertInsideChroot()
  opts = _ParseArguments(argv)
  filename = binhost.PrebuiltMapping.GetFilename(opts.buildroot,
                                                 opts.prebuilt_type)
  pfq_configs = binhost.PrebuiltMapping.Load(filename)
  extra_useflags = os.environ.get('USE', '').split()
  compat_id = binhost.CalculateCompatId(opts.board, extra_useflags)
  for key in pfq_configs.GetPrebuilts(compat_id):
    print(key.board)
