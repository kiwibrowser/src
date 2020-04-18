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

#include "google/cacheinvalidation/impl/persistence-utils.h"

namespace invalidation {

void PersistenceUtils::SerializeState(
    PersistentTiclState state, DigestFunction* digest_fn, string* result) {
  string mac = GenerateMac(state, digest_fn);
  PersistentStateBlob blob;
  blob.mutable_ticl_state()->CopyFrom(state);
  blob.set_authentication_code(mac);
  blob.SerializeToString(result);
}

bool PersistenceUtils::DeserializeState(
    Logger* logger, const string& state_blob_bytes, DigestFunction* digest_fn,
    PersistentTiclState* ticl_state) {
  PersistentStateBlob state_blob;
  state_blob.ParseFromString(state_blob_bytes);
  if (!state_blob.IsInitialized()) {
    TLOG(logger, WARNING, "could not parse state blob from %s",
         state_blob_bytes.c_str());
    return false;
  }

  // Check the mac in the envelope against the recomputed mac from the state.
  ticl_state->CopyFrom(state_blob.ticl_state());
  const string& mac = GenerateMac(*ticl_state, digest_fn);
  if (mac != state_blob.authentication_code()) {
    TLOG(logger, WARNING, "Ticl state failed MAC check: computed %s vs %s",
         mac.c_str(), state_blob.authentication_code().c_str());
    return false;
  }
  return true;
}

string PersistenceUtils::GenerateMac(
    const PersistentTiclState& state, DigestFunction* digest_fn) {
  string serialized;
  state.SerializeToString(&serialized);
  digest_fn->Reset();
  digest_fn->Update(serialized);
  return digest_fn->GetDigest();
}

}  // namespace invalidation
