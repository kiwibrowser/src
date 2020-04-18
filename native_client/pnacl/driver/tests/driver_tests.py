#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Run all python tests in this directory."""

import os
import sys
import unittest

MODULES = [
    'default_output_filename_test',
    'diagnostic_flags_test',
    'driver_env_test',
    'driver_tools_test',
    'expand_response_file_test',
    'filetype_test',
    'fix_private_libs_test',
    'force_file_type_test',
    'help_message_test',
    'native_objects_test',
    'path_length_test',
    'pch_test',
    'pnacl_compress_test',
    'pnacl_ld_options_test',
    'strip_test',
    'translate_options_test',
]

# The tested modules live in the parent directory. Set up the import path
# accordingly.
my_dir = os.path.dirname(sys.argv[0])
sys.path.append(os.path.join(my_dir, '..'))

suite = unittest.TestLoader().loadTestsFromNames(MODULES)
result = unittest.TextTestRunner(verbosity=2).run(suite)
if result.wasSuccessful():
  sys.exit(0)
else:
  sys.exit(1)
