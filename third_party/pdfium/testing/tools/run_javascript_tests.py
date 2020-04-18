#!/usr/bin/env python
# Copyright 2015 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

import test_runner

def main():
  runner = test_runner.TestRunner('javascript')
  return runner.Run()

if __name__ == '__main__':
  sys.exit(main())
