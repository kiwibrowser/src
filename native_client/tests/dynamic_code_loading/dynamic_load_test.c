/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/dynamic_code_loading/dynamic_load_test.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <nacl/nacl_dyncode.h>

#include "native_client/src/include/arm_sandbox.h"
#include "native_client/tests/dynamic_code_loading/dynamic_segment.h"
#include "native_client/tests/dynamic_code_loading/templates.h"
#include "native_client/tests/inbrowser_test_runner/test_runner.h"

#if defined(__x86_64__)
/* On x86-64, template functions do not fit in 32-byte buffers */
#define BUF_SIZE 64
#elif defined(__i386__) || defined(__arm__)
#define BUF_SIZE 32
#else
#error "Unknown Platform"
#endif

/*
 * TODO(bsy): get this value from the toolchain.  Get the toolchain
 * team to provide this value.
 */
#define NUM_BUNDLES_FOR_HLT 3

int nacl_load_code(void *dest, void *src, int size) {
  int rc = nacl_dyncode_create(dest, src, size);
  /*
   * Undo the syscall wrapper's errno handling, because it's more
   * convenient to test a single return value.
   */
  return rc == 0 ? 0 : -errno;
}

char *next_addr = NULL;

char *allocate_code_space(int pages) {
  char *addr;
  if (next_addr == NULL) {
    next_addr = (char *) DYNAMIC_CODE_SEGMENT_START;
  }
  addr = next_addr;
  next_addr += PAGE_SIZE * pages;
  assert(next_addr < (char *) DYNAMIC_CODE_SEGMENT_END);
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

void fill_hlts(uint8_t *data, size_t size) {
#if defined(__i386__) || defined(__x86_64__)
  memset(data, 0xf4, size); /* HLTs */
#elif defined(__arm__)
  fill_int32(data, size, NACL_INSTR_ARM_HALT_FILL);
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
  assert(dest_size % NACL_BUNDLE_SIZE == 0);
  assert(fragment_size <= dest_size);
  fill_nops(dest, dest_size);
  memcpy(dest, fragment_start, fragment_size);
}

/* Check that we can load and run code. */
void test_loading_code(void) {
  void *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;
  int (*func)(void);

  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);

  rc = nacl_load_code(load_area, buf, sizeof(buf));
  assert(rc == 0);
  assert(memcmp(load_area, buf, sizeof(buf)) == 0);
  /*
   * Need double cast otherwise gcc complains with "ISO C forbids
   * conversion of object pointer to function pointer type
   * [-pedantic]".
   */
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_OLD);
}

/*
 * This is mostly the same as test_loading_code() except that we
 * repeat the test many times within the same page.  Unlike the other
 * tests, this will consistently fail on ARM if we do not flush the
 * instruction cache, so it reproduces the bug
 * http://code.google.com/p/nativeclient/issues/detail?id=699
 */
void test_stress(void) {
  void *load_area = allocate_code_space(1);
  uint8_t *dest;
  uint8_t *dest_max;
  uint8_t buf[BUF_SIZE];

  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);

  dest_max = (uint8_t *) load_area + DYNAMIC_CODE_PAGE_SIZE;
  for (dest = load_area; dest < dest_max; dest += sizeof(buf)) {
    int (*func)(void);
    int rc;

    rc = nacl_load_code(dest, buf, sizeof(buf));
    assert(rc == 0);
    func = (int (*)(void)) (uintptr_t) dest;
    rc = func();
    assert(rc == MARKER_OLD);
  }
}

/*
 * The syscall may have to mmap() shared memory temporarily,
 * so there is some interaction with page size.
 * Check that we can load to non-page-aligned addresses.
 */
void test_loading_code_non_page_aligned(void) {
  char *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;

  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);

  rc = nacl_load_code(load_area, buf, sizeof(buf));
  assert(rc == 0);
  assert(memcmp(load_area, buf, sizeof(buf)) == 0);

  load_area += sizeof(buf);
  rc = nacl_load_code(load_area, buf, sizeof(buf));
  assert(rc == 0);
  assert(memcmp(load_area, buf, sizeof(buf)) == 0);
}

/*
 * Since there is an interaction with page size, we also test loading
 * a multi-page chunk of code.
 */
