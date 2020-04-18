/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <nacl/nacl_dyncode.h>

#include "native_client/tests/dynamic_code_loading/dynamic_segment.h"
#include "native_client/tests/dynamic_code_loading/templates.h"
#include "native_client/tests/inbrowser_test_runner/test_runner.h"

#if defined(__x86_64__)
#define BUF_SIZE 64
#else
#define BUF_SIZE 32
#endif

#define NACL_BUNDLE_SIZE  32
/*
 * TODO(bsy): get this value from the toolchain.  Get the toolchain
 * team to provide this value.
 */
#define NUM_BUNDLES_FOR_HLT 3

struct code_section {
  char *name;
  char *start;
  char *end;
};

struct code_section illegal_code_sections[] = {
  { "misaligned_replacement",
    &template_func_misaligned_replacement,
    &template_func_misaligned_replacement_end },
  { "illegal_register_replacement",
    &template_func_illegal_register_replacement,
    &template_func_illegal_register_replacement_end },
  { "illegal_guard_replacement",
    &template_func_illegal_guard_replacement,
    &template_func_illegal_guard_replacement_end },
  { "illegal_call_target",
    &template_func_illegal_call_target,
    &template_func_illegal_call_target_end },
  { "illegal_constant_replacement",
    &template_func_illegal_constant_replacement,
    &template_func_illegal_constant_replacement_end },
};

uint8_t *next_addr = NULL;

uint8_t *allocate_code_space(int pages) {
  uint8_t *addr;
  if (next_addr == NULL) {
    next_addr = (uint8_t *) DYNAMIC_CODE_SEGMENT_START;
  }
  addr = next_addr;
  next_addr += 0x10000 * pages;
  assert(next_addr < (uint8_t *) DYNAMIC_CODE_SEGMENT_END);
  return addr;
}

void fill_int32(uint8_t *data, size_t size, int32_t value) {
  int i;
  assert(size % 4 == 0);
  /*
   * All the archs we target supported unaligned word read/write, but
   * check that the pointer is aligned anyway.
   */
  assert(((uintptr_t) data) % 4 == 0);
  for (i = 0; i < size / 4; i++)
    ((uint32_t *) data)[i] = value;
}

void fill_nops(uint8_t *data, size_t size) {
#if defined(__i386__) || defined(__x86_64__)
  memset(data, 0x90, size); /* NOPs */
#elif defined(__arm__)
  fill_int32(data, size, 0xe1a00000); /* NOP (MOV r0, r0) */
#else
# error "Unknown arch"
#endif
}

/*
 * Getting the assembler to pad our code fragments in templates.S is
 * awkward because we have to output them in data mode, in which the
 * assembler wants to output zeroes instead of NOPs for padding.
 */
void copy_and_pad_fragment(void *dest,
                           int dest_size,
                           const char *fragment_start,
                           const char *fragment_end) {
  int fragment_size = fragment_end - fragment_start;
  assert(dest_size % 32 == 0);
  assert(fragment_size <= dest_size);
  fill_nops(dest, dest_size);
  memcpy(dest, fragment_start, fragment_size);
}

/* Check that we can dynamically rewrite code. */
void test_replacing_code(void) {
  uint8_t *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;
  int (*func)(void);

  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);
  rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
  assert(rc == 0);
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_OLD);

  /* write replacement to the same location */
  copy_and_pad_fragment(buf, sizeof(buf), &template_func_replacement,
                                          &template_func_replacement_end);
  rc = nacl_dyncode_modify(load_area, buf, sizeof(buf));
  assert(rc == 0);
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_NEW);
}


/* Check that we can dynamically rewrite code. */
void test_replacing_code_unaligned(void) {
  uint8_t *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int first_diff = 0;
  int rc;
  int (*func)(void);

  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);
  rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
  assert(rc == 0);
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_OLD);

  /* write replacement to the same location, unaligned */
  copy_and_pad_fragment(buf, sizeof(buf), &template_func_replacement,
                                          &template_func_replacement_end);
  /* we find first byte where old and new code differs */
  while (buf[first_diff] == load_area[first_diff] && first_diff < sizeof buf) {
    first_diff++;
  }
  /* and check, that there is some data in common, and some different */
  assert(first_diff > 0 && first_diff < sizeof(buf));
  rc = nacl_dyncode_modify(load_area+first_diff, buf+first_diff,
                           sizeof(buf)-first_diff);
  assert(rc == 0);
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_NEW);
}

