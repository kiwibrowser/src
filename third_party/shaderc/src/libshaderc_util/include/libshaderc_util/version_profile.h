// Copyright 2015 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LIBSHADERC_UTIL_INC_VERSION_PROFILE_H_
#define LIBSHADERC_UTIL_INC_VERSION_PROFILE_H_

#include <string>

#include "glslang/MachineIndependent/Versions.h"

namespace shaderc_util {

// Returns true if the given version is an accepted GLSL (ES) version.
inline bool IsKnownVersion(int version) {
  return version == 100 || version == 110 || version == 120 || version == 130 ||
         version == 140 || version == 150 || version == 300 || version == 330 ||
         version == 310 || version == 400 || version == 410 || version == 420 ||
         version == 430 || version == 440 || version == 450;
}

// Given a string version_profile containing both version and profile, decodes
// it and puts the decoded version in version, decoded profile in profile.
// Returns true if decoding is successful and version and profile are accepted.
bool ParseVersionProfile(const std::string& version_profile, int* version,
                         EProfile* profile);

}  // namespace shaderc_util

#endif  // LIBSHADERC_UTIL_INC_VERSION_PROFILE_H_
