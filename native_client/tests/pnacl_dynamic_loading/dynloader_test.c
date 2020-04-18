/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/pnacl_dynloader/dynloader.h"
#include "native_client/tests/pnacl_dynamic_loading/test_pso.h"


static void assert_buffer_is_zeroed(const char *array, size_t size) {
  size_t i;
  for (i = 0; i < size; i++)
    ASSERT_EQ(array[i], 0);
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: dynloader_test <ELF file>...\n");
    return 1;
  }
  const char *test_dso_file = argv[1];
  const char *data_only_dso_file = argv[2];
  const char *data_only_dso_largebss_file = argv[3];

  void *pso_root;
  int err;

  printf("Testing %s...\n", test_dso_file);
  err = pnacl_load_elf_file(test_dso_file, &pso_root);
  ASSERT_EQ(err, 0);
  struct test_pso_root *root = pso_root;
  int val = 1000000;
  int result = root->example_func(&val);
  ASSERT_EQ(result, 1001234);
  ASSERT_EQ(*root->get_var(), 2345);
  /* Test that a variable in the BSS is properly zero-initialized. */
  assert_buffer_is_zeroed(root->bss_var, BSS_VAR_SIZE);

  /* Test use of LLVM's memcpy intrinsic inside the PSO. */
  uint8_t src_buf[0x1000];
  uint8_t dest_buf[0x1000];
  memset(dest_buf, 0, sizeof(dest_buf));
  for (size_t i = 0; i < sizeof(src_buf); i++)
    src_buf[i] = i;
  void *memcpy_result = root->memcpy_example(dest_buf, src_buf,
                                             sizeof(dest_buf));
  ASSERT_EQ(memcpy_result, dest_buf);
  for (size_t i = 0; i < sizeof(dest_buf); i++)
    ASSERT_EQ(dest_buf[i], (uint8_t) i);

  /* Test use of division (e.g. via __divdi3) inside the PSO. */
  ASSERT_EQ(root->division_example(0x123456789abcdef1, 7),
            0x123456789abcdef1 / 7);

  /*
   * Each call to pnacl_load_elf_file() should create a fresh instantiation
   * of the PSO/DSO in memory.
   */
  printf("Testing loading DSO a second time...\n");
  void *pso_root2;
  err = pnacl_load_elf_file(test_dso_file, &pso_root2);
  ASSERT_EQ(err, 0);
  struct test_pso_root *root2 = pso_root2;
  ASSERT_NE(pso_root2, pso_root);
  ASSERT_NE(root2->get_var, root->get_var);
  ASSERT_NE(root2->get_var(), root->get_var());
  ASSERT_EQ(*root2->get_var(), 2345);

  printf("Testing %s...\n", data_only_dso_file);
  err = pnacl_load_elf_file(data_only_dso_file, &pso_root);
  ASSERT_EQ(err, 0);
  uint32_t *ptr = pso_root;
  ASSERT_EQ(ptr[0], 0x44332211);
  ASSERT_EQ(ptr[1], 0xDDCCBBAA);

  printf("Testing %s...\n", data_only_dso_largebss_file);
  err = pnacl_load_elf_file(data_only_dso_largebss_file, &pso_root);
  ASSERT_EQ(err, 0);
  char **bss_var_ptr = pso_root;
  assert_buffer_is_zeroed(*bss_var_ptr, 1000000);

  return 0;
}
