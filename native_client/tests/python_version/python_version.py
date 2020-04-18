#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

print "CURRENT CONFIG"
print sys.version


pver = sys.version.split()[0]
print "CURRENT VERSION"
print pver

if pver < "2.4":
  print "ERROR Python version is too old"
  sys.exit(-1)

# NOTE: we may want to guard against this as well
#if pver > "2.6":
#  print "ERROR Python version is too new"
#  sys.exit(-1)

print "Python version is OK"
sys.exit(0)
