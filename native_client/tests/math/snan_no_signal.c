/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test that at startup, signaling NaNs do not trap for PNaCl.
 *
 * This is a requirement for PNaCl version 1 because there is no
 * portable representation of signaling NaNs vs quiet NaNs,
 * and we don't want different trapping behavior:
 * http://code.google.com/p/nativeclient/issues/detail?id=3536
 *
 * This is currently ensured by the way the runtime sets up the
 * untrusted state.
 *
 * We don't test that trapping is possible at all, since it is not always
 * possible. For example, ARM VFPv3 does not support trapping (only the
 * VFPv3U variant does).
 */

extern void try_operations_with_snans(void);

void try_c_level_operations(void) {
  double volatile n = __builtin_nan("");
  double volatile sn = __builtin_nans("");
  n = n + n;
  sn = sn + sn;
}

int main(int argc, char *argv[]) {
  /*
   * Call assembly routines where we know the exact sNaN bit pattern
   * for the architecture.
   */
  try_operations_with_snans();
  /*
   * Try some C-level constructs where the qNaN vs sNaN bit-pattern
   * might be reversed.  Since we test both patterns, one will be
   * the sNaN pattern.
   */
  try_c_level_operations();
  return 0;
}
