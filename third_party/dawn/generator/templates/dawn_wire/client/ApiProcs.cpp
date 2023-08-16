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

#include "common/Log.h"
#include "dawn_wire/client/ApiObjects.h"
#include "dawn_wire/client/Client.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace dawn_wire { namespace client {
    namespace {

        //* Outputs an rvalue that's the number of elements a pointer member points to.
        {% macro member_length(member, accessor) -%}
            {%- if member.length == "constant" -%}
                {{member.constant_length}}
            {%- else -%}
                {{accessor}}{{as_varName(member.length.name)}}
            {%- endif -%}
        {%- endmacro %}

        {% for type in by_category["object"] %}
            DAWN_DECLARE_UNUSED bool DeviceMatches(const Device* device, const {{as_cType(type.name)}} obj) {
                return device == reinterpret_cast<const {{as_wireType(type)}}>(obj)->device;
            }

            DAWN_DECLARE_UNUSED bool DeviceMatches(const Device* device, const {{as_cType(type.name)}} *const obj, uint32_t count = 1) {
                ASSERT(count == 0 || obj != nullptr);
                for (uint32_t i = 0; i < count; ++i) {
                    if (!DeviceMatches(device, obj[i])) {
                        return false;
                    }
                }
                return true;
            }
        {% endfor %}

        bool DeviceMatches(const Device* device, WGPUChainedStruct const* chainedStruct);

        {% for type in by_category["structure"] if type.may_have_dawn_object %}
            DAWN_DECLARE_UNUSED bool DeviceMatches(const Device* device, const {{as_cType(type.name)}}& obj) {
                {% if type.extensible %}
                    if (!DeviceMatches(device, obj.nextInChain)) {
                        return false;
                    }
                {% endif %}
                {% for member in type.members if member.type.may_have_dawn_object or member.type.category == "object" %}
                    {% if member.optional %}
                        if (obj.{{as_varName(member.name)}} != nullptr)
                    {% endif %}
                    {
                        if (!DeviceMatches(device, obj.{{as_varName(member.name)}}
                            {%- if member.annotation != "value" and member.length != "strlen" -%}
                                , {{member_length(member, "obj.")}}
                            {%- endif -%})) {
                            return false;
                        }
                    }
                {% endfor %}
                return true;
            }

            DAWN_DECLARE_UNUSED bool DeviceMatches(const Device* device, const {{as_cType(type.name)}} *const obj, uint32_t count = 1) {
                for (uint32_t i = 0; i < count; ++i) {
                    if (!DeviceMatches(device, obj[i])) {
                        return false;
                    }
                }
                return true;
            }
        {% endfor %}

        bool DeviceMatches(const Device* device, WGPUChainedStruct const* chainedStruct) {
            while (chainedStruct != nullptr) {
                switch (chainedStruct->sType) {
                    {% for sType in types["s type"].values if sType.valid %}
                        {% set CType = as_cType(sType.name) %}
                        case {{as_cEnum(types["s type"].name, sType.name)}}: {
                            {% if types[sType.name.get()].may_have_dawn_object %}
                                if (!DeviceMatches(device, reinterpret_cast<const {{CType}}*>(chainedStruct))) {
                                    return false;
                                }
                            {% endif %}
                            break;
                        }
                    {% endfor %}
                    case WGPUSType_Invalid:
                        break;
                    default:
                        UNREACHABLE();
                        dawn::WarningLog()
                            << "All objects may not be from the same device. "
                            << "Unknown sType " << chainedStruct->sType << " discarded.";
                        return false;
                }
                chainedStruct = chainedStruct->next;
            }
            return true;
        }

    }  // anonymous namespace

    //* Implementation of the client API functions.
    {% for type in by_category["object"] %}
        {% set Type = type.name.CamelCase() %}
        {% set cType = as_cType(type.name) %}

        {% for method in type.methods %}
            {% set Suffix = as_MethodSuffix(type.name, method.name) %}

            {% if Suffix in client_handwritten_commands %}
                static
            {% endif %}
            {{as_cType(method.return_type.name)}} Client{{Suffix}}(
                {{-cType}} cSelf
                {%- for arg in method.arguments -%}
                    , {{as_annotated_cType(arg)}}
                {%- endfor -%}
            ) {
                {% if len(method.arguments) > 0 %}
                    {
                        bool sameDevice = true;
                        auto self = reinterpret_cast<{{as_wireType(type)}}>(cSelf);
                        Device* device = self->device;
                        DAWN_UNUSED(device);

                        do {
                            {% for arg in method.arguments if arg.type.may_have_dawn_object or arg.type.category == "object" %}
                                {% if arg.optional %}
                                    if ({{as_varName(arg.name)}} != nullptr)
                                {% endif %}
                                {
                                    if (!DeviceMatches(device, {{as_varName(arg.name)}}
                                        {%- if arg.annotation != "value" and arg.length != "strlen" -%}
                                            , {{member_length(arg, "")}}
                                        {%- endif -%})) {
                                        sameDevice = false;
                                        break;
                                    }
                                }
                            {% endfor %}
                        } while (false);

                        if (DAWN_UNLIKELY(!sameDevice)) {
                            device->InjectError(WGPUErrorType_Validation,
                                                "All objects must be from the same device.");
                            {% if method.return_type.category == "object" %}
                                // Allocate an object without registering it on the server. This is backed by a real allocation on
                                // the client so commands can be sent with it. But because it's not allocated on the server, it will
                                // be a fatal error to use it.
                                auto self = reinterpret_cast<{{as_wireType(type)}}>(cSelf);
                                auto* allocation = self->device->GetClient()->{{method.return_type.name.CamelCase()}}Allocator().New(self->device);
                                return reinterpret_cast<{{as_cType(method.return_type.name)}}>(allocation->object.get());
                            {% elif method.return_type.name.canonical_case() == "void" %}
                                return;
                            {% else %}
                                return {};
                            {% endif %}
                        }
                    }
                {% endif %}

                auto self = reinterpret_cast<{{as_wireType(type)}}>(cSelf);
                {% if Suffix not in client_handwritten_commands %}
                    Device* device = self->device;
                    {{Suffix}}Cmd cmd;

                    //* Create the structure going on the wire on the stack and fill it with the value
                    //* arguments so it can compute its size.
                    cmd.self = cSelf;

                    //* For object creation, store the object ID the client will use for the result.
                    {% if method.return_type.category == "object" %}
                        auto* allocation = self->device->GetClient()->{{method.return_type.name.CamelCase()}}Allocator().New(self->device);
                        cmd.result = ObjectHandle{allocation->object->id, allocation->generation};
                    {% endif %}

                    {% for arg in method.arguments %}
                        cmd.{{as_varName(arg.name)}} = {{as_varName(arg.name)}};
                    {% endfor %}

                    //* Allocate space to send the command and copy the value args over.
                    device->GetClient()->SerializeCommand(cmd);

                    {% if method.return_type.category == "object" %}
                        return reinterpret_cast<{{as_cType(method.return_type.name)}}>(allocation->object.get());
                    {% endif %}
                {% else %}
                    return self->{{method.name.CamelCase()}}(
                        {%- for arg in method.arguments -%}
                            {%if not loop.first %}, {% endif %} {{as_varName(arg.name)}}
                        {%- endfor -%});
                {% endif %}
            }
        {% endfor %}

        {% if not type.name.canonical_case() == "device" %}
            //* When an object's refcount reaches 0, notify the server side of it and delete it.
            void Client{{as_MethodSuffix(type.name, Name("release"))}}({{cType}} cObj) {
                {{Type}}* obj = reinterpret_cast<{{Type}}*>(cObj);
                obj->refcount --;

                if (obj->refcount > 0) {
                    return;
                }

                DestroyObjectCmd cmd;
                cmd.objectType = ObjectType::{{type.name.CamelCase()}};
                cmd.objectId = obj->id;

                obj->device->GetClient()->SerializeCommand(cmd);
                obj->device->GetClient()->{{type.name.CamelCase()}}Allocator().Free(obj);
            }

            void Client{{as_MethodSuffix(type.name, Name("reference"))}}({{cType}} cObj) {
                {{Type}}* obj = reinterpret_cast<{{Type}}*>(cObj);
                obj->refcount ++;
            }
        {% endif %}
    {% endfor %}

    namespace {
        WGPUInstance ClientCreateInstance(WGPUInstanceDescriptor const* descriptor) {
            UNREACHABLE();
            return nullptr;
        }

        void ClientDeviceReference(WGPUDevice) {
        }

        void ClientDeviceRelease(WGPUDevice) {
        }

        struct ProcEntry {
            WGPUProc proc;
            const char* name;
        };
        static const ProcEntry sProcMap[] = {
            {% for (type, method) in c_methods_sorted_by_name %}
                { reinterpret_cast<WGPUProc>(Client{{as_MethodSuffix(type.name, method.name)}}), "{{as_cMethod(type.name, method.name)}}" },
            {% endfor %}
        };
        static constexpr size_t sProcMapSize = sizeof(sProcMap) / sizeof(sProcMap[0]);
    }  // anonymous namespace

    WGPUProc ClientGetProcAddress(WGPUDevice, const char* procName) {
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
            return reinterpret_cast<WGPUProc>(ClientGetProcAddress);
        }

        if (strcmp(procName, "wgpuCreateInstance") == 0) {
            return reinterpret_cast<WGPUProc>(ClientCreateInstance);
        }

        return nullptr;
    }

    std::vector<const char*> GetProcMapNamesForTesting() {
        std::vector<const char*> result;
        result.reserve(sProcMapSize);
        for (const ProcEntry& entry : sProcMap) {
            result.push_back(entry.name);
        }
        return result;
    }

    //* Some commands don't have a custom wire format, but need to be handled manually to update
    //* some client-side state tracking. For these we have two functions:
    //*  - An autogenerated Client{{suffix}} method that sends the command on the wire
    //*  - A manual ProxyClient{{suffix}} method that will be inserted in the proctable instead of
    //*    the autogenerated one, and that will have to call Client{{suffix}}
    DawnProcTable GetProcs() {
        DawnProcTable table;
        table.getProcAddress = ClientGetProcAddress;
        table.createInstance = ClientCreateInstance;
        {% for type in by_category["object"] %}
            {% for method in c_methods(type) %}
                {% set suffix = as_MethodSuffix(type.name, method.name) %}
                table.{{as_varName(type.name, method.name)}} = Client{{suffix}};
            {% endfor %}
        {% endfor %}
        return table;
    }
}}  // namespace dawn_wire::client
