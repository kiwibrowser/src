// Copyright (C) 2013 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

// A test used to check that __cxa_get_globals() does not use malloc.
// This will do the following:
//
//  - Lazily load libtest_malloc_lockup.so, which includes a copy of
//    GAbi++ linked with malloc() / free() functions that exit() with
//    an error if called.
//
//  - Create a large number of concurrent threads, and have each one
//    call the library's 'get_globals' function, which returns the
//    result of __cxa_get_globals() linked against the special mallocs,
//    then store the value in a global array.
//
//  - Tell all the threads to stop, wait for them to complete.
//
//  - Look at the values stored in the global arrays. They should not be NULL
//    (to indicate succesful allocation), and all different (each one should
//    correspond to a thread-specific instance of __cxa_eh_globals).
//
//  - Unload the library.
//

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void* (*get_globals_fn)();

static get_globals_fn g_get_globals;

// Number of threads to create. Must be > 4096 to really check slab allocation.
static const size_t kMaxThreads = 5000;

static pthread_t g_threads[kMaxThreads];
static void* g_thread_objects[kMaxThreads];

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond_exit = PTHREAD_COND_INITIALIZER;
static pthread_cond_t g_cond_counter = PTHREAD_COND_INITIALIZER;
static unsigned g_thread_count = 0;
static bool g_can_exit = false;

// Thread routine, just call 'get_globals' and store the result in our global
// array, then wait for an event from the main thread. This guarantees that
// no thread exits before another one starts, and thus that allocation slots
// are not reused.
static void* my_thread(void* param) {
  // Get thread-specific object pointer, store it in global array.
  int id = (int)(intptr_t)param;
  g_thread_objects[id] = (*g_get_globals)();

  // Increment global thread counter and tell the main thread about this.
  pthread_mutex_lock(&g_lock);
  g_thread_count += 1;
  pthread_cond_signal(&g_cond_counter);

  // The thread object will be automatically released/recycled when the thread
  // exits. Wait here until signaled by the main thread to avoid this.
  while (!g_can_exit)
    pthread_cond_wait(&g_cond_exit, &g_lock);
  pthread_mutex_unlock(&g_lock);

  return NULL;
}

int main(void) {
  // Load the library.
  void* lib = dlopen("libtest_malloc_lockup.so", RTLD_NOW);
  if (!lib) {
    fprintf(stderr, "ERROR: Can't find library: %s\n", strerror(errno));
    return 1;
  }

  // Extract 'get_globals' function address.
  g_get_globals = reinterpret_cast<get_globals_fn>(dlsym(lib, "get_globals"));
  if (!g_get_globals) {
    fprintf(stderr, "ERROR: Could not find 'get_globals' function: %s\n",
            dlerror());
    dlclose(lib);
    return 1;
  }

  // Use a smaller stack per thread to be able to create lots of them.
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 16384);

  // Start as many threads as needed.
  printf("Creating %d threads\n", kMaxThreads);
  for (size_t n = 0; n < kMaxThreads; ++n) {
    int ret = pthread_create(&g_threads[n], &attr, my_thread, (void*)n);
    if (ret != 0) {
      fprintf(stderr, "ERROR: Thread #%d creation error: %s\n",
              n + 1, strerror(errno));
      return 2;
    }
  }

  // Wait until they all ran, then tell them to exit.
  printf("Waiting for all threads to run\n");
  pthread_mutex_lock(&g_lock);
  while (g_thread_count < kMaxThreads)
    pthread_cond_wait(&g_cond_counter, &g_lock);

  printf("Waking up threads\n");
  g_can_exit = true;
  pthread_cond_broadcast(&g_cond_exit);
  pthread_mutex_unlock(&g_lock);

  // Wait for them to complete.
  printf("Waiting for all threads to complete\n");
  for (size_t n = 0; n < kMaxThreads; ++n) {
    void* dummy;
    pthread_join(g_threads[n], &dummy);
  }

  // Verify that the thread objects are all non-NULL and different.
  printf("Checking results\n");
  size_t failures = 0;
  const size_t kMaxFailures = 16;
  for (size_t n = 0; n < kMaxThreads; ++n) {
    void* obj = g_thread_objects[n];
    if (obj == NULL) {
      if (++failures < kMaxFailures)
        printf("Thread %d got a NULL object!\n", n + 1);
    } else {
      for (size_t m = n + 1; m < kMaxThreads; ++m) {
        if (g_thread_objects[m] == obj) {
          if (++failures < kMaxFailures)
            printf("Thread %d has same object as thread %d (%p)\n",
                   n + 1, m + 1, obj);
        }
      }
    }
  }

  // We're done.
  dlclose(lib);
  if (failures > 0) {
    fprintf(stderr, "%d failures detected!\n", failures);
    return 1;
  }

  printf("All OK!\n");
  return 0;
}
