/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>

/* Implementation of srandom() in terms of srand() (which is part of newlib) */

void srandom(unsigned int seed) {
  srand(seed);
}
