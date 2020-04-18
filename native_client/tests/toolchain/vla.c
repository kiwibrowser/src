/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Simple test to verify that C99 VLAs (variable length arrays) work.
 */
#include <stdio.h>

void test_basic_vla(int size) {
  /* "volatile", to try to prevent optimizing known summation patterns. */
  volatile int vla[size];
  for (int i = 0; i < size; ++i) {
    vla[i] = i + 10;
  }
  printf("test_basic_vla w/ size %d\n", size);
  printf("  %d + 10 == %d\n", 0, vla[0]);
  printf("  %d + 10 == %d\n", size / 2, vla[size / 2]);
  printf("  %d + 10 == %d\n", size - 1, vla[size - 1]);
}

void test_basic_in_loop(int size) {
  int saved_0 = 0;
  int saved_mid = 0;
  int saved_last = 0;
  for (int i = 0; i < size; ++i) {
    volatile int vla[size];
    vla[i] = i + 10;
    if (i == 0)
      saved_0 = vla[i];
    if (i == size / 2)
      saved_mid = vla[i];
    if (i == size - 1)
      saved_last = vla[i];
  }
  printf("test_basic_in_loop w/ size %d\n", size);
  printf("  %d + 10 == %d\n", 0, saved_0);
  printf("  %d + 10 == %d\n", size / 2, saved_mid);
  printf("  %d + 10 == %d\n", size - 1, saved_last);
}

void test_two_in_loops(int size) {
  volatile int vla1[size];
  for (int i = 0; i < size; i++) {
    volatile int vla2[size];
    for (int j = 0; j < size; j++) {
      vla2[j] = j;
    }
    /*
     * At this point, vla2[size - 1] == size - 1.
     * We keep adding that vla2[size - 1] to the previous value in vla1:
     * (size - 1) + (size - 1) + ... + (size - 1), a total of "size" times.
     */
    if (i > 0)
      vla1[i] = vla1[i - 1] + vla2[size - 1];
    else
      vla1[i] = vla2[size - 1];
  }
  printf("test_two_in_loops w/ size %d, should be\n  (%d - 1) * %d == %d\n",
         size, size, size, vla1[size - 1]);
}

/*
 * Just like test_two_in_loops, but with recursion.
 * Make test_two_recursion_help() "noinline" to ensure that there really
 * is a function call within the loop (to adjust the stack), and the
 * recursive call doesn't get optimized into a loop.
 */
void test_two_recursion_help(int size,
                             int counter,
                             volatile int *arr) __attribute__((noinline));
void test_two_recursion(int size) {
  volatile int vla1[size];
  for (int i = 0; i < size; i++) {
    volatile int vla2[size];
    /* Adjust stack again with function call */
    test_two_recursion_help(size, 0, vla2);
    if (i > 0)
      vla1[i] = vla1[i - 1] + vla2[size - 1];
    else
      vla1[i] = vla2[size - 1];
  }
  printf("test_two_recursion w/ size %d, should be\n  (%d - 1) * %d == %d\n",
         size, size, size, vla1[size - 1]);
}

void test_two_recursion_help(int size, int counter, volatile int *arr) {
  if (counter < size) {
    arr[counter] = counter;
    test_two_recursion_help(size, ++counter, arr);
  }
}

int main(int argc, char *argv[]) {
  volatile int one_iter = 1;
  volatile int ten_iters = 10;
  test_basic_vla(one_iter);
  test_basic_vla(ten_iters);
  test_basic_in_loop(one_iter);
  test_basic_in_loop(ten_iters);
  test_two_in_loops(one_iter);
  test_two_in_loops(ten_iters);
  test_two_recursion(one_iter);
  test_two_recursion(ten_iters);
  return 0;
}
