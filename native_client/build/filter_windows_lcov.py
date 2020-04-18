#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
import sys

for line in sys.stdin:
  if line.startswith('SF:'):
    filename = line[3:].strip()
    p = subprocess.Popen(['cygpath', filename], stdout=subprocess.PIPE)
    (p_stdout, _) = p.communicate()
    print 'SF:' + p_stdout.strip()
  else:
    print line.strip()
