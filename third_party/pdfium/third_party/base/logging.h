// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDFIUM_THIRD_PARTY_BASE_LOGGING_H_
#define PDFIUM_THIRD_PARTY_BASE_LOGGING_H_

#include <assert.h>
#include <stdlib.h>

#ifndef _WIN32
#define NULL_DEREF_IF_POSSIBLE \
  *(reinterpret_cast<volatile char*>(NULL) + 42) = 0x42;
#else
#define NULL_DEREF_IF_POSSIBLE
#endif

#define CHECK(condition)   \
  if (!(condition)) {      \
    abort();               \
    NULL_DEREF_IF_POSSIBLE \
  }

// TODO(palmer): These are quick hacks to import PartitionAlloc with minimum
// hassle. Look into pulling in the real DCHECK definition. It might be more
// than we need, or have more dependencies than we want. In the meantime, this
// is safe, at the cost of some performance.
#define DCHECK CHECK
#define DCHECK_EQ(x, y) CHECK((x) == (y))
#define DCHECK_IS_ON() true

// TODO(palmer): Also a quick hack. IMMEDIATE_CRASH used to be simple in
// Chromium base/, but it got way more complicated and has lots of base/
// dependencies now. Sad!
#define IMMEDIATE_CRASH() abort();

#define NOTREACHED() assert(false)

#endif  // PDFIUM_THIRD_PARTY_BASE_LOGGING_H_
