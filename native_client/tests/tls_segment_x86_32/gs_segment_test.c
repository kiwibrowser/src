/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


void *gs_segment_read_only_test(void *unused_thread_arg) {
  uint32_t value;
  __asm__ volatile("movl %%gs:0, %0" : "=r"(value));
  fprintf(stderr, "** intended_exit_status=untrusted_sigsegv_or_equivalent\n");
  /*
   * Check that the %gs segment is read-only by attempting to write to
   * it.  We write the same value that it currently contains so that,
   * if the write succeeds, we do not corrupt TLS and we can report
   * the failure by exiting without crashing (since crashing could
   * make the test succeed accidentally).
   *
   * Note that we expect this instruction to be disallowed by the
   * validator in the future, so this program may need to be run with
   * validation disabled.
   * See http://code.google.com/p/nativeclient/issues/detail?id=2250
   */
  __asm__ volatile("movl %0, %%gs:0\n" : : "r"(value));
  /* Should not reach here. */
  _exit(1);
}

void *gs_segment_size_test(void *unused_thread_arg) {
  /*
   * Check that the %gs segment is 16 bytes in size by attempting to
   * read 1 byte beyond the end.
   */
  uint32_t dummy;
  fprintf(stderr, "** intended_exit_status=untrusted_sigsegv_or_equivalent\n");
  __asm__ volatile("movl %%gs:(16-3), %0" : "=r"(dummy));
  /* Should not reach here. */
  _exit(1);
}

void run_in_thread(void *(*func)(void *thread_arg)) {
  pthread_t tid;
  int rc;
  rc = pthread_create(&tid, NULL, func, NULL);
  assert(rc == 0);
  rc = pthread_join(tid, NULL);
  assert(rc == 0);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Expected test type argument\n");
    return 1;
  }
  if (strcmp(argv[1], "gs_segment_read_only_test") == 0) {
    gs_segment_read_only_test(NULL);
  } else if (strcmp(argv[1], "gs_segment_read_only_test_thread") == 0) {
    run_in_thread(gs_segment_read_only_test);
  } else if (strcmp(argv[1], "gs_segment_size_test") == 0) {
    gs_segment_size_test(NULL);
  } else if (strcmp(argv[1], "gs_segment_size_test_thread") == 0) {
    run_in_thread(gs_segment_size_test);
  } else {
    fprintf(stderr, "Unrecognised test type argument: %s\n", argv[1]);
  }
  /* Should not reach here because the test should crash. */
  return 1;
}
