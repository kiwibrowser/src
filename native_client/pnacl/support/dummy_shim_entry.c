/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

/*
 * Keep in sync with "native_client/src/untrusted/nacl/nacl_startup.h".
 */
extern void _start(uint32_t *info);

/*
 * This is the true entry point for PNaCl's untrusted code.
 * This is used for commandline nexes.  PPAPI browser-facing nexes may use
 * a different _pnacl_wrapper_start() that may shim the PPAPI interfaces.
 */
void _pnacl_wrapper_start(uint32_t *info) {
  _start(info);
}