#if defined(__i386__) || defined(__x86_64__)
/* Check that we can rewrite instruction that crosses align boundaries. */
void test_replacing_code_slowpaths(void) {
  uint8_t *load_area = allocate_code_space(1);
  uint8_t buf[NACL_BUNDLE_SIZE];
  size_t size;
  int rc;
  /* Offsets to copy an instruction to. */
  int off1, off2, off3;

  fill_nops(buf, sizeof(buf));
  size = (size_t) (&template_instr_end - &template_instr);
  assert(size <= 5);
  off1 = 4 - size + 1; /* Cross 4 byte boundary */
  off2 = 24 - size + 1; /* Cross 8 byte boundary */
  off3 = 16 - size + 1; /* Cross 16 byte boundary */
  memcpy(buf + off1, &template_instr, size);
  memcpy(buf + off2, &template_instr, size);
  memcpy(buf + off3, &template_instr, size);
  rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
  assert(rc == 0);

  memcpy(buf + off1, &template_instr_replace, size);
  rc = nacl_dyncode_modify(load_area + off1, buf + off1, size);
  assert(rc == 0);
  assert(memcmp(buf + off1, load_area + off1, size) == 0);

  memcpy(buf + off2, &template_instr_replace, size);
  rc = nacl_dyncode_modify(load_area + off2, buf + off2, size);
  assert(rc == 0);
  assert(memcmp(buf + off2, load_area + off2, size) == 0);

  memcpy(buf + off3, &template_instr_replace, size);
  rc = nacl_dyncode_modify(load_area + off3, buf + off3, size);
  assert(rc == 0);
  assert(memcmp(buf + off3, load_area + off3, size) == 0);
}
#endif

/* Check code replacement constraints */
void test_illegal_code_replacment(void) {
  uint8_t *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;
  int i;
  int (*func)(void);

  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);
  rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
  assert(rc == 0);
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_OLD);

  for (i = 0;
       i < (sizeof(illegal_code_sections) / sizeof(struct code_section));
       i++) {
    printf("\t%s\n", illegal_code_sections[i].name);

    /* write illegal replacement to the same location */
    copy_and_pad_fragment(buf, sizeof(buf), illegal_code_sections[i].start,
                                            illegal_code_sections[i].end);
    rc = nacl_dyncode_modify(load_area, buf, sizeof(buf));
    assert(rc != 0);
    func = (int (*)(void)) (uintptr_t) load_area;
    rc = func();
    assert(rc == MARKER_OLD);
  }
}

void test_external_jump_target_replacement(void) {
  uint8_t *load_area = allocate_code_space(1);
  /* BUF_SIZE * 2 because this function necessarily has an extra bundle. */
  uint8_t buf[BUF_SIZE * 2];
  int rc;
  int (*func)(void);
  const int kNaClBundleSize = NACL_BUNDLE_SIZE;

  copy_and_pad_fragment(buf, sizeof(buf),
                        &template_func_external_jump_target,
                        &template_func_external_jump_target_end);

  rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
  assert(rc == 0);
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_OLD);

  copy_and_pad_fragment(buf, sizeof(buf),
                        &template_func_external_jump_target_replace,
                        &template_func_external_jump_target_replace_end);
  /* Only copy one bundle so we can test an unaligned external jump target */
  rc = nacl_dyncode_modify(load_area, buf, kNaClBundleSize);
  assert(rc == 0);
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_NEW);
}

#if defined(__i386__) || defined(__x86_64__)
void test_jump_into_super_inst_create(void) {
  uint8_t *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;

  /* A direct jump into a bundle is invalid. */
  copy_and_pad_fragment(buf, sizeof(buf), &jump_into_super_inst_modified,
                        &jump_into_super_inst_modified_end);
  rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
  assert(rc != 0);
  assert(errno == EINVAL);
}

int test_simle_replacement(const char *fragment1, const char *fragment1_end,
                           const char *fragment2, const char *fragment2_end) {
  uint8_t *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;

  /* The original version is fine. */
  copy_and_pad_fragment(buf, sizeof(buf), fragment1, fragment1_end);
  rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
  assert(rc == 0);

  copy_and_pad_fragment(buf, sizeof(buf), fragment2, fragment2_end);
  rc = nacl_dyncode_modify(load_area, buf, sizeof(buf));
  return rc;
}

void test_start_with_super_inst_replace(void) {
  int rc;

  /*
   * Replace the code with itself.  This makes sure that replacement code can
   * start with a super instruction.
   */
  rc = test_simle_replacement(&jump_into_super_inst_original,
                              &jump_into_super_inst_original_end,
                              &jump_into_super_inst_original,
                              &jump_into_super_inst_original_end);
  assert(rc == 0);
}

