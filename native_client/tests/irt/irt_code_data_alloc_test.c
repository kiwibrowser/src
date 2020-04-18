/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/untrusted/irt/irt.h"

/* Default distance between the start of code and start of data. */
#define CODE_DATA_SEP 0x10000000

void test_code_data_allocations(void) {
  const size_t page = getpagesize();
  struct nacl_irt_code_data_alloc alloc;
  int rc;
  uintptr_t begin1;
  uintptr_t begin2;
  uintptr_t begin3;
  uintptr_t empty_data_begin;
  uintptr_t hint_begin;
  uintptr_t diff_sep_begin;
  void *data_probe;

  rc = nacl_interface_query(NACL_IRT_CODE_DATA_ALLOC_v0_1,
                            &alloc, sizeof alloc);
  assert(rc == sizeof alloc);

  rc = alloc.allocate_code_data(0, page, CODE_DATA_SEP, page, &begin1);
  assert(rc == 0);

  rc = alloc.allocate_code_data(0, page, CODE_DATA_SEP, 2 * page, &begin2);
  assert(rc == 0);

  rc = alloc.allocate_code_data(0, page, CODE_DATA_SEP, page, &begin3);
  assert(rc == 0);

  /*
   * Expected Allocations:
   *
   * |*******|*******|*******|*******|**|*******|*******|*******|*******|
   * | Code1 | Code2 | Blank | Code3 |  | Data1 | Data2 | Data2 | Data3 |
   * |*******|*******|*******|*******|**|*******|*******|*******|*******|
   */
  printf("Begin1: %x\n", (unsigned int)begin1);
  printf("Begin2: %x\n", (unsigned int)begin2);
  printf("Begin3: %x\n", (unsigned int)begin3);
  assert(begin2 == (begin1 + page));
  assert(begin3 == (begin2 + 2 * page));

  /*
   * Verify the data segments are reserved as expected using mmap.
   */
  data_probe = mmap((void *) (begin3 + CODE_DATA_SEP),
                    page,
                    PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1,
                    (off_t) 0);
  assert(data_probe == (void *) (begin3 + CODE_DATA_SEP + page));

  /*
   * Validate empty data sizes are allowed.
   */
  rc = alloc.allocate_code_data(0, page, 0, 0, &empty_data_begin);
  assert(rc == 0);

  /*
   * Supplying a hint should override where the allocation begins.
   */
  rc = alloc.allocate_code_data(begin3 + 10 * page, page, CODE_DATA_SEP,
                                page, &hint_begin);
  assert(rc == 0);
  assert(hint_begin == begin3 + 10 * page);

  /*
   * Validate larger code and data separations are respected.
   */
  rc = alloc.allocate_code_data(0, page, CODE_DATA_SEP + page,
                                page, &diff_sep_begin);
  assert(rc == 0);

  data_probe = mmap((void *) (diff_sep_begin + CODE_DATA_SEP + page),
                    page,
                    PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1,
                    (off_t) 0);
  assert(data_probe == (void *) (diff_sep_begin + CODE_DATA_SEP + page + page));
}

void test_bad_code_data_allocations(void) {
  const size_t page = getpagesize();
  struct nacl_irt_code_data_alloc alloc;
  int rc;
  uintptr_t err_begin;

  rc = nacl_interface_query(NACL_IRT_CODE_DATA_ALLOC_v0_1,
                            &alloc, sizeof alloc);
  assert(rc == sizeof alloc);

  /*
   * Code size cannot be empty.
   */
  rc = alloc.allocate_code_data(0, 0, CODE_DATA_SEP, page, &err_begin);
  assert(rc == EINVAL);

  /*
   * Hint must be page aligned.
   */
  rc = alloc.allocate_code_data(1, page, CODE_DATA_SEP, page, &err_begin);
  assert(rc == EINVAL);

  /*
   * Code size must be page aligned.
   */
  rc = alloc.allocate_code_data(0, page + 1, CODE_DATA_SEP, page, &err_begin);
  assert(rc == EINVAL);

  /*
   * Data offset must be page aligned.
   */
  rc = alloc.allocate_code_data(0, page, CODE_DATA_SEP + 1, page, &err_begin);
  assert(rc == EINVAL);

  /*
   * Data size must be page aligned.
   */
  rc = alloc.allocate_code_data(0, page, CODE_DATA_SEP, page + 1, &err_begin);
  assert(rc == EINVAL);

  /*
   * Data offset must be larger than the CODE_DATA_SEP.
   */
  rc = alloc.allocate_code_data(0, page, CODE_DATA_SEP - page, page,
                                &err_begin);
  assert(rc == EINVAL);
}


int main(void) {
  test_code_data_allocations();
  test_bad_code_data_allocations();
  return 0;
}
