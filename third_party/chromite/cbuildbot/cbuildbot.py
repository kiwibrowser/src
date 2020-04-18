#!/usr/bin/env python2
# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(ferringb): remove this as soon as depot_tools is gutted of its
# import logic, and is just a re-exec.

"""Dynamic wrapper to invoke cbuildbot with standardized import paths."""

from __future__ import print_function

import os
import sys

def main():
  # Bypass all of chromite_wrappers attempted 'help', and execve to the actual
  # cbuildbot wrapper/helper chromite has.
  location = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
  location = os.path.join(location, 'bin', 'cbuildbot')
  os.execv(location, [location] + sys.argv[1:])

if __name__ == '__main__':
  main()
