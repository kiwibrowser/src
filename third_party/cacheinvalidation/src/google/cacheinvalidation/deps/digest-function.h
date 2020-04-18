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

// Interface specifying a function to compute digests.

#ifndef GOOGLE_CACHEINVALIDATION_DEPS_DIGEST_FUNCTION_H_
#define GOOGLE_CACHEINVALIDATION_DEPS_DIGEST_FUNCTION_H_

#include <string>

#include "google/cacheinvalidation/deps/stl-namespace.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::string;

class DigestFunction {
 public:
  virtual ~DigestFunction() {}

  /* Clears the digest state. */
  virtual void Reset() = 0;

  /* Adds data to the digest being computed. */
  virtual void Update(const string& data) = 0;

  /* Stores the digest of the data added by Update. After this call has been
   * made, reset must be called before Update and GetDigest can be called.
   */
  virtual string GetDigest() = 0;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_DEPS_DIGEST_FUNCTION_H_