void test_loading_large_chunk(void) {
  char *load_area = allocate_code_space(2);
  int size = 0x20000;
  uint8_t *data = alloca(size);
  int rc;

  fill_nops(data, size);
  rc = nacl_load_code(load_area, data, size);
  assert(rc == 0);
  assert(memcmp(load_area, data, size) == 0);
}

void test_loading_zero_size(void) {
  char *load_area = allocate_code_space(1);
  int rc = nacl_load_code(load_area, &template_func, 0);
  assert(rc == 0);
}

/*
 * In general, the failure tests don't check that loading fails for
 * the reason we expect.  TODO(mseaborn): We could do this by
 * comparing with expected log output.
 */

void test_fail_on_validation_error(void) {
  void *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;

  copy_and_pad_fragment(buf, sizeof(buf), &invalid_code, &invalid_code_end);

  rc = nacl_load_code(load_area, buf, sizeof(buf));
  assert(rc == -EINVAL);
}

void test_validation_error_does_not_leak(void) {
  void *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;

  copy_and_pad_fragment(buf, sizeof(buf), &invalid_code, &invalid_code_end);
  rc = nacl_load_code(load_area, buf, sizeof(buf));
  assert(rc == -EINVAL);

  /*
   * Make sure that the failed validation didn't claim the memory.
   * See: http://code.google.com/p/nativeclient/issues/detail?id=2566
   */
  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);
  rc = nacl_load_code(load_area, buf, sizeof(buf));
  assert(rc == 0);
}

void test_fail_on_non_bundle_aligned_dest_addresses(void) {
  char *load_area = allocate_code_space(1);
  int rc;
  uint8_t nops[BUF_SIZE];

  fill_nops(nops, sizeof(nops));

  /* Test unaligned destination. */
  rc = nacl_load_code(load_area + 1, nops, NACL_BUNDLE_SIZE);
  assert(rc == -EINVAL);
  rc = nacl_load_code(load_area + 4, nops, NACL_BUNDLE_SIZE);
  assert(rc == -EINVAL);

  /* Test unaligned size. */
  rc = nacl_load_code(load_area, nops + 1, NACL_BUNDLE_SIZE - 1);
  assert(rc == -EINVAL);
  rc = nacl_load_code(load_area, nops + 4, NACL_BUNDLE_SIZE - 4);
  assert(rc == -EINVAL);

  /* Check that the code we're trying works otherwise. */
  rc = nacl_load_code(load_area, nops, NACL_BUNDLE_SIZE);
  assert(rc == 0);
}

/*
 * In principle we could load into the initially-loaded executable's
 * code area, but at the moment we don't allow it.
 */
void test_fail_on_load_to_static_code_area(void) {
  int size = &hlts_end - &hlts;
  int rc = nacl_load_code(&hlts, &hlts, size);
  assert(rc == -EFAULT);
}

uint8_t block_in_data_segment[64];

void test_fail_on_load_to_data_area(void) {
  uint8_t *data;
  int rc;

  fill_hlts(block_in_data_segment, sizeof(block_in_data_segment));

  /*
   * Align to bundle size so that we don't fail for a reason we're not
   * testing for.
   */
  data = block_in_data_segment;
  while (((int) data) % NACL_BUNDLE_SIZE != 0)
    data++;
  rc = nacl_load_code(data, data, NACL_BUNDLE_SIZE);
  assert(rc == -EFAULT);
}

void test_fail_on_overwrite(void) {
  void *load_area = allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;

  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);

  rc = nacl_load_code(load_area, buf, sizeof(buf));
  assert(rc == 0);

  copy_and_pad_fragment(buf, sizeof(buf), &template_func,
                                          &template_func_end);

  rc = nacl_load_code(load_area, buf, sizeof(buf));
  assert(rc == -EINVAL);
}


