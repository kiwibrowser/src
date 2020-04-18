/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>

int main(void) {
  void *buf = malloc(128 << 20);
  return buf == NULL;
}
