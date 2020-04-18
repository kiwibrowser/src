# -*- python -*-
# Copyright (c) 2010 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

f = open("premade_text_file", "w")
for i in range(0, 32):
  f.write(chr(ord('A')+i))
f.close()

f = open("premade_binary_file", "wb")
for i in range(0, 32):
  f.write(chr(i))
f.close()
