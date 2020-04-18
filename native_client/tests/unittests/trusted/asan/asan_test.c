/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Sanity test for AddressSanitizer. */

volatile int idx = 2;
volatile char a[1] = {0};

int main(void) {
  return a[idx];
}
