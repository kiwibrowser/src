/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * gc_noinst.c : Functions that shouldn't be instrumented by the compiler
 */

#include "native_client/tests/compiler_thread_suspension/gc_inst.h"


__thread unsigned int thread_suspend_if_needed_count = 0;

/* TODO(elijahtaylor): This will need to be changed
 *  when the compiler instrumentation changes.
 */
volatile int __nacl_thread_suspension_needed = 1;

void __nacl_suspend_thread_if_needed(void) {
  thread_suspend_if_needed_count++;
}
