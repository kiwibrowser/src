# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import inspect
import os
import sys

from telemetry import story

from py_utils import discover

# Import all submodules' StorySet classes.
start_dir = os.path.dirname(os.path.abspath(__file__))
top_level_dir = os.path.abspath(os.path.join(start_dir, os.pardir, os.pardir))
base_class = story.StorySet
for cls in discover.DiscoverClasses(
    start_dir, top_level_dir, base_class).values():
  setattr(sys.modules[__name__], cls.__name__, cls)

