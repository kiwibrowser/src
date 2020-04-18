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

// Digest-related utilities for object ids.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_OBJECT_ID_DIGEST_UTILS_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_OBJECT_ID_DIGEST_UTILS_H_

#include <map>

#include "google/cacheinvalidation/deps/digest-function.h"
#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::map;

class ObjectIdDigestUtils {
 public:
  /* Returns the digest of the set of keys in the given map. */
  template<typename T>
  static string GetDigest(
      map<string, T> registrations, DigestFunction* digest_fn) {
    digest_fn->Reset();
    for (map<string, ObjectIdP>::iterator iter = registrations.begin();
         iter != registrations.end(); ++iter) {
      digest_fn->Update(iter->first);
    }
    return digest_fn->GetDigest();
  }

  /* Returns the digest of object_id using digest_fn. */
  static string GetDigest(
      const ObjectIdP& object_id, DigestFunction* digest_fn);
};
}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_OBJECT_ID_DIGEST_UTILS_H_
