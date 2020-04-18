// Copyright 2013 Google Inc.
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

//
// Brings RepeatedField classes into invalidation namespace.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_REPEATED_FIELD_NAMESPACE_FIX_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_REPEATED_FIELD_NAMESPACE_FIX_H_

#include "google/protobuf/repeated_field.h"

namespace invalidation {

using ::google::protobuf::RepeatedField;
using ::google::protobuf::RepeatedPtrField;

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_REPEATED_FIELD_NAMESPACE_FIX_H_
