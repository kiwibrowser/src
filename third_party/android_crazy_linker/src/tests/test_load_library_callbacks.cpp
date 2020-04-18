// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A crazy linker test to test callbacks for delayed execution.

#include <pthread.h>
#include <stdio.h>
#include <crazy_linker.h>

#include "test_util.h"

namespace {

typedef void (*FunctionPtr)();

// Data block passed between callback poster and callback handler.
class CallbackData {
 public:
  CallbackData() {
    callback_.handler = NULL;
    callback_.opaque = NULL;
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
  }

  crazy_callback_t callback_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_;
};

bool PostCallback(crazy_callback_t* callback, void* poster_opaque) {
  printf("Post callback, poster_opaque %p, handler %p, opaque %p\n",
         poster_opaque,
         callback->handler,
         callback->opaque);

  CallbackData* callback_data = reinterpret_cast<CallbackData*>(poster_opaque);

  // Set callback_ and signal the arrival of the PostCallback() call.
  pthread_mutex_lock(&callback_data->mutex_);
  callback_data->callback_ = *callback;
  pthread_cond_signal(&callback_data->cond_);
  pthread_mutex_unlock(&callback_data->mutex_);

  return true;
}

void CheckAndRunCallback(CallbackData* callback_data) {
  printf("Run callback, callback_data %p\n", callback_data);

  if (!callback_data->callback_.handler) {
    Panic("Post for delayed execution not invoked\n");
  }

  // Run the callback, then clear it.
  crazy_callback_run(&callback_data->callback_);
  callback_data->callback_.handler = NULL;
  callback_data->callback_.opaque = NULL;
}

struct ThreadData {
  crazy_library_t* library;
  crazy_context_t* context;
};

void* ThreadBody(void *thread_arg) {
  const ThreadData* thread_data = reinterpret_cast<ThreadData*>(thread_arg);

  // Close the library, asynchronously.
  crazy_library_close_with_context(thread_data->library, thread_data->context);
  pthread_exit(NULL);
}

pthread_t AsyncCrazyLibraryCloseWithContext(crazy_library_t* library,
                                            crazy_context_t* context,
                                            CallbackData* callback_data) {
  printf("Async close, library %p, context %p\n", library, context);

  ThreadData thread_data = {library, context};
  void* thread_arg = reinterpret_cast<void*>(&thread_data);

  // Clear the indication that the new thread has called PostCallback().
  pthread_mutex_lock(&callback_data->mutex_);
  callback_data->callback_.handler = NULL;
  callback_data->callback_.opaque = NULL;
  pthread_mutex_unlock(&callback_data->mutex_);

  // Start the thread that closes the library.
  pthread_t thread;
  if (pthread_create(&thread, NULL, ThreadBody, thread_arg) != 0) {
    Panic("Failed to create thread for close\n");
  }

  // Wait for the library close to call PostCallback() before returning.
  printf("Waiting for PostCallback() call\n");
  pthread_mutex_lock(&callback_data->mutex_);
  while (!callback_data->callback_.handler) {
    pthread_cond_wait(&callback_data->cond_, &callback_data->mutex_);
  }
  pthread_mutex_unlock(&callback_data->mutex_);
  printf("Done waiting for PostCallback() call\n");

  return thread;
}

}  // namespace

#define LIB_NAME "libcrazy_linker_tests_libfoo.so"

int main() {
  crazy_context_t* context = crazy_context_create();
  crazy_library_t* library;

  // DEBUG
  crazy_context_set_load_address(context, 0x20000000);

  // Set a callback poster.
  CallbackData callback_data;
  crazy_context_set_callback_poster(context, &PostCallback, &callback_data);

  crazy_callback_poster_t poster;
  void* poster_opaque;

  // Check that the API returns the values we set.
  crazy_context_get_callback_poster(context, &poster, &poster_opaque);
  if (poster != &PostCallback || poster_opaque != &callback_data) {
    Panic("Get callback poster error\n");
  }

  // Load library
  if (!crazy_library_open(&library, LIB_NAME, context)) {
    Panic("Could not open library: %s\n", crazy_context_get_error(context));
  }
  CheckAndRunCallback(&callback_data);

  // Find the "Foo" symbol.
  FunctionPtr foo_func;
  if (!crazy_library_find_symbol(
           library, "Foo", reinterpret_cast<void**>(&foo_func))) {
    Panic("Could not find 'Foo' in %s\n", LIB_NAME);
  }

  // Call it.
  (*foo_func)();

  // Close the library.  Because the close operation will wait for the
  // callback before returning, we have to run it in a separate thread, and
  // wait for it to call PostCallback() before continuing.
  pthread_t thread =
      AsyncCrazyLibraryCloseWithContext(library, context, &callback_data);
  CheckAndRunCallback(&callback_data);

  if (pthread_join(thread, NULL) != 0) {
    Panic("Failed to join thread for close\n");
  }

  crazy_context_destroy(context);
  return 0;
}
