#!/usr/bin/env python
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper around
   third_party/blink/tools/run_web_tests.py"""
import os
import subprocess
import sys

def main():
    print '\n    Please use third_party/blink/tools/run_web_tests.*.  ' \
        'This command will be removed.\n'
    src_dir = os.path.abspath(os.path.join(sys.path[0], '..', '..'))
    script_dir=os.path.join(src_dir, "third_party", "blink", "tools")
    script = os.path.join(script_dir, 'run_web_tests.py')
    cmd = [sys.executable, script] + sys.argv[1:]
    return subprocess.call(cmd)

if __name__ == '__main__':
    sys.exit(main())
