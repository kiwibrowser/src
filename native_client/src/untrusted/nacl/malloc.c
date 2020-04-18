/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This provides the malloc implementation used with newlib.
 * We override the native newlib one because a newer one is better.
 */

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

/* @IGNORE_LINES_FOR_CODE_HYGIENE[1] */
#include "native_client/src/third_party/dlmalloc/malloc.c"

/*
 * Crufty newlib internals use these entry points rather than the standard ones.
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
