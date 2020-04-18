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

// Various constants common to clients and servers used in version 2 of the
// Ticl.

#include "google/cacheinvalidation/impl/build_constants.h"
#include "google/cacheinvalidation/impl/constants.h"

namespace invalidation {

const int Constants::kClientMajorVersion = 3;
const int Constants::kClientMinorVersion = BUILD_DATESTAMP;
const int Constants::kProtocolMajorVersion = 3;
const int Constants::kProtocolMinorVersion = 2;
const int Constants::kConfigMajorVersion = 3;
const int Constants::kConfigMinorVersion = 2;
}  // namespace invalidation
