// Copyright 2018 The Dawn Authors
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

#ifndef DAWNNATIVE_EXPORT_H_
#define DAWNNATIVE_EXPORT_H_

#if defined(DAWN_NATIVE_SHARED_LIBRARY)
#    if defined(_WIN32)
#        if defined(DAWN_NATIVE_IMPLEMENTATION)
#            define DAWN_NATIVE_EXPORT __declspec(dllexport)
#        else
#            define DAWN_NATIVE_EXPORT __declspec(dllimport)
#        endif
#    else  // defined(_WIN32)
#        if defined(DAWN_NATIVE_IMPLEMENTATION)
#            define DAWN_NATIVE_EXPORT __attribute__((visibility("default")))
#        else
#            define DAWN_NATIVE_EXPORT
#        endif
#    endif  // defined(_WIN32)
#else       // defined(DAWN_NATIVE_SHARED_LIBRARY)
#    define DAWN_NATIVE_EXPORT
#endif  // defined(DAWN_NATIVE_SHARED_LIBRARY)

#endif  // DAWNNATIVE_EXPORT_H_
