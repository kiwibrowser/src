/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Dummy file that goes with dwarf_local_var.c to introduce linking
 * and LTO into the equation.
 */

int bar(int x) {
  if (x > 0)
    return x;
  else
    return -1;
}
