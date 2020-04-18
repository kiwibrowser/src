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

// Utility methods for handling the Ticl persistent state.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_PERSISTENCE_UTILS_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_PERSISTENCE_UTILS_H_

#include <string>

#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/deps/digest-function.h"
#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"
#include "google/cacheinvalidation/impl/log-macro.h"

namespace invalidation {

class PersistenceUtils {
 public:
  /* Serializes a Ticl state blob. */
  static void SerializeState(
      PersistentTiclState state, DigestFunction* digest_fn, string* result);

  /* Deserializes a Ticl state blob. Returns whether the parsed state could be
   * parsed.
   */
  static bool DeserializeState(
      Logger* logger, const string& state_blob_bytes, DigestFunction* digest_fn,
      PersistentTiclState* ticl_state);

  /* Returns a message authentication code over state. */
  static string GenerateMac(
      const PersistentTiclState& state, DigestFunction* digest_fn);

 private:
  PersistenceUtils() {
    // Prevent instantiation.
  }
};  // class PersistenceUtils

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_PERSISTENCE_UTILS_H_
