/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Simple test to verify that memcpy, memmove and memset are found and
 * work properly.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum {
/* A size between 8 and 256 bytes, which is one case in the x86-64 asm memcpy */
  medium_length = 8 * sizeof(long),
/* 256 bytes, a different case in x86-64 asm memcpy */
  large_length = 64 * sizeof(long),
/* A size big enough to hold 2 copies of the large length plus extra to test
 *  an unaligned destination.
 */
  total_buf_len = large_length * 2 + medium_length,
};
/*
 * Create buffer as an array of long, to ensure word-size alignment. The
 * actual code accesses it via a char*.
 */
volatile long buf[total_buf_len / sizeof(long)] = {0};


/*
 * Reset global buf to the sequence of bytes: 0, 1, 2 ... LENGTH - 1
 */
void reset_buf(void) {
  unsigned char *bufptr = (unsigned char *) buf;
  unsigned i;
  for (i = 0; i < total_buf_len; ++i)
    bufptr[i] = i;
}

void dump_buf(void) {
  unsigned char *bufptr = (unsigned char *) buf;
  for (int i = 0; i < total_buf_len; ++i)
    printf("buf[%u] (%p) = %u\n", i, (void *) &bufptr[i], bufptr[0]);
}

/*
 * Each function we're testing has a "checked" version that runs it and makes
 * sure the destination pointer is returned correctly. For memcpy, additionally
 * check that the source and the destination match after the call.
 */
void checked_memcpy(void *dst, void *src, unsigned n) {
  assert((unsigned char *)dst + n < (unsigned char *)buf + total_buf_len);
  void *ret = memcpy(dst, src, n);
  if (ret != dst) {
    printf("Wrong memcpy return value: %p != %p\n", ret, dst);
    dump_buf();
    exit(1);
  }
  if (memcmp(dst, src, n)) {
    printf("memcmp after memcpy failure: %p -> %p len %u\n", src, dst, n);
    dump_buf();
    exit(1);
  }
}

void checked_memmove(void *dst, void *src, unsigned n) {
  void *ret = memmove(dst, src, n);
  if (ret != dst) {
    printf("Wrong memmove return value: %p != %p\n", ret, dst);
    exit(1);
  }
}

void checked_memset(void *s, int c, unsigned n) {
  void *ret = memset(s, c, n);
  if (ret != s) {
    printf("Wrong memset return value: %p != %p\n", ret, s);
    exit(1);
  }
  char *s_char = (char *)s;
  char *dst_char = s_char;
  for (unsigned i = 0; i < n; ++i, ++dst_char) {
    if (*dst_char != c) {
      printf("memset failure: index %d (%p) = %u\n",
             i, (void *) dst_char, *dst_char);
      dump_buf();
      exit(1);
    }
  }
  if (*dst_char == c) {
    printf("memset failure: wrote %d past the end of buffer\n", c);
    exit(1);
  }
}

