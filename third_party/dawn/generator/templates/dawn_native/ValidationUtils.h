//* Copyright 2018 The Dawn Authors
//*
//* Licensed under the Apache License, Version 2.0 (the "License");
//* you may not use this file except in compliance with the License.
//* You may obtain a copy of the License at
//*
//*     http://www.apache.org/licenses/LICENSE-2.0
//*
//* Unless required by applicable law or agreed to in writing, software
//* distributed under the License is distributed on an "AS IS" BASIS,
//* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//* See the License for the specific language governing permissions and
//* limitations under the License.

#ifndef BACKEND_VALIDATIONUTILS_H_
#define BACKEND_VALIDATIONUTILS_H_

#include "dawn/webgpu_cpp.h"

#include "dawn_native/Error.h"

namespace dawn_native {

    // Helper functions to check the value of enums and bitmasks
    {% for type in by_category["enum"] + by_category["bitmask"] %}
        MaybeError Validate{{type.name.CamelCase()}}(wgpu::{{as_cppType(type.name)}} value);
    {% endfor %}

} // namespace dawn_native

#endif  // BACKEND_VALIDATIONUTILS_H_