/* Allowing mmap() to overwrite the dynamic code area would be unsafe. */
void test_fail_on_mmap_to_dyncode_area(void) {
  void *addr = allocate_code_space(1);
  size_t page_size = 0x10000;
  void *result;
  int rc;

  assert((uintptr_t) addr % page_size == 0);
  result = mmap(addr, page_size, PROT_READ | PROT_WRITE,
                MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(result == MAP_FAILED);
  assert(errno == EINVAL);

  rc = munmap(addr, page_size);
  assert(rc == -1);
  assert(errno == EINVAL);

  /* TODO(mseaborn): Test mprotect() once NaCl provides it. */
}

void test_branches_outside_chunk(void) {
  char *load_area = allocate_code_space(1);
  int rc;
  int size = &branch_forwards_end - &branch_forwards;
  assert(size == 16 || size == 32);
  assert(&branch_backwards_end - &branch_backwards == size);

  rc = nacl_load_code(load_area, &branch_forwards, size);
  assert(rc == 0);
  rc = nacl_load_code(load_area + size, &branch_backwards, size);
  assert(rc == 0);
}

void test_end_of_code_region(void) {
  int rc;
  void *dest;
  uint8_t data[BUF_SIZE];
  fill_nops(data, sizeof(data));

  /*
   * This tries to load into the data segment, which is definitely not
   * allowed.
   */
  dest = (uint8_t *) DYNAMIC_CODE_SEGMENT_END;
  rc = nacl_load_code(dest, data, sizeof(data));
  assert(rc == -EFAULT);

  /*
   * This tries to load into the last bundle of the code region, which
   * sel_ldr disallows just in case there is some CPU bug in which the
   * CPU fails to check for running off the end of an x86 code
   * segment.  This is applied to other architectures for
   * consistency.
   */
  dest = (uint8_t *) DYNAMIC_CODE_SEGMENT_END - sizeof(data);
  rc = nacl_load_code(dest, data, sizeof(data));
  assert(rc == -EFAULT);

  dest = (uint8_t *) DYNAMIC_CODE_SEGMENT_END - sizeof(data) * 2;
  rc = nacl_load_code(dest, data, sizeof(data));
  assert(rc == 0);
}

void test_hlt_filled_bundle(void) {
  uint8_t bad_code[NUM_BUNDLES_FOR_HLT * NACL_BUNDLE_SIZE];
  void *load_area;
  int ix;

  for (ix = 0; ix < NUM_BUNDLES_FOR_HLT; ++ix) {
    fill_nops(bad_code, sizeof bad_code);
    fill_hlts(bad_code + ix * NACL_BUNDLE_SIZE, NACL_BUNDLE_SIZE);

    load_area = allocate_code_space(1);
    /* hlts are now allowed */
    assert(0 == nacl_load_code(load_area, bad_code, sizeof bad_code));
    /* but not twice... */
    assert(0 != nacl_load_code(load_area, bad_code, sizeof bad_code));
  }
}

/*
 * If threading tests have run before in this process, nacl_dyncode_delete will
 * return EAGAIN if the threads have not finished trusted-side cleanup yet.
 * (this is related to the
 * https://code.google.com/p/nativeclient/issues/detail?id=1028).
 * If we have joined the thread already, then we just need to wait until it
 * finishes untrusted-side cleanup and calls IRT thread_exit. Doing this allows
 * the tests to run in any order. Only the first deletion in a non-threaded test
 * needs to do this.
 */
int dyncode_delete_with_retry(void *dest, size_t size) {
  int rc;
  do {
    rc = nacl_dyncode_delete(dest, size);
  } while (rc != 0 && errno == EAGAIN);
  return rc;
}

/* Check that we can dynamically delete code. */
void test_deleting_code(void) {
  uint8_t *load_area = (uint8_t *) allocate_code_space(1);
  uint8_t buf[BUF_SIZE];
  int rc;
  int (*func)(void);

  copy_and_pad_fragment(buf, sizeof(buf), &template_func, &template_func_end);
  rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
  assert(rc == 0);
  func = (int (*)(void)) (uintptr_t) load_area;
  rc = func();
  assert(rc == MARKER_OLD);

  rc = dyncode_delete_with_retry(load_area, sizeof(buf));
  assert(rc == 0);
  assert(load_area[0] != buf[0]);

  /* Attempting to unload the code again should fail. */
  rc = nacl_dyncode_delete(load_area, sizeof(buf));
  assert(rc == -1);
  assert(errno == EFAULT);

  /*
   * We should be able to load new code at the same address.  This
   * assumes that no other threads are running, otherwise this request
   * can be rejected.
   *
   * This fails under ARM QEMU.  QEMU will flush its instruction
   * translation cache based on writes to the same virtual address,
   * but it ignores our explicit cache flush system calls.  Valgrind
   * has a similar problem, except that there is no cache flush system
   * call on x86.
   */
  if (getenv("UNDER_QEMU_ARM") != NULL ||
      getenv("RUNNING_ON_VALGRIND") != NULL) {
    printf("Skipping loading new code under emulator\n");
  } else {
    printf("Testing loading new code...\n");
    copy_and_pad_fragment(buf, sizeof(buf), &template_func_replacement,
                          &template_func_replacement_end);
    rc = nacl_dyncode_create(load_area, buf, sizeof(buf));
    assert(rc == 0);
    func = (int (*)(void)) (uintptr_t) load_area;
    rc = func();
    assert(rc == MARKER_NEW);

    rc = nacl_dyncode_delete(load_area, sizeof(buf));
    assert(rc == 0);
    assert(load_area[0] != buf[0]);
  }
}

/* nacl_dyncode_delete() succeeds trivially on the empty range. */
void test_deleting_zero_size(void) {
  uint8_t *load_addr = (uint8_t *) allocate_code_space(1);
  int rc = nacl_dyncode_delete(load_addr, 0);
  assert(rc == 0);
}

void test_deleting_code_from_invalid_ranges(void) {
  uint8_t *load_addr = (uint8_t *) allocate_code_space(1) + 32;
  uint8_t buf[64];
  int rc;

  /* We specifically want to test using multiple instruction bundles. */
  assert(sizeof(buf) / NACL_BUNDLE_SIZE >= 2);
  assert(sizeof(buf) % NACL_BUNDLE_SIZE == 0);

  rc = dyncode_delete_with_retry(load_addr, sizeof(buf));
  assert(rc == -1);
  assert(errno == EFAULT);

  fill_hlts(buf, sizeof(buf));
  rc = nacl_dyncode_create(load_addr, buf, sizeof(buf));
  assert(rc == 0);

  /* Overlapping before. */
  rc = nacl_dyncode_delete(load_addr - NACL_BUNDLE_SIZE,
                           sizeof(buf) + NACL_BUNDLE_SIZE);
  assert(rc == -1);
  assert(errno == EFAULT);
  /* Overlapping after. */
  rc = nacl_dyncode_delete(load_addr, sizeof(buf) + NACL_BUNDLE_SIZE);
  assert(rc == -1);
  assert(errno == EFAULT);
  /* Missing the end of the loaded chunk. */
  rc = nacl_dyncode_delete(load_addr, sizeof(buf) - NACL_BUNDLE_SIZE);
  assert(rc == -1);
  assert(errno == EFAULT);
  /* Missing the start of the loaded chunk. */
  rc = nacl_dyncode_delete(load_addr + NACL_BUNDLE_SIZE,
                           sizeof(buf) - NACL_BUNDLE_SIZE);
  assert(rc == -1);
  assert(errno == EFAULT);
  /* The correct range should work, though. */
  rc = nacl_dyncode_delete(load_addr, sizeof(buf));
  assert(rc == 0);
}

void check_region_is_filled_with_hlts(const char *data, size_t size) {
#if defined(__i386__) || defined(__x86_64__)
  uint8_t halts = 0xf4; /* HLT */
#elif defined(__arm__)
  uint32_t halts = NACL_INSTR_ARM_HALT_FILL;
#else
# error "Unknown arch"
#endif
  const char *ptr;
  for (ptr = data; ptr < data + size; ptr += sizeof(halts)) {
    assert(memcmp(ptr, &halts, sizeof(halts)) == 0);
  }
}

/*
 * Check that regions surrounding the region we load code into are
 * correctly filled with halt instructions.  Loading code causes the
 * pages to become allocated, and unused parts of these pages should
 * be filled with halts.
 */
void test_demand_alloc_surrounding_hlt_filling(void) {
  int pad_size = 0x4000; /* This must be less than one 64k page. */
  int code_size = 0x28000;
  int total_size = pad_size * 2 + code_size;
  assert(total_size % DYNAMIC_CODE_PAGE_SIZE == 0);
  char *load_area = allocate_code_space(total_size / DYNAMIC_CODE_PAGE_SIZE);
  uint8_t *data = alloca(code_size);
  int rc;

  fill_nops(data, code_size);
  rc = nacl_load_code(load_area + pad_size, data, code_size);
  assert(rc == 0);
  check_region_is_filled_with_hlts(load_area, pad_size);
  assert(memcmp(load_area + pad_size, data, code_size) == 0);
  check_region_is_filled_with_hlts(load_area + pad_size + code_size, pad_size);
}

/*
 * Check that dyncode_create() works on a set of pages when a strict
 * subset of those pages were allocated by a previous dyncode_create()
 * call.  This provides some coverage of the coalescing of mprotect()
 * calls that dyncode_create() does.
 */
void test_demand_alloc_of_fragmented_pages(void) {
  int smaller_size = 2 * DYNAMIC_CODE_PAGE_SIZE;
  int smaller_size_load_offset = 2 * DYNAMIC_CODE_PAGE_SIZE;
  int larger_size = 6 * DYNAMIC_CODE_PAGE_SIZE;
  char *load_area = allocate_code_space(6);
  uint8_t *data = alloca(larger_size);
  int rc;

  fill_nops(data, larger_size);

  /* Cause pages 2 and 3 to be allocated. */
  rc = nacl_load_code(load_area + smaller_size_load_offset, data, smaller_size);
  assert(rc == 0);

  rc = dyncode_delete_with_retry(load_area + smaller_size_load_offset,
                                 smaller_size);
  assert(rc == 0);

  /* Cause pages 0, 1, 4 and 5 to be allocated as well. */
  rc = nacl_load_code(load_area, data, larger_size);
  assert(rc == 0);
}

void run_test(const char *test_name, void (*test_func)(void)) {
  printf("Running %s...\n", test_name);
  test_func();
}

#define RUN_TEST(test_func) (run_test(#test_func, test_func))

int TestMain(void) {
  /*
   * This should come first, so that we test loading code into the first page.
   * See http://code.google.com/p/nativeclient/issues/detail?id=1143
   */
  RUN_TEST(test_loading_code);

  RUN_TEST(test_loading_code_non_page_aligned);
  RUN_TEST(test_loading_large_chunk);
  RUN_TEST(test_loading_zero_size);
  RUN_TEST(test_fail_on_validation_error);
  RUN_TEST(test_validation_error_does_not_leak);
  RUN_TEST(test_fail_on_non_bundle_aligned_dest_addresses);
  RUN_TEST(test_fail_on_load_to_static_code_area);
  RUN_TEST(test_fail_on_load_to_data_area);
  RUN_TEST(test_fail_on_overwrite);
  RUN_TEST(test_fail_on_mmap_to_dyncode_area);
  RUN_TEST(test_branches_outside_chunk);
  RUN_TEST(test_end_of_code_region);
  RUN_TEST(test_hlt_filled_bundle);
  /*
   * dyncode_delete() tests have been broken inside Chromium by the
   * switch to the new Chrome-IPC-based PPAPI proxy.  The new proxy
   * uses a background thread which does not "check in" in a way to
   * satisfy dyncode_delete()'s requirements.
   * See https://code.google.com/p/nativeclient/issues/detail?id=3199
   * TODO(mseaborn): Re-enable these again when they work.
  */
  if (!TestRunningInBrowser()) {
    RUN_TEST(test_deleting_code);
    RUN_TEST(test_deleting_zero_size);
    RUN_TEST(test_deleting_code_from_invalid_ranges);
    RUN_TEST(test_threaded_delete);
  }
  RUN_TEST(test_demand_alloc_surrounding_hlt_filling);
  /*
   * This test uses dyncode_delete() which is broken inside Chromium
   * with the new PPAPI proxy.  See above.  TODO(mseaborn): Re-enable.
   */
  if (!TestRunningInBrowser()) {
    RUN_TEST(test_demand_alloc_of_fragmented_pages);
  }
  RUN_TEST(test_stress);
  /*
   * TODO(ncbray) reenable when kernel bug is fixed.
   * http://code.google.com/p/nativeclient/issues/detail?id=2678
   */
#ifndef __arm__
  /*
   * TODO(ncbray) reenable when cause of flake is understood.
   * http://code.google.com/p/chromium/issues/detail?id=120355
   */
  if (!TestRunningInBrowser())
    RUN_TEST(test_threaded_loads);
#endif

  /* Test again to make sure we didn't run out of space. */
  RUN_TEST(test_loading_code);

  return 0;
}

int main(void) {
  return RunTests(TestMain);
}
