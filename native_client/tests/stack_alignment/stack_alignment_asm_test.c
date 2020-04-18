/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"


/*
 * This test checks that new threads are run with their stack pointer
 * suitably aligned, as per architecture-specific ABI requirements.
 */

/* We use 'volatile' because we spin waiting for these variables to be set. */
static volatile int32_t g_stack_in_use;
static char *volatile g_stack_ptr;
static char stack[0x1000];

#if defined(__i386__)

__asm__(
    ".pushsection .text, \"ax\", @progbits\n"
    "ThreadStartWrapper:\n"
    "push %esp\n"  /* Push argument */
    "call ThreadStart\n"
    ".popsection\n");

static const int kStackAlignment = 16;
static const int kStackPadBelowAlign = 4;

#elif defined(__x86_64__)

__asm__(
    ".pushsection .text, \"ax\", @progbits\n"
    "ThreadStartWrapper:\n"
    "movl %esp, %edi\n"  /* Set argument */
    "call ThreadStart\n"
    ".popsection\n");

static const int kStackAlignment = 16;
static const int kStackPadBelowAlign = 8;

#elif defined(__arm__)

__asm__(
    ".pushsection .text, \"ax\", %progbits\n"
    "ThreadStartWrapper:\n"
    "mov r0, sp\n"  /* Set argument */
    "nop; nop\n"  /* Padding to put the "bl" at the end of the bundle */
    "bl ThreadStart\n"
    ".popsection\n");

static const int kStackAlignment = 8;
static const int kStackPadBelowAlign = 0;

#elif defined(__mips__)

__asm__(
    ".pushsection .text, \"ax\", @progbits\n"
    ".set noreorder\n"
    ".global ThreadStartWrapper\n"
    "ThreadStartWrapper:\n"
    "move $a0, $sp\n"  /* Set argument. */
    "lui $t9, %hi(ThreadStart)\n"
    "bal ThreadStart\n"
    "addiu $t9, $t9, %lo(ThreadStart)\n"
    ".set reorder\n"
    ".popsection\n");

static const int kStackAlignment = 8;
static const int kStackPadBelowAlign = 0;

#else
# error Unsupported architecture
#endif

void ThreadStartWrapper(void);

void ThreadStart(char *stack_ptr) {
  /*
   * We do not have TLS set up in this thread, so we don't use libc
   * functions like assert() here.  Instead, we save stack_ptr and let
   * the main thread check it.
   */
  g_stack_ptr = stack_ptr;
  __libnacl_irt_thread.thread_exit((int32_t *) &g_stack_in_use);
}

int main(void) {
  __nc_initialize_interfaces();

  int offset;
  for (offset = 0; offset <= 32; offset++) {
    char *stack_top = stack + sizeof(stack) - offset;
    printf("Checking offset %i: stack_top=%p...\n", offset, (void *) stack_top);
    g_stack_ptr = NULL;
    g_stack_in_use = 1;
    void *dummy_tls = &dummy_tls;
    int rc = __libnacl_irt_thread.thread_create(ThreadStartWrapper, stack_top,
                                                dummy_tls);
    assert(rc == 0);
    /* Spin until the thread exits. */
    while (g_stack_in_use) {
      sched_yield();
    }
    printf("got g_stack_ptr=%p\n", (void *) g_stack_ptr);
    assert(g_stack_ptr <= stack_top);
    assert(((uintptr_t) g_stack_ptr + kStackPadBelowAlign) % kStackAlignment
           == 0);
  }

  return 0;
}
