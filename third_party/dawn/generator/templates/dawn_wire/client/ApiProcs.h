//* Copyright 2019 The Dawn Authors
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

#ifndef DAWNWIRE_CLIENT_APIPROCS_AUTOGEN_H_
#define DAWNWIRE_CLIENT_APIPROCS_AUTOGEN_H_

#include <dawn/dawn.h>

namespace dawn_wire { namespace client {

    //* Dawn API
    {% for type in by_category["object"] %}
        {% set cType = as_cType(type.name) %}
        {% for method in native_methods(type) %}
            {% set Suffix = as_MethodSuffix(type.name, method.name) %}
            {{as_cType(method.return_type.name)}} Client{{Suffix}}(
              {{-cType}} cSelf
              {%- for arg in method.arguments -%}
                  , {{as_annotated_cType(arg)}}
              {%- endfor -%}
            );
        {% endfor %}
    {% endfor %}

}}  // namespace dawn_wire::client

#endif  // DAWNWIRE_CLIENT_APIPROCS_AUTOGEN_H_
