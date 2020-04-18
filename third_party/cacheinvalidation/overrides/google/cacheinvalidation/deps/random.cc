// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google/cacheinvalidation/deps/random.h"

namespace invalidation {

double Random::RandDouble() {
  return base::RandDouble();
}

uint64_t Random::RandUint64() {
  return base::RandUint64();
}

}  // namespace invalidation
