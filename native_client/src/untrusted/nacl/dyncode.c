/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/nacl/nacl_dyncode.h"

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/irt/irt.h"

/*
 * ABI table for underyling NaCl dyncode interfaces.
 * We set this up on demand.
 */
static struct nacl_irt_dyncode irt_dyncode;

/*
 * We don't do any locking here, but simultaneous calls are harmless enough.
 * They'll all be writing the same values to the same words.
 */
static void setup_irt_dyncode(void) {
  if (nacl_interface_query(NACL_IRT_DYNCODE_v0_1, &irt_dyncode,
                           sizeof(irt_dyncode)) != sizeof(irt_dyncode)) {
    static const char fail_msg[] =
        "IRT interface query failed for essential interface \""
        NACL_IRT_DYNCODE_v0_1 "\"!\n";
    write(2, fail_msg, sizeof(fail_msg) - 1);
    _exit(-1);
  }
}

int nacl_dyncode_create(void *dest, const void *src, size_t size) {
  if (NULL == irt_dyncode.dyncode_create)
    setup_irt_dyncode();
  int error = irt_dyncode.dyncode_create(dest, src, size);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}

int nacl_dyncode_modify(void *dest, const void *src, size_t size) {
  if (NULL == irt_dyncode.dyncode_modify)
    setup_irt_dyncode();
  int error = irt_dyncode.dyncode_modify(dest, src, size);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}

int nacl_dyncode_delete(void *dest, size_t size) {
  if (NULL == irt_dyncode.dyncode_delete)
    setup_irt_dyncode();
  int error = irt_dyncode.dyncode_delete(dest, size);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}
