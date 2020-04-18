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

//
// length_limits.h
//

#if !defined(__LENGTH_LIMITS_H)
#define __LENGTH_LIMITS_H 1

// These constants are factored out from the rest of the headers to
// make it easier to reference them from the compiler sources.

// These lengths do not include the NULL terminator.
#define MAX_SYMBOL_NAME_LEN 256
#define MAX_STRING_LEN 511

#endif // !(defined(__LENGTH_LIMITS_H)
