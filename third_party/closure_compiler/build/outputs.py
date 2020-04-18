#!/usr/bin/python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys


def GetPath(resource):
  src_root = os.path.join(os.path.dirname(__file__), "..", "..", "..")
  resource_path = os.path.join(os.getcwd(), resource)
  return os.path.relpath(resource_path, src_root)


if __name__ == "__main__":
  print GetPath(sys.argv[1])
