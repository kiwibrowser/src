// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits.h>
#if !defined _MSC_VER
#include <stdint.h>
#endif
#include <stdlib.h>
#include "share/alloc.h"

void *safe_malloc_(size_t size) {
  return malloc(size ? size : 1);
}

void *safe_calloc_(size_t num_items, size_t size) {
  if (num_items && size)
    return calloc(num_items, size);
  return malloc(1);
}

void *safe_malloc_add_2op_(size_t size1, size_t size2) {
  size_t size = size1 + size2;
  if (size < size1)
    return NULL;

  return safe_malloc_(size);
}

void *safe_malloc_mul_2op_(size_t size1, size_t size2) {
  if (!size1 || !size2)
    return malloc(1);

  if (size1 > SIZE_MAX / size2)
    return NULL;

  return malloc(size1 * size2);
}

void *safe_malloc_muladd2_(size_t size1, size_t size2, size_t size3) {
  size_t size = size2 + size3;
  if (size < size3)
    return 0;

  if (!size1 || !size)
    return malloc(1);

  return safe_malloc_mul_2op_(size1, size2);
}

void *safe_realloc_mul_2op_(void *ptr, size_t size1, size_t size2) {
  if (!size1 || !size2)
    return realloc(ptr, 0);

  if (size1 > SIZE_MAX / size2)
    return 0;

  return realloc(ptr, size1 * size2);
}
