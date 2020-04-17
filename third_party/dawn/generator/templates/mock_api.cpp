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

#include "mock_dawn.h"

using namespace testing;

namespace {
    {% for type in by_category["object"] %}
        {% for method in native_methods(type) if len(method.arguments) < 10 %}
            {{as_cType(method.return_type.name)}} Forward{{as_MethodSuffix(type.name, method.name)}}(
                {{-as_cType(type.name)}} self
                {%- for arg in method.arguments -%}
                    , {{as_annotated_cType(arg)}}
                {%- endfor -%}
            ) {
                auto object = reinterpret_cast<ProcTableAsClass::Object*>(self);
                return object->procs->{{as_MethodSuffix(type.name, method.name)}}(self
                    {%- for arg in method.arguments -%}
                        , {{as_varName(arg.name)}}
                    {%- endfor -%}
                );
            }
        {% endfor %}

    {% endfor %}
}

ProcTableAsClass::~ProcTableAsClass() {
}

void ProcTableAsClass::GetProcTableAndDevice(DawnProcTable* table, DawnDevice* device) {
    *device = GetNewDevice();

    {% for type in by_category["object"] %}
        {% for method in native_methods(type) if len(method.arguments) < 10 %}
            table->{{as_varName(type.name, method.name)}} = reinterpret_cast<{{as_cProc(type.name, method.name)}}>(Forward{{as_MethodSuffix(type.name, method.name)}});
        {% endfor %}
    {% endfor %}
}

void ProcTableAsClass::DeviceSetErrorCallback(DawnDevice self,
                                              DawnDeviceErrorCallback callback,
                                              void* userdata) {
    auto object = reinterpret_cast<ProcTableAsClass::Object*>(self);
    object->deviceErrorCallback = callback;
    object->userdata1 = userdata;

    OnDeviceSetErrorCallback(self, callback, userdata);
}

void ProcTableAsClass::BufferMapReadAsync(DawnBuffer self,
                                          DawnBufferMapReadCallback callback,
                                          void* userdata) {
    auto object = reinterpret_cast<ProcTableAsClass::Object*>(self);
    object->mapReadCallback = callback;
    object->userdata1 = userdata;

    OnBufferMapReadAsyncCallback(self, callback, userdata);
}

void ProcTableAsClass::BufferMapWriteAsync(DawnBuffer self,
                                           DawnBufferMapWriteCallback callback,
                                           void* userdata) {
    auto object = reinterpret_cast<ProcTableAsClass::Object*>(self);
    object->mapWriteCallback = callback;
    object->userdata1 = userdata;

    OnBufferMapWriteAsyncCallback(self, callback, userdata);
}

void ProcTableAsClass::FenceOnCompletion(DawnFence self,
                                         uint64_t value,
                                         DawnFenceOnCompletionCallback callback,
                                         void* userdata) {
    auto object = reinterpret_cast<ProcTableAsClass::Object*>(self);
    object->fenceOnCompletionCallback = callback;
    object->userdata1 = userdata;

    OnFenceOnCompletionCallback(self, value, callback, userdata);
}

void ProcTableAsClass::CallDeviceErrorCallback(DawnDevice device, const char* message) {
    auto object = reinterpret_cast<ProcTableAsClass::Object*>(device);
    object->deviceErrorCallback(message, object->userdata1);
}
void ProcTableAsClass::CallMapReadCallback(DawnBuffer buffer, DawnBufferMapAsyncStatus status, const void* data, uint32_t dataLength) {
    auto object = reinterpret_cast<ProcTableAsClass::Object*>(buffer);
    object->mapReadCallback(status, data, dataLength, object->userdata1);
}

void ProcTableAsClass::CallMapWriteCallback(DawnBuffer buffer, DawnBufferMapAsyncStatus status, void* data, uint32_t dataLength) {
    auto object = reinterpret_cast<ProcTableAsClass::Object*>(buffer);
    object->mapWriteCallback(status, data, dataLength, object->userdata1);
}

void ProcTableAsClass::CallFenceOnCompletionCallback(DawnFence fence,
                                                     DawnFenceCompletionStatus status) {
    auto object = reinterpret_cast<ProcTableAsClass::Object*>(fence);
    object->fenceOnCompletionCallback(status, object->userdata1);
}

{% for type in by_category["object"] %}
    {{as_cType(type.name)}} ProcTableAsClass::GetNew{{type.name.CamelCase()}}() {
        mObjects.emplace_back(new Object);
        mObjects.back()->procs = this;
        return reinterpret_cast<{{as_cType(type.name)}}>(mObjects.back().get());
    }
{% endfor %}

MockProcTable::MockProcTable() {
}

void MockProcTable::IgnoreAllReleaseCalls() {
    {% for type in by_category["object"] %}
        EXPECT_CALL(*this, {{as_MethodSuffix(type.name, Name("release"))}}(_)).Times(AnyNumber());
    {% endfor %}
}
