/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/nacl_thread.h"
#include "native_client/src/untrusted/nacl/tls.h"

/*
 * The linker generates calls to __nacl_add_tp() for TLS accesses,
 * when optimizing from Dynamic TLS models to the Exec ones.
 * This is primarily used for x86-64.
 * See the nacl-specific TLS optimizations in binutils:
 * http://codereview.chromium.org/8161011/diff/7001/bfd/elf64-x86-64.c
 *
 * NOTE: In the glibc build, this is defined in ld.so rather than here.
 *
 * More detailed comment from glibc repo copied verbatim:
 *
 * NaCl imposes severe limitations on call instruction alignment. This implies
 * we have to keep calls on the same position when doing TLS rewrites.
 * Unfortunately, dynamic TLS models call __tls_get_addr at the end, while exec
 * TLS models call __nacl_read_tp at the beginning.
 *
 * __nacl_add_tp returns thread pointer plus an offset passed in the first
 * argument. Using __nacl_add_tp we can create exec TLS models code sequences
 * with call at the end, thus enabling dynamic to exec TLS rewrites.
 *
 * For example, __nacl_read_tp initial exec code sequence is:
 *   call __nacl_read_tp
 *   add x@gottpoff(%rip),%eax
 *
 * And __nacl_add_tp equivalent is:
 *   mov x@gottpoff(%rip),%edi  #  first integer argument is passed in %rdi
 *   call __nacl_add_tp
 */
void *__nacl_add_tp(ptrdiff_t off) {
  return (char *) nacl_tls_get() + off;
}