int main(void) {
  /* arrptr is an aligned pointer to the buffer. */
  unsigned char *arrptr = (unsigned char*) buf, *src, *dst;
  if ((long) arrptr & (sizeof(long) - 1)) {
    puts("Internal error: unaligned buf\n");
    return 1;
  }
  reset_buf();

  /*
   * Test 1: memcpy small chunk, from aligned to aligned address.
   * "small chunk" is anything smaller than UNROLLBLOCKSIZE in our
   * implementation of these functions.
   */
  reset_buf();
  src = arrptr;
  dst = arrptr + medium_length * 2;
  checked_memcpy(dst, src, 6);
  printf("1: %u\n", (unsigned)dst[4]);    /* expect 4 */

  /* Test 2: memcpy small chunk, from aligned to unaligned address */
  reset_buf();
  src = arrptr;
  dst = arrptr + medium_length * 2 + 1;
  checked_memcpy(dst, src, 6);
  printf("2: %u\n", (unsigned)dst[4]);    /* expect 4 */

  /* Test 3: memcpy small chunk, from unaligned to aligned address */
  reset_buf();
  src = arrptr + 1;
  dst = arrptr + medium_length * 2;
  checked_memcpy(dst, src, 6);
  printf("3: %u\n", (unsigned)dst[4]);    /* expect 5 */

  /* Test 4: memcpy small chunk, from unaligned to unaligned address */
  reset_buf();
  src = arrptr + 3;
  dst = arrptr + medium_length * 2 + 3;
  checked_memcpy(dst, src, 6);
  printf("4: %u\n", (unsigned)dst[4]);    /* expect 7 */

  /* Test 5: memcpy medium chunk, from aligned to aligned address */
  reset_buf();
  src = arrptr;
  dst = arrptr + medium_length * 2;
  checked_memcpy(dst, src, medium_length);
  printf("5: %u\n", (unsigned)dst[30]);    /* expect 30 */

  /* Test 6: memcpy medium chunk, from aligned to unaligned address */
  reset_buf();
  src = arrptr;
  dst = arrptr + medium_length * 2 + 1;
  checked_memcpy(dst, src, medium_length);
  printf("6: %u\n", (unsigned)dst[30]);    /* expect 30 */

  /* Test 7: memcpy medium chunk, from unaligned to aligned address */
  reset_buf();
  src = arrptr + 1;
  dst = arrptr + medium_length * 2;
  checked_memcpy(dst, src, medium_length);
  printf("7: %u\n", (unsigned)dst[30]);    /* expect 31 */

  /* Test 8: memcpy medium chunk, from unaligned to unaligned address */
  reset_buf();
  src = arrptr + 3;
  dst = arrptr + medium_length * 2 + 3;
  checked_memcpy(dst, src, medium_length);
  printf("8: %u\n", (unsigned)dst[30]);    /* expect 33 */

  /* Test 9: memcpy medium chunk, near edges/overlap */
  reset_buf();
  src = arrptr;
  dst = arrptr + medium_length * 2;
  checked_memcpy(dst, src, medium_length * 2);
  printf("9: %u\n", (unsigned)dst[10]);    /* expect 10 */

  /* Test 10: memcpy large chunk, from aligned to aligned address */
  reset_buf();
  src = arrptr;
  dst = arrptr + large_length;
  checked_memcpy(dst, src, large_length);
  printf("10: %u\n", (unsigned)dst[129]);   /* expect 129 */

  /* Test 11: memcpy large chunk, from unaligned to aligned address */
  reset_buf();
  src = arrptr + 1;
  dst = arrptr + large_length;
  checked_memcpy(dst, src, large_length);
  printf("11: %u\n", (unsigned)dst[129]);   /* expect 130 */

  /* Test 12: memcpy large chunk, from aligned to unaligned address */
  reset_buf();
  src = arrptr;
  dst = arrptr + large_length + 3;
  checked_memcpy(dst, src, large_length);
  printf("12: %u\n", (unsigned)dst[129]);   /* expect 130 */

  /* Test 13: memcpy 0-sized chunk */
  reset_buf();
  src = arrptr;
  dst = arrptr + medium_length * 2;
  checked_memcpy(dst, src, 0);
  printf("13: %u\n", (unsigned)dst[0]);   /* expect 64 */


  /* Test 100: memset small chunk, aligned address */
  reset_buf();
  checked_memset(arrptr, 99, 5);
  printf("100: %u\n", (unsigned)arrptr[4]);   /* expect 99 */

  /* Test 101: memset small chunk, unaligned address */
  reset_buf();
  checked_memset(arrptr + 3, 99, 5);
  printf("101a: %u\n", (unsigned)arrptr[7]);   /* expect 99 */
  printf("101b: %u\n", (unsigned)arrptr[2]);   /* expect 2 */
  printf("101c: %u\n", (unsigned)arrptr[8]);   /* expect 8 */

  /* Test 102: memset medium chunk, aligned address */
  reset_buf();
  checked_memset(arrptr, 99, medium_length);
  printf("102: %u\n", (unsigned)arrptr[31]);   /* expect 99 */

  /* Test 103: memset medium chunk, unaligned address */
  reset_buf();
  checked_memset(arrptr + 3, 99, medium_length);
  printf("103: %u\n", (unsigned)arrptr[34]);   /* expect 99 */

  /* Test 104: edge */
  reset_buf();
  checked_memset(arrptr, 99, medium_length * 2);
  printf("104: %u\n", (unsigned)arrptr[medium_length * 2 - 1]);  /* expect 99 */

  /* Test 105: memset large chunk, aligned address */
  reset_buf();
  checked_memset(arrptr, 99, large_length);
  printf("105: %u\n", (unsigned)arrptr[large_length - 1]); /* expect 99 */

  /* Test 106: memset large chunk, unaligned address */
  reset_buf();
  checked_memset(arrptr + 3, 99, large_length);
  printf("106: %u\n", (unsigned)arrptr[large_length + 2]); /* expect 99 */

  /* Test 107: memset zero size */
  reset_buf();
  checked_memset(arrptr, 99, 0);
  printf("107: %u\n", (unsigned)arrptr[0]); /* expect 0 */

  /*
   * The non-overlapping logic of memmove is pretty much the same as memcpy.
   * Do a sanity check and then test overlapping addresses.
   */

  /* Test 201: memmove medium chunk, from aligned to aligned address */
  reset_buf();
  src = arrptr;
  dst = arrptr + medium_length * 2;
  checked_memmove(dst, src, medium_length);
  printf("201: %u\n", (unsigned)dst[31]);    /* expect 31 */

  /* Test 202: memmove small chunk in overlapping addresses */
  reset_buf();
  src = arrptr + 4;
  dst = arrptr;
  checked_memmove(dst, src, 8);
  printf("202: %u\n", (unsigned)dst[7]);     /* expect 11 */

  /* Test 203: memmove large chunk in overlapping addresses */
  reset_buf();
  src = arrptr + 1;
  dst = arrptr;
  checked_memmove(dst, src, medium_length * 2);
  printf("203: %u\n", (unsigned)dst[63]);    /* expect 64 */

  /* Test 204: memmove at edge */
  reset_buf();
  src = arrptr + 1;
  dst = arrptr;
  checked_memmove(dst, src, medium_length * 4 - 1);
  /* expect length-1 */
  printf("204: %u\n", (unsigned)dst[medium_length * 4 - 2]);

  return 0;
}
