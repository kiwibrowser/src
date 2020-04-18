/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

int main(void) {
  /* Should never reach here: nexe should be rejected by sel_ldr. */
  return 17;
}
