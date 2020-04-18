/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <semaphore.h>

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <limits>

#include "native_client/tests/syscalls/test.h"

// Test error conditions of sem_init and return the number of failed checks.
//
// According to the man page on Linux:
// ===================================
// RETURN VALUE
//        sem_init() returns 0 on success; on error, -1 is returned, and errno
//        is set to indicate the error.
//
// ERRORS
//        EINVAL value exceeds SEM_VALUE_MAX.
//
//        ENOSYS pshared is non-zero, but the system does  not  support
//               process-shared semaphores (see sem_overview(7)).
// ===================================
// However, pshared is not supported in NaCl currently, so a non-zero pshared
// value should yield an error (EINVAL).
int TestSemInitErrors() {
  START_TEST("sem_init error conditions");
  // First, make sure that it is possible to exceed SEM_VALUE_MAX
  // for this test, otherwise we can't cause this failure mode.
  EXPECT(SEM_VALUE_MAX < std::numeric_limits<unsigned int>::max());

  sem_t my_semaphore;

  // Create a value just beyond SEM_VALUE_MAX, try to initialize the semaphore.
  const unsigned int sem_max_plus_1 = (unsigned) SEM_VALUE_MAX + 1;

  // sem_init should return -1 and errno should equal EINVAL
  EXPECT(-1 == sem_init(&my_semaphore, 0, sem_max_plus_1));
  EXPECT(EINVAL == errno);

  // Try with the largest possible unsigned int.
  EXPECT(-1 == sem_init(&my_semaphore,
                        0,
                        std::numeric_limits<unsigned int>::max()));
  EXPECT(EINVAL == errno);

#if !defined(__GLIBC__)
  // nacl-newlib's semaphores do not currently support the pshared
  // option, so this should fail with an ENOSYS error.  If pshared
  // gets added, we should begin testing it for proper successful
  // behavior.
  EXPECT(-1 == sem_init(&my_semaphore, 1, 0));
  EXPECT(ENOSYS == errno);
#endif

  END_TEST();
}

// Test error conditions of sem_post and return the number of failed checks.
//
// According to the man page on Linux:
// ===================================
// RETURN VALUE
//        sem_post() returns 0 on success; on error, the value of the semaphore
//        is left unchanged, -1 is returned, and errno is set to indicate the
//        error.
//
//  ERRORS
//         EINVAL sem is not a valid semaphore.
//
//         EOVERFLOW
//                The maximum allowable value for a semaphore would be exceeded.
// ===================================
int TestSemPostErrors() {
  START_TEST("sem_post error conditions");

  // Initialize a semaphore with the max value, and try to post to it.
  sem_t my_semaphore;
  EXPECT(0 == sem_init(&my_semaphore, 0, SEM_VALUE_MAX));
  EXPECT(-1 == sem_post(&my_semaphore));
  EXPECT(EOVERFLOW == errno);
  EXPECT(0 == sem_destroy(&my_semaphore));

  END_TEST();
}

// The real type of the void* argument to PostThreadFunc.  See PostThreadFunc
// for more information.
struct PostThreadArg {
  // The semaphore to which PostThreadFunc will post.
  sem_t* semaphore;
  // An amount of time to sleep before each post (in microseconds).
  unsigned int sleep_microseconds;
  // The number of times to post before exiting the function.
  unsigned int iterations;
};

// Post to the given semaphore some number of times, with a sleep before each
// post.  poster_thread_arg must be of type PosterThreadArg.  Returns NULL.
void* PostThreadFunc(void* poster_thread_arg) {
  PostThreadArg* pta = static_cast<PostThreadArg*>(poster_thread_arg);
  for (unsigned int i = 0; i < pta->iterations; ++i) {
    usleep(pta->sleep_microseconds);
    sem_post(pta->semaphore);
  }
  return NULL;
}

int TestSemNormalOperation() {
  START_TEST("semaphore normal operation");

  // Test 1 thread posting to 1 semaphore.
  sem_t my_semaphore;
  EXPECT(0 == sem_init(&my_semaphore, 0, 0));
  PostThreadArg pta = { &my_semaphore, /* semaphore */
                        500000u, /* sleep_microseconds */
                        1 /* iterations */ };
  pthread_t my_thread;
  EXPECT(0 == pthread_create(&my_thread, 0, &PostThreadFunc, &pta));
  EXPECT(0 == sem_wait(&my_semaphore));
  EXPECT(0 == pthread_join(my_thread, 0));
  EXPECT(0 == sem_destroy(&my_semaphore));

  // Reinitialize a previously used semaphore, test 10 threads posting to 1
  // semaphore, 5 times each.
  EXPECT(0 == sem_init(&my_semaphore, 0, 0));
  pta.iterations = 5;
  pthread_t my_thread_array[10];
  for (int i = 0; i < 10; ++i) {
    EXPECT(0 == pthread_create(&my_thread_array[i], 0, &PostThreadFunc, &pta));
  }
  // Wait 5*10 times, once per post:  5 posts for each of 10 posting-threads.
  for (int i = 0; i < 5*10; ++i) {
    EXPECT(0 == sem_wait(&my_semaphore));
  }
  for (int i = 0; i < 10; ++i) {
    EXPECT(0 == pthread_join(my_thread_array[i], 0));
  }
  EXPECT(0 == sem_destroy(&my_semaphore));

  // Reinitialize the previously used semaphore again, this time with a positive
  // starting value.
  EXPECT(0 == sem_init(&my_semaphore, 0, 5));
  pta.iterations = 1;
  EXPECT(0 == pthread_create(&my_thread, 0, &PostThreadFunc, &pta));
  // Wait 6 times, once for the post, 5 times for the initial starting value.
  for (int i = 0; i < 6; ++i) {
    EXPECT(0 == sem_wait(&my_semaphore));
  }
  EXPECT(0 == pthread_join(my_thread, 0));
  EXPECT(0 == sem_destroy(&my_semaphore));

  END_TEST();
}

int TestSemTryWait() {
  START_TEST("test sem_trywait() and sem_getvalue()");

  int start_value = 10;
  sem_t sem;
  EXPECT(0 == sem_init(&sem, 0, start_value));

  int value = -1;
  EXPECT(0 == sem_getvalue(&sem, &value));
  EXPECT(10 == value);
  // When the semaphore's value is positive, each call to
  // sem_trywait() should decrement the semaphore's value.
  for (int i = 1; i <= start_value; i++) {
    EXPECT(0 == sem_trywait(&sem));
    EXPECT(0 == sem_getvalue(&sem, &value));
    EXPECT(start_value - i == value);
  }
  // When the semaphore's value is zero, sem_trywait() should fail.
  EXPECT(-1 == sem_trywait(&sem));
  EXPECT(EAGAIN == errno);
  EXPECT(0 == sem_getvalue(&sem, &value));
  EXPECT(0 == value);

  EXPECT(0 == sem_destroy(&sem));

  END_TEST();
}

int main() {
  int fail_count = 0;
  fail_count += TestSemInitErrors();
  fail_count += TestSemPostErrors();
  fail_count += TestSemNormalOperation();
  fail_count += TestSemTryWait();
  std::exit(fail_count);
}
