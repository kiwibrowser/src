# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""All things Chrome OS signing related"""

from __future__ import print_function

import os

from chromite.lib import constants


SIGNING_DIR = os.path.join(constants.CHROMITE_DIR, 'signing')
INPUT_INSN_DIR = os.path.join(constants.SOURCE_ROOT, 'crostools',
                              'signer_instructions')
TEST_INPUT_INSN_DIR = os.path.join(SIGNING_DIR, 'signer_instructions')
