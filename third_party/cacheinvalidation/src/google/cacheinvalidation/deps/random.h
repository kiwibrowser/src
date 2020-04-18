// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GOOGLE_CACHEINVALIDATION_DEPS_RANDOM_H_
#define GOOGLE_CACHEINVALIDATION_DEPS_RANDOM_H_

#error This file should be replaced with an implementation of the following \
  interface.

namespace invalidation {

class Random {
 public:
  explicit Random(int64_t seed);

  // Returns a pseudorandom value between 0 (inclusive) and 1 (exclusive).
  virtual double RandDouble();

  // Returns a pseudorandom unsigned 64-bit number.
  virtual uint64_t RandUint64();
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_DEPS_RANDOM_H_
