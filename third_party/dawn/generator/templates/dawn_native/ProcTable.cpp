//* Copyright 2017 The Dawn Authors
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

#include "dawn_native/dawn_platform.h"
#include "dawn_native/DawnNative.h"

#include <algorithm>
#include <vector>

{% for type in by_category["object"] %}
    {% if type.name.canonical_case() not in ["texture view"] %}
        #include "dawn_native/{{type.name.CamelCase()}}.h"
    {% endif %}
{% endfor %}

namespace dawn_native {

    // Type aliases to make all frontend types appear as if they have "Base" at the end when some
    // of them are actually pure-frontend and don't have the Base.
    using CommandEncoderBase = CommandEncoder;
    using ComputePassEncoderBase = ComputePassEncoder;
    using FenceBase = Fence;
    using RenderPassEncoderBase = RenderPassEncoder;
    using RenderBundleEncoderBase = RenderBundleEncoder;
    using SurfaceBase = Surface;

    namespace {

        {% for type in by_category["object"] %}
            {% for method in c_methods(type) %}
                {% set suffix = as_MethodSuffix(type.name, method.name) %}

                {{as_cType(method.return_type.name)}} Native{{suffix}}(
                    {{-as_cType(type.name)}} cSelf
                    {%- for arg in method.arguments -%}
                        , {{as_annotated_cType(arg)}}
                    {%- endfor -%}
                ) {
                    //* Perform conversion between C types and frontend types
                    auto self = reinterpret_cast<{{as_frontendType(type)}}>(cSelf);

                    {% for arg in method.arguments %}
                        {% set varName = as_varName(arg.name) %}
                        {% if arg.type.category in ["enum", "bitmask"] %}
                            auto {{varName}}_ = static_cast<{{as_frontendType(arg.type)}}>({{varName}});
                        {% elif arg.annotation != "value" or arg.type.category == "object" %}
                            auto {{varName}}_ = reinterpret_cast<{{decorate("", as_frontendType(arg.type), arg)}}>({{varName}});
                        {% else %}
                            auto {{varName}}_ = {{as_varName(arg.name)}};
                        {% endif %}
                    {%- endfor-%}

                    {% if method.return_type.name.canonical_case() != "void" %}
                        auto result =
                    {%- endif %}
                    self->{{method.name.CamelCase()}}(
                        {%- for arg in method.arguments -%}
                            {%- if not loop.first %}, {% endif -%}
                            {{as_varName(arg.name)}}_
                        {%- endfor -%}
                    );
                    {% if method.return_type.name.canonical_case() != "void" %}
                        {% if method.return_type.category == "object" %}
                            return reinterpret_cast<{{as_cType(method.return_type.name)}}>(result);
                        {% else %}
                            return result;
                        {% endif %}
                    {% endif %}
                }
            {% endfor %}
        {% endfor %}

        struct ProcEntry {
            WGPUProc proc;
            const char* name;
        };
        static const ProcEntry sProcMap[] = {
            {% for (type, method) in c_methods_sorted_by_name %}
                { reinterpret_cast<WGPUProc>(Native{{as_MethodSuffix(type.name, method.name)}}), "{{as_cMethod(type.name, method.name)}}" },
            {% endfor %}
        };
        static constexpr size_t sProcMapSize = sizeof(sProcMap) / sizeof(sProcMap[0]);
    }

    WGPUInstance NativeCreateInstance(WGPUInstanceDescriptor const* cDescriptor) {
        const dawn_native::InstanceDescriptor* descriptor =
            reinterpret_cast<const dawn_native::InstanceDescriptor*>(cDescriptor);
        return reinterpret_cast<WGPUInstance>(InstanceBase::Create(descriptor));
    }

    WGPUProc NativeGetProcAddress(WGPUDevice, const char* procName) {
        if (procName == nullptr) {
            return nullptr;
        }

        const ProcEntry* entry = std::lower_bound(&sProcMap[0], &sProcMap[sProcMapSize], procName,
            [](const ProcEntry &a, const char *b) -> bool {
                return strcmp(a.name, b) < 0;
            }
        );

        if (entry != &sProcMap[sProcMapSize] && strcmp(entry->name, procName) == 0) {
            return entry->proc;
        }

        // Special case the two free-standing functions of the API.
        if (strcmp(procName, "wgpuGetProcAddress") == 0) {
            return reinterpret_cast<WGPUProc>(NativeGetProcAddress);
        }

        if (strcmp(procName, "wgpuCreateInstance") == 0) {
            return reinterpret_cast<WGPUProc>(NativeCreateInstance);
        }

        return nullptr;
    }

    std::vector<const char*> GetProcMapNamesForTestingInternal() {
        std::vector<const char*> result;
        result.reserve(sProcMapSize);
        for (const ProcEntry& entry : sProcMap) {
            result.push_back(entry.name);
        }
        return result;
    }

    DawnProcTable GetProcsAutogen() {
        DawnProcTable table;
        table.getProcAddress = NativeGetProcAddress;
        table.createInstance = NativeCreateInstance;
        {% for type in by_category["object"] %}
            {% for method in c_methods(type) %}
                table.{{as_varName(type.name, method.name)}} = Native{{as_MethodSuffix(type.name, method.name)}};
            {% endfor %}
        {% endfor %}
        return table;
    }

}
