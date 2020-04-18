#!/usr/bin/env vpython
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compile DevTools frontend code with Closure compiler.

This script wraps devtools/scripts/compile_frontend.py.
DevTools bot kicks this script.
"""

import os
import sys

sys.path.append(os.path.join(
    os.path.dirname(__file__), '..', '..', 'blink', 'renderer', 'devtools', 'scripts'))
import compile_frontend

if __name__ == '__main__':
    sys.exit(compile_frontend.main())
