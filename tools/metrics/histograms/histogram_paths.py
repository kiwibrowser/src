# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Paths to description XML files in this directory."""

import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'common'))
import path_util

HISTOGRAMS_XML = path_util.GetInputFile(
    'tools/metrics/histograms/histograms.xml')

ENUMS_XML = path_util.GetInputFile(
    'tools/metrics/histograms/enums.xml')

ALL_XMLS = [HISTOGRAMS_XML, ENUMS_XML]