#if defined(__i386__)
void test_delete_superinstruction(void) {
  int rc;

  /* Replace the superinstruction with normal instruction of the same size. */
  rc = test_simle_replacement(&delete_superinstruction,
                              &delete_superinstruction_end,
                              &delete_superinstruction_replace,
                              &delete_superinstruction_replace_end);
  assert(rc != 0);
  assert(errno == EINVAL);
}

void test_delete_superinstruction_split(void) {
  int rc;

  /*
   * Replace the superinstruction with couple of normal instruction of the same
   * size as atomic instructions of the superinstruction.
   */
  rc = test_simle_replacement(&delete_superinstruction_split,
                              &delete_superinstruction_split_end,
                              &delete_superinstruction_split_replace,
                              &delete_superinstruction_split_replace_end);
  assert(rc != 0);
  assert(errno == EINVAL);
}

void test_create_superinstruction(void) {
  int rc;

  /* Replace normal instruction with the superinstruction of the same size. */
  rc = test_simle_replacement(&create_superinstruction_split,
                              &create_superinstruction_split_end,
                              &create_superinstruction_split_replace,
                              &create_superinstruction_split_replace_end);
  assert(rc != 0);
  assert(errno == EINVAL);
}

void test_create_superinstruction_split(void) {
  int rc;

  /*
   * Replace couple of normal instruction which have sizes identical to the size
   * of atomic instructions in the superinstruction with said superinstruction.
   */
  rc = test_simle_replacement(&create_superinstruction,
                              &create_superinstruction_end,
                              &create_superinstruction_replace,
                              &create_superinstruction_replace_end);
  assert(rc != 0);
  assert(errno == EINVAL);
}

void test_change_boundaris_first_instructions(void) {
  int rc;

  /*
   * Replace the code with the code which shifts boundary between first and
   * second instruction.
   */
  rc = test_simle_replacement(
      &change_boundaries_first_instructions,
      &change_boundaries_first_instructions_end,
      &change_boundaries_first_instructions_replace,
      &change_boundaries_first_instructions_replace_end);
  assert(rc != 0);
  assert(errno == EINVAL);
}

void test_change_boundaris_last_instructions(void) {
  int rc;

  /*
   * Replace the code with the code which shifts boundary between last and next
   * to last instructions.
   */
  rc = test_simle_replacement(&change_boundaries_last_instructions,
                              &change_boundaries_last_instructions_end,
                              &change_boundaries_last_instructions_replace,
                              &change_boundaries_last_instructions_replace_end);
  assert(rc != 0);
  assert(errno == EINVAL);
}
#endif

void test_jump_into_super_inst_replace(void) {
  int rc;

  /*
   * The modified version cannot be used as a replacement.
   * See: http://code.google.com/p/nativeclient/issues/detail?id=2563
   */
  rc = test_simle_replacement(&jump_into_super_inst_original,
                              &jump_into_super_inst_original_end,
                              &jump_into_super_inst_modified,
                              &jump_into_super_inst_modified_end);
  assert(rc != 0);
  assert(errno == EINVAL);
}
#endif

void run_test(const char *test_name, void (*test_func)(void)) {
  printf("Running %s...\n", test_name);
  test_func();
}

int is_replacement_enabled(void) {
  char trash;
  return (0 == nacl_dyncode_modify(allocate_code_space(1), &trash, 0));
}

#define RUN_TEST(test_func) (run_test(#test_func, test_func))

int TestMain(void) {
  /* Turn off stdout buffering to aid debugging in case of a crash. */
  setvbuf(stdout, NULL, _IONBF, 0);

  assert(is_replacement_enabled());

  RUN_TEST(test_replacing_code);
  RUN_TEST(test_replacing_code_unaligned);
#if defined(__i386__) || defined(__x86_64__)
  RUN_TEST(test_replacing_code_slowpaths);
  RUN_TEST(test_jump_into_super_inst_create);
  RUN_TEST(test_start_with_super_inst_replace);
  RUN_TEST(test_jump_into_super_inst_replace);
#endif
  RUN_TEST(test_illegal_code_replacment);
  RUN_TEST(test_external_jump_target_replacement);
#if defined(__i386__)
  RUN_TEST(test_delete_superinstruction);
  RUN_TEST(test_delete_superinstruction_split);
  RUN_TEST(test_create_superinstruction);
  RUN_TEST(test_create_superinstruction_split);
  RUN_TEST(test_change_boundaris_first_instructions);
  RUN_TEST(test_change_boundaris_last_instructions);
#endif

  return 0;
}

int main(void) {
  return RunTests(TestMain);
}
