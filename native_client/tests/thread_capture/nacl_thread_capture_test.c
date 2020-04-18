/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This is untrusted code to test the thread capture mitigation
 * mechanism.  In order for this to work, the (undocumented,
 * test-only) syscall NaCl_test_capture, which is installed at
 * NACL_sys_test_syscall_1 when NACL_FAULT_INJECTION is used to inject
 * this code via the InjectThreadCaptureTest fault injection control,
 * must be active.
 *
 * NB: this test syscall can permit sandbox escape.
 *
 * The idea is that this test will write out something to show
 * liveness, then invoke NACL_sys_test_syscall_1 to pretend to be a
 * captured non-NaCl thread.  Thereafter, any non-fast-path syscall
 * (including write) should cause a segfault -- and we do a write(2).
 *
 * Note that we try to avoid using newlib or glibc for anything more
 * complicated than the basic startup code or other, clearly
 * side-effect-free functions.  The reason for this is that these
 * libraries may use other syscalls internally, e.g., for memory
 * allocation (mmap/sbrk) for standard I/O buffers or for TLS access
 * (__nacl_read_tp), which uses %gs (on x86-32) or use the fast-path
 * syscall to get the TLS base (x86-64).  Since this test should be
 * portable, this means that we cannot depend on stdio functions --
 * otherwise the test will crash due to using some random syscall,
 * rather than at the precise syscall that we expect.  (This is only
 * relevant if we are looking at the nexe from a debugger -- wrt the
 * apoptosis mechanism for detecting thread capture, the system will
 * still crash the NaCl application.)
 *
 * The test framework (via test injection into sel_ldr) expects that
 * result -- this is a death test.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"


void my_write(int desc, char const *buffer, size_t nbytes) {
  (void) NACL_SYSCALL(write)(desc, buffer, nbytes);
}

void my_puts(char const *str) {
  size_t len = strlen(str);
  my_write(1, str, len);
}

int main(void) {
  my_puts("Hello world.\n");
  my_puts("Goodbye cruel world.\n");
  ((void (*)(void)) NACL_SYSCALL_ADDR(NACL_sys_test_syscall_1))();
  my_puts("Pull trigger...\n");  /* the write syscall will do it */
  /* should be truly dead code below */
  my_puts("AFTER DEATH EXPERIENCE!\n");
  return 1;
}
