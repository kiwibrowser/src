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

// Helper utilities for dealing with protocol buffers.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_PROTO_HELPERS_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_PROTO_HELPERS_H_

#include <stdint.h>

#include <sstream>
#include <string>

#include "google/cacheinvalidation/client_protocol.pb.h"
#include "google/cacheinvalidation/deps/logging.h"
#include "google/cacheinvalidation/deps/stl-namespace.h"
#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"
#include "google/cacheinvalidation/impl/constants.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::string;
using ::ipc::invalidation::ProtocolVersion;

// Functor to compare various protocol messages.
struct ProtoCompareLess {
  bool operator()(const ObjectIdP& object_id1,
                  const ObjectIdP& object_id2) const {
    // If the sources differ, then the one with the smaller source is the
    // smaller object id.
    int source_diff = object_id1.source() - object_id2.source();
    if (source_diff != 0) {
      return source_diff < 0;
    }
    // Otherwise, the one with the smaller name is the smaller object id.
    return object_id1.name().compare(object_id2.name()) < 0;
  }

  bool operator()(const InvalidationP& inv1,
                  const InvalidationP& inv2) const {
    const ProtoCompareLess& compare_less_than = *this;
    // If the object ids differ, then the one with the smaller object id is the
    // smaller invalidation.
    if (compare_less_than(inv1.object_id(), inv2.object_id())) {
      return true;
    }
    if (compare_less_than(inv2.object_id(), inv1.object_id())) {
      return false;
    }

    // Otherwise, the object ids are the same, so we need to look at the
    // versions.

    // We define an unknown version to be less than a known version.
    int64_t known_version_diff =
        inv1.is_known_version() - inv2.is_known_version();
    if (known_version_diff != 0) {
      return known_version_diff < 0;
    }

    // Otherwise, they're both known both unknown, so the one with the smaller
    // version is the smaller invalidation.
    return inv1.version() < inv2.version();
  }

  bool operator()(const RegistrationSubtree& reg_subtree1,
                  const RegistrationSubtree& reg_subtree2) const {
    const RepeatedPtrField<ObjectIdP>& objects1 =
        reg_subtree1.registered_object();
    const RepeatedPtrField<ObjectIdP>& objects2 =
        reg_subtree2.registered_object();
    // If they have different numbers of objects, the one with fewer is smaller.
    if (objects1.size() != objects2.size()) {
      return objects1.size() < objects2.size();
    }
    // Otherwise, compare the object ids in order.
    RepeatedPtrField<ObjectIdP>::const_iterator iter1, iter2;
    const ProtoCompareLess& compare_less_than = *this;
    for (iter1 = objects1.begin(), iter2 = objects2.begin();
         iter1 != objects1.end(); ++iter1, ++iter2) {
      if (compare_less_than(*iter1, *iter2)) {
        return true;
      }
      if (compare_less_than(*iter2, *iter1)) {
        return false;
      }
    }
    // The registration subtrees are the same.
    return false;
  }
};

// Other protocol message utilities.
class ProtoHelpers {
 public:
  // Converts a value to a printable/readable string format.
  template<typename T>
  static string ToString(const T& value);

  // Initializes |reg| to be a (un) registration for object |oid|.
  static void InitRegistrationP(const ObjectIdP& oid,
      RegistrationP::OpType op_type, RegistrationP* reg);

  static void InitInitializeMessage(
      const ApplicationClientIdP& application_client_id, const string& nonce,
      InitializeMessage* init_msg);

  // Initializes |protocol_version| to the current protocol version.
  static void InitProtocolVersion(ProtocolVersion* protocol_version);

  // Initializes |client_version| to the current client version.
  static void InitClientVersion(const string& platform,
      const string& application_info, ClientVersion* client_version);

  // Initializes |config_version| to the current config version.
  static void InitConfigVersion(Version* config_version);

  // Initializes |rate_limit| with the given window interval and count of
  // messages.
  static void InitRateLimitP(int window_ms, int count, RateLimitP *rate_limit);

 private:
  static const int NUM_CHARS = 256;
  static char CHAR_OCTAL_STRINGS1[NUM_CHARS];
  static char CHAR_OCTAL_STRINGS2[NUM_CHARS];
  static char CHAR_OCTAL_STRINGS3[NUM_CHARS];

  // Have the above arrays been initialized or not.
  static bool is_initialized;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_PROTO_HELPERS_H_
