/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/src/trusted/service_runtime/include/sys/nacl_test_crash.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"


typedef int (*TYPE_nacl_test_syscall_1)(void);
typedef int (*TYPE_nacl_test_syscall_2)(void);


char g_stack[0x10000];

static int look_up_crash_type(const char *name) {
#define MAP_NAME(type) if (strcmp(name, #type) == 0) { return type; }
  MAP_NAME(NACL_TEST_CRASH_MEMORY);
  MAP_NAME(NACL_TEST_CRASH_LOG_FATAL);
  MAP_NAME(NACL_TEST_CRASH_CHECK_FAILURE);
#undef MAP_NAME

  fprintf(stderr, "Unknown crash type: \"%s\"\n", name);
  exit(1);
}

static void exception_handler(struct NaClExceptionContext *context) {
  fprintf(stderr, "exception_handler() called unexpectedly\n");
  _exit(1);
}

static void register_exception_handler(void) {
  int rc = nacl_exception_set_handler(exception_handler);
  assert(rc == 0);
  rc = nacl_exception_set_stack(g_stack, sizeof(g_stack));
  assert(rc == 0);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <crash-type>\n", argv[0]);
    return 1;
  }
  char *crash_type = argv[1];
  if (strcmp(crash_type, "NACL_TEST_CRASH_JUMP_TO_ZERO") == 0) {
    register_exception_handler();
    NACL_SYSCALL(test_syscall_1)();
  } else if (strcmp(crash_type, "NACL_TEST_CRASH_JUMP_INTO_SANDBOX") == 0) {
    register_exception_handler();
    NACL_SYSCALL(test_syscall_2)();
  } else {
    NACL_SYSCALL(test_crash)(look_up_crash_type(crash_type));
  }
  return 1;
}
