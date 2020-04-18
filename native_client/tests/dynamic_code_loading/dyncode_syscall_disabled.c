/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include <nacl/nacl_dyncode.h>

#include "native_client/tests/inbrowser_test_runner/test_runner.h"

void test_dyncode_create(void) {
  assert(nacl_dyncode_create(NULL, NULL, 0) == -1);
  assert(errno == ENOSYS);
}

void test_dyncode_modify(void) {
  assert(nacl_dyncode_modify(NULL, NULL, 0) == -1);
  assert(errno == ENOSYS);
}

void test_dyncode_delete(void) {
  assert(nacl_dyncode_delete(NULL, 0) == -1);
  assert(errno == ENOSYS);
}

void run_test(const char *test_name, void (*test_func)(void)) {
  printf("Running %s...\n", test_name);
  test_func();
}

#define RUN_TEST(test_func) (run_test(#test_func, test_func))

int TestMain(void) {
  RUN_TEST(test_dyncode_create);
  RUN_TEST(test_dyncode_modify);
  RUN_TEST(test_dyncode_delete);

  return 0;
}

int main(void) {
  return RunTests(TestMain);
}
