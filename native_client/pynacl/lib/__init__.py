#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Unfortunately python does not have a good way to deal with module name
# clashes. The easiest way to get around it is to use a layer of indirection.
# For example, if pynacl.platform needs to use the python root platform module,
# it should import lib instead and use lib.platform.

# List name clashed modules here in alphabetical order
import platform
