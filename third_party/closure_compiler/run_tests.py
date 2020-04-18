#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import compiler_test
import processor_test


for test_module in [compiler_test, processor_test]:
  test_module.unittest.main(test_module)
