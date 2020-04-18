/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
__thread int tlsvar;

int check_set_tls(int expected, int value) {
  if (tlsvar != expected)
    abort();
  int oldval = tlsvar;
  tlsvar = value;
  return oldval;
}
