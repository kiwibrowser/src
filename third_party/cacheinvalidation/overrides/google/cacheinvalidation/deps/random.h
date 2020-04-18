// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_CACHEINVALIDATION_DEPS_RANDOM_H_
#define GOOGLE_CACHEINVALIDATION_DEPS_RANDOM_H_

#include <stdint.h>

#include "base/rand_util.h"

namespace invalidation {

class Random {
 public:
  // We don't actually use the seed.
  explicit Random(int64_t seed) {}

  virtual ~Random() {}

  // Returns a pseudorandom value between(inclusive) and(exclusive).
  virtual double RandDouble();

  virtual uint64_t RandUint64();
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_DEPS_RANDOM_H_
