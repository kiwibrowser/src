/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Used to provide stack traceback support in NaCl modules in the debugger.
 * When the application is compiled with -finstrument functions, stack tracing
 * is turned on.  This module maintains a stack of function entry addresses.
 * The state of the stack trace can be printed at any point by calling
 * __nacl_dump_stack_trace.
 */

#include <stdio.h>

/*
 * Because of the use of static variables, this module is NOT THREAD SAFE.
 * TODO(sehr): use thread local variables for this, etc., if possible.
 */
#define STACK_MAX_DEPTH 1024
static void* stack[STACK_MAX_DEPTH];

static int top_loc = 0;

static void push(void* function_address) {
  if (top_loc < STACK_MAX_DEPTH) {
    stack[top_loc] = function_address;
  }
  /*
   * Although we have a static limit on the number of function addresses that
   * may be pushed, we keep track of the depth even beyond that.  Deeply
   * recursive chains will look confused, and may make top_loc wrap around
   * (to a negative number) in unusual cases, but we can improve this later.
   */
  ++top_loc;
}

static void pop(void) {
  /*
   * It's possible the calls and returns won't match.  Don't decrement past the
   * bottom of the stack.
   */
  if (top_loc > 0) {
    --top_loc;
  }
}

void __cyg_profile_func_enter(void* this_fn, void* call_site) {
  push(this_fn);
}

void __cyg_profile_func_exit(void* this_fn, void* call_site) {
  pop();
}

void __nacl_dump_stack_trace(void) {
  int i;
  int top_to_display;
  if (top_loc > STACK_MAX_DEPTH) {
    printf("Overflow: %d calls were not recorded.\n",
           top_loc - STACK_MAX_DEPTH);
    top_to_display = STACK_MAX_DEPTH;
  } else {
    top_to_display = top_loc;
  }
  for (i = top_to_display - 1; i >= 0; i--) {
    printf("%d: %p\n", top_loc - 1 - i, stack[i]);
  }
}
