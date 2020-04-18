/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This provides a private allocator to be used only within the IRT.
 * This is distinct from the user application's allocators.  This
 * allocator cannot use sbrk (the NaCl brk syscall), which is reserved
 * for the user application.
 *
 * NOTE: However, this allocator is exposed to PPAPI applications via the
 * PPB_Core MemAlloc and MemFree function pointers.  That should go away.
 * See http://code.google.com/p/chromium/issues/detail?id=80610
 */

#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "native_client/src/untrusted/irt/irt_private.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"

/*
 * These macros parameterize the dlmalloc code the way we want it.
 * See dlmalloc/malloc.c for the details.
 *
 * We of course do not lack <time.h>, but this prevents malloc from calling
 * time() in its initialization, which is a dependency we want to avoid.
 */
#define LACKS_TIME_H            1
#define USE_LOCKS               1
#define USE_SPIN_LOCKS          1
#define HAVE_MORECORE           0  /* Don't try to use sbrk() */
#define HAVE_MMAP               1
#define HAVE_MREMAP             0
#define NO_MALLINFO             1
#define NO_MALLOC_STATS         1

/*
 * This is called before malloc et al return NULL.
 *
 * In early startup of the IRT itself, we cannot recover from failure.
 */
#define MALLOC_FAILURE_ACTION   irt_malloc_failure()
static void irt_malloc_failure(void) {
  if (!__nc_thread_initialized) {
    static const char msg[] = "Memory allocation failure in IRT startup!\n";
    write(2, msg, sizeof msg - 1);
    _exit(-1);
  }

  errno = ENOMEM;
}

/* @IGNORE_LINES_FOR_CODE_HYGIENE[1] */
#include "native_client/src/third_party/dlmalloc/malloc.c"

/*
 * Crufty newlib internals use these entry points rather than the standard ones.
 * We must define them here to avoid bringing in newlib's allocator too.
 */

void *_malloc_r(struct _reent *ignored, size_t size) {
  return malloc(size);
}

void *_calloc_r(struct _reent *ignored, size_t n, size_t size) {
  return calloc(n, size);
}

void *_realloc_r(struct _reent *ignored, void *ptr, size_t size) {
  return realloc(ptr, size);
}

void _free_r(struct _reent *ignored, void *ptr) {
  free(ptr);
}

/*
 * This must never be called.  It's here to catch any stray calls.
 */
void *sbrk(intptr_t increment) {
  static const char msg[] = "BUG! IRT code called sbrk\n";
  write(2, msg, sizeof(msg) - 1);
  _exit(-1);
  return NULL;
}
