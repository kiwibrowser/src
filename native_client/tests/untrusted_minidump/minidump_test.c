/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/include/nacl/nacl_minidump.h"
#include "native_client/src/untrusted/minidump_generator/build_id.h"
#include "native_client/tests/untrusted_minidump/minidump_test_lib.h"

#if defined(__GLIBC__)
# define DYNAMIC_LOADING_SUPPORT 1
#else
# define DYNAMIC_LOADING_SUPPORT 0
#endif


const char *g_minidump_filename;


static void crash_callback(const void *minidump_data, size_t size) {
  assert(size != 0);
  FILE *fp = fopen(g_minidump_filename, "wb");
  assert(fp != NULL);
  int written = fwrite(minidump_data, 1, size, fp);
  assert(written == size);
  int rc = fclose(fp);
  assert(rc == 0);

  fprintf(stderr, "** intended_exit_status=0\n");
  exit(0);
}

__attribute__((noinline))
void crash(void) {
  *(volatile int *) 1 = 1;
}

/* Use some nested function calls to test stack backtracing. */
__attribute__((noinline))
void crash_wrapper1(int crash_in_lib) {
  if (crash_in_lib) {
    lib_crash();
  } else {
    crash();
  }
}

__attribute__((noinline))
void crash_wrapper2(int crash_in_lib) {
  crash_wrapper1(crash_in_lib);
}

static void test_nacl_get_build_id(void) {
#if !DYNAMIC_LOADING_SUPPORT
  const char *id_data;
  size_t id_size;
  int got_build_id = nacl_get_build_id(&id_data, &id_size);
  assert(got_build_id);
  assert(id_data != NULL);
  assert(id_size == 20);  /* ld uses SHA1 hash by default. */
#endif
}

int main(int argc, char **argv) {
  assert(argc == 4);
  g_minidump_filename = argv[1];
  int crash_in_lib = atoi(argv[2]);
  int modules_live = atoi(argv[3]);

  test_nacl_get_build_id();

  nacl_minidump_set_callback(crash_callback);
  nacl_minidump_set_module_name("minidump_test.nexe");
  nacl_minidump_register_crash_handler();
  nacl_minidump_snapshot_module_list();
  if (!modules_live) {
    /* Verify that the module list can be cleared. */
    nacl_minidump_clear_module_list();
  }

  /* Cause crash. */
  crash_wrapper2(crash_in_lib);

  /* Should not reach here. */
  return 1;
}
