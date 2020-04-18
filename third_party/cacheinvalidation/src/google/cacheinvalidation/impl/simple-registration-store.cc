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

// Simple, map-based implementation of DigestStore.

#include "google/cacheinvalidation/impl/simple-registration-store.h"

#include <stddef.h>

#include "google/cacheinvalidation/impl/object-id-digest-utils.h"

namespace invalidation {

bool SimpleRegistrationStore::Add(const ObjectIdP& oid) {
  const string digest = ObjectIdDigestUtils::GetDigest(oid, digest_function_);
  bool will_add = (registrations_.find(digest) == registrations_.end());
  if (will_add) {
    registrations_[digest] = oid;
    RecomputeDigest();
  }
  return will_add;
}

void SimpleRegistrationStore::Add(const vector<ObjectIdP>& oids,
                                  vector<ObjectIdP>* oids_to_send) {
  for (size_t i = 0; i < oids.size(); ++i) {
    const ObjectIdP& oid = oids[i];
    const string digest = ObjectIdDigestUtils::GetDigest(oid, digest_function_);
    bool will_add = (registrations_.find(digest) == registrations_.end());
    if (will_add) {
      registrations_[digest] = oid;
      oids_to_send->push_back(oid);
    }
  }
  if (!oids_to_send->empty()) {
    // Only recompute the digest if we made changes.
    RecomputeDigest();
  }
}

bool SimpleRegistrationStore::Remove(const ObjectIdP& oid) {
  const string digest = ObjectIdDigestUtils::GetDigest(oid, digest_function_);
  bool will_remove = (registrations_.find(digest) != registrations_.end());
  if (will_remove) {
    registrations_.erase(digest);
    RecomputeDigest();
  }
  return will_remove;
}

void SimpleRegistrationStore::Remove(const vector<ObjectIdP>& oids,
                                     vector<ObjectIdP>* oids_to_send) {
  for (size_t i = 0; i < oids.size(); ++i) {
    const ObjectIdP& oid = oids[i];
    const string digest = ObjectIdDigestUtils::GetDigest(oid, digest_function_);
    bool will_remove = (registrations_.find(digest) != registrations_.end());
    if (will_remove) {
      registrations_.erase(digest);
      oids_to_send->push_back(oid);
    }
  }
  if (!oids_to_send->empty()) {
    // Only recompute the digest if we made changes.
    RecomputeDigest();
  }
}

void SimpleRegistrationStore::RemoveAll(vector<ObjectIdP>* oids) {
  for (map<string, ObjectIdP>::const_iterator iter = registrations_.begin();
       iter != registrations_.end(); ++iter) {
    oids->push_back(iter->second);
  }
  registrations_.clear();
  RecomputeDigest();
}

bool SimpleRegistrationStore::Contains(const ObjectIdP& oid) {
  return registrations_.find(
      ObjectIdDigestUtils::GetDigest(oid, digest_function_)) !=
      registrations_.end();
}

void SimpleRegistrationStore::GetElements(
    const string& oid_digest_prefix, int prefix_len,
    vector<ObjectIdP>* result) {
  // We always return all the registrations and let the Ticl sort it out.
  for (map<string, ObjectIdP>::iterator iter = registrations_.begin();
       iter != registrations_.end(); ++iter) {
    result->push_back(iter->second);
  }
}

void SimpleRegistrationStore::RecomputeDigest() {
  digest_ = ObjectIdDigestUtils::GetDigest(
      registrations_, digest_function_);
}

}  // namespace invalidation
