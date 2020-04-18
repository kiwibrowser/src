/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/mman.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/tests/irt_ext/error_report.h"
#include "native_client/tests/irt_ext/libc/libc_test.h"
#include "native_client/tests/irt_ext/mem_calls.h"

typedef int (*TYPE_mem_test)(struct mem_calls_environment *env);

static int do_mmap_test(struct mem_calls_environment *env) {
  mmap(NULL, 0, 0, 0, 0, 0);
  if (env->num_mmap_calls != 1) {
    irt_ext_test_print("do_mmap_test: mmap() call was not intercepted.\n");
    return 1;
  }

  return 0;
}

static int do_munmap_test(struct mem_calls_environment *env) {
  munmap(NULL, 0);
  if (env->num_munmap_calls != 1) {
    irt_ext_test_print("do_munmap_test: munmap() call was not intercepted.\n");
    return 1;
  }

  return 0;
}

static int do_mprotect_test(struct mem_calls_environment *env) {
  mprotect(NULL, 0, 0);
  if (env->num_mprotect_calls != 1) {
    irt_ext_test_print("do_mprotect_test: mprotect() call was not"
                       " intercepted.\n");
    return 1;
  }

  return 0;
}

static const TYPE_mem_test g_mem_tests[] = {
  do_mmap_test,
  do_munmap_test,
  do_mprotect_test,
};

static void setup(struct mem_calls_environment *env) {
  init_mem_calls_environment(env);
  activate_mem_calls_env(env);
}

static void teardown(void) {
  deactivate_mem_calls_env();
}

DEFINE_TEST(Mem, g_mem_tests, struct mem_calls_environment, setup, teardown)
