# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Add the path of blinkpy to sys.path.

You don't need to call a function to do it. You need just import this. e.g.
    import webkitpy.common.add_blinkpy  # pylint: disable=unused-import
    or
    from webkitpy.common import add_blinkpy  # pylint: disable=unused-import

This is a transitional solution to handle both of blinkpy and webkitpy. We'll
remove this file when we finish moving webkitpy to blinkpy.
"""

import os
import sys

# Without abspath(), PathFinder can't find chromium_base correctly.
sys.path.append(os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..',
                 'blink', 'tools')))
