# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script generates brick runtime environment metadata from appc manifests.

On the device image we encode the outlines of the brick runtime environment in
SandboxSpecs, a protocol buffer understood by somad.  Brick developers specify
the information that goes into a SandboxSpec in the form of an appc pod
manifest, which a JSON blob adhering to an open standard.  This scripts maps
from pod manifests to SandboxSpecs.
"""

from __future__ import print_function

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import sandbox_spec_generator


def main(argv):
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--sysroot', type='path',
                      help='The sysroot to use for brick metadata validation.')
  parser.add_argument('appc_pod_manifest_path', type='path',
                      help='path to appc pod manifest')
  parser.add_argument('sandbox_spec_path', type='path',
                      help='path to file to write resulting SandboxSpec to. '
                           'Must not exist.')
  options = parser.parse_args(argv)
  options.Freeze()

  cros_build_lib.AssertInsideChroot()

  generator = sandbox_spec_generator.SandboxSpecGenerator(options.sysroot)
  generator.WriteSandboxSpec(options.appc_pod_manifest_path,
                             options.sandbox_spec_path)
  return 0
