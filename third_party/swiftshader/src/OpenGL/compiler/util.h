// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMPILER_UTIL_H
#define COMPILER_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

// atof_clamp is like atof but
//   1. it forces C locale, i.e. forcing '.' as decimal point.
//   2. it sets the value to FLT_MAX if overflow happens.
// Return false if overflow happens.
bool atof_clamp(const char *str, float *value);

// If overflow happens, value is set to INT_MAX.
// Return false if overflow happens.
bool atoi_clamp(const char *str, int *value);

// If overflow happens, value is set to UINT_MAX.
// Return false if overflow happens.
bool atou_clamp(const char *str, unsigned int *value);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // COMPILER_UTIL_H
