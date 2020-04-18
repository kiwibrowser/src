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

// Utilities to convert between protobufs and externally-exposed types in the
// Ticl.

#include "google/cacheinvalidation/impl/proto-converter.h"

#include "google/cacheinvalidation/deps/logging.h"
#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"

namespace invalidation {

void ProtoConverter::ConvertFromObjectIdProto(
    const ObjectIdP& object_id_proto, ObjectId* object_id) {
  object_id->Init(object_id_proto.source(), object_id_proto.name());
}

void ProtoConverter::ConvertToObjectIdProto(
    const ObjectId& object_id, ObjectIdP* object_id_proto) {
  object_id_proto->set_source(object_id.source());
  object_id_proto->set_name(object_id.name());
}

void ProtoConverter::ConvertFromInvalidationProto(
    const InvalidationP& invalidation_proto, Invalidation* invalidation) {
  ObjectId object_id;
  ConvertFromObjectIdProto(invalidation_proto.object_id(), &object_id);
  bool is_trickle_restart = invalidation_proto.is_trickle_restart();
  if (invalidation_proto.has_payload()) {
    invalidation->Init(object_id, invalidation_proto.version(),
                       invalidation_proto.payload(), is_trickle_restart);
  } else {
    invalidation->Init(object_id, invalidation_proto.version(),
                       is_trickle_restart);
  }
}

void ProtoConverter::ConvertToInvalidationProto(
    const Invalidation& invalidation, InvalidationP* invalidation_proto) {
  ConvertToObjectIdProto(
      invalidation.object_id(), invalidation_proto->mutable_object_id());
  invalidation_proto->set_version(invalidation.version());
  if (invalidation.has_payload()) {
    invalidation_proto->set_payload(invalidation.payload());
  }
  bool is_trickle_restart = invalidation.is_trickle_restart_for_internal_use();
  invalidation_proto->set_is_trickle_restart(is_trickle_restart);
}

}  // namespace invalidation
