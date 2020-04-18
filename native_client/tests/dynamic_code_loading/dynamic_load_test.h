/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_DYNAMIC_CODE_LOADING_DYNAMIC_LOAD_TEST_H_
#define NATIVE_CLIENT_TESTS_DYNAMIC_CODE_LOADING_DYNAMIC_LOAD_TEST_H_

#include <stdint.h>
#include <stdlib.h>

#if defined(__i386__) || defined(__x86_64__)
#define NACL_BUNDLE_SIZE  32
#elif defined(__arm__)
#define NACL_BUNDLE_SIZE  16
#else
#error "Unknown Platform"
#endif

#define PAGE_SIZE 0x10000

char *allocate_code_space(int pages);

int nacl_load_code(void *dest, void *src, int size);

void fill_nops(uint8_t *data, size_t size);

void test_threaded_loads(void);

void test_threaded_delete(void);

#endif  /* NATIVE_CLIENT_TESTS_DYNAMIC_CODE_LOADING_DYNAMIC_LOAD_TEST_H_ */
