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

#include "dawn_native/ValidationUtils_autogen.h"

namespace dawn_native {

    {% for type in by_category["enum"] %}
        MaybeError Validate{{type.name.CamelCase()}}(wgpu::{{as_cppType(type.name)}} value) {
            switch (value) {
                {% for value in type.values if value.valid %}
                    case wgpu::{{as_cppType(type.name)}}::{{as_cppEnum(value.name)}}:
                        return {};
                {% endfor %}
                default:
                    return DAWN_VALIDATION_ERROR("Invalid value for {{as_cType(type.name)}}");
            }
        }

    {% endfor %}

    {% for type in by_category["bitmask"] %}
        MaybeError Validate{{type.name.CamelCase()}}(wgpu::{{as_cppType(type.name)}} value) {
            if ((value & static_cast<wgpu::{{as_cppType(type.name)}}>(~{{type.full_mask}})) == 0) {
                return {};
            }
            return DAWN_VALIDATION_ERROR("Invalid value for {{as_cType(type.name)}}");
        }

    {% endfor %}

} // namespace dawn_native
