// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_FLAC_INCLUDE_SHARE_ALLOC_H
#define THIRD_PARTY_FLAC_INCLUDE_SHARE_ALLOC_H

void *safe_malloc_(size_t size);

void *safe_calloc_(size_t num_items, size_t size);

void *safe_malloc_add_2op_(size_t size1, size_t size2);

void *safe_malloc_mul_2op_(size_t size1, size_t size2);

void *safe_malloc_muladd2_(size_t size1, size_t size2, size_t size3);

void *safe_realloc_mul_2op_(void *ptr, size_t size1, size_t size2);

#endif  // THIRD_PARTY_FLAC_INCLUDE_SHARE_ALLOC_H
