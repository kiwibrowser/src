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

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_PROTO_CONVERTER_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_PROTO_CONVERTER_H_

#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"

namespace invalidation {

class ProtoConverter {
 public:
  /* Converts an object id protocol buffer 'object_id_proto' to the
   * corresponding external type 'object_id'.
   */
  static void ConvertFromObjectIdProto(
      const ObjectIdP& object_id_proto, ObjectId* object_id);

  /* Converts an object id 'object_id' to the corresponding protocol buffer
   * 'object_id_proto'.
   */
  static void ConvertToObjectIdProto(
      const ObjectId& object_id, ObjectIdP* object_id_proto);

  /* Converts an invalidation protocol buffer 'invalidation_proto' to the
   * corresponding external object 'invalidation'.
   */
  static void ConvertFromInvalidationProto(
      const InvalidationP& invalidation_proto, Invalidation* invalidation);

  /* Converts an invalidation to the corresponding protocol
   * buffer and returns it.
   */
  static void ConvertToInvalidationProto(
      const Invalidation& invalidation, InvalidationP* invalidation_proto);

  static bool IsAllObjectIdP(const ObjectIdP& object_id_proto) {
    return (object_id_proto.source() == ObjectSource_Type_INTERNAL) &&
        (object_id_proto.name() == "");
  }

 private:
  ProtoConverter() {
    // To prevent instantiation.
  }
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_PROTO_CONVERTER_H_
