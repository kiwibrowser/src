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

#ifndef MOCK_WEBGPU_H
#define MOCK_WEBGPU_H

#include <dawn/dawn_proc_table.h>
#include <dawn/webgpu.h>
#include <gmock/gmock.h>

#include <memory>

// An abstract base class representing a proc table so that API calls can be mocked. Most API calls
// are directly represented by a delete virtual method but others need minimal state tracking to be
// useful as mocks.
class ProcTableAsClass {
    public:
        virtual ~ProcTableAsClass();

        void GetProcTableAndDevice(DawnProcTable* table, WGPUDevice* device);

        // Creates an object that can be returned by a mocked call as in WillOnce(Return(foo)).
        // It returns an object of the write type that isn't equal to any previously returned object.
        // Otherwise some mock expectation could be triggered by two different objects having the same
        // value.
        {% for type in by_category["object"] %}
            {{as_cType(type.name)}} GetNew{{type.name.CamelCase()}}();
        {% endfor %}

        {% for type in by_category["object"] %}
            {% for method in type.methods if len(method.arguments) < 10 and not has_callback_arguments(method) %}
                virtual {{as_cType(method.return_type.name)}} {{as_MethodSuffix(type.name, method.name)}}(
                    {{-as_cType(type.name)}} {{as_varName(type.name)}}
                    {%- for arg in method.arguments -%}
                        , {{as_annotated_cType(arg)}}
                    {%- endfor -%}
                ) = 0;
            {% endfor %}
            virtual void {{as_MethodSuffix(type.name, Name("reference"))}}({{as_cType(type.name)}} self) = 0;
            virtual void {{as_MethodSuffix(type.name, Name("release"))}}({{as_cType(type.name)}} self) = 0;
        {% endfor %}

        // Stores callback and userdata and calls the On* methods
        void DeviceSetUncapturedErrorCallback(WGPUDevice self,
                                    WGPUErrorCallback callback,
                                    void* userdata);
        void DeviceSetDeviceLostCallback(WGPUDevice self,
                                         WGPUDeviceLostCallback callback,
                                         void* userdata);
        bool DevicePopErrorScope(WGPUDevice self, WGPUErrorCallback callback, void* userdata);
        void BufferMapReadAsync(WGPUBuffer self,
                                WGPUBufferMapReadCallback callback,
                                void* userdata);
        void BufferMapWriteAsync(WGPUBuffer self,
                                 WGPUBufferMapWriteCallback callback,
                                 void* userdata);
        void BufferMapAsync(WGPUBuffer self,
                            WGPUMapModeFlags mode,
                            size_t offset,
                            size_t size,
                            WGPUBufferMapCallback callback,
                            void* userdata);
        void FenceOnCompletion(WGPUFence self,
                               uint64_t value,
                               WGPUFenceOnCompletionCallback callback,
                               void* userdata);

        // Special cased mockable methods
        virtual void OnDeviceSetUncapturedErrorCallback(WGPUDevice device,
                                              WGPUErrorCallback callback,
                                              void* userdata) = 0;
        virtual void OnDeviceSetDeviceLostCallback(WGPUDevice device,
                                                   WGPUDeviceLostCallback callback,
                                                   void* userdata) = 0;
        virtual bool OnDevicePopErrorScopeCallback(WGPUDevice device,
                                              WGPUErrorCallback callback,
                                              void* userdata) = 0;
        virtual void OnBufferMapReadAsyncCallback(WGPUBuffer buffer,
                                                  WGPUBufferMapReadCallback callback,
                                                  void* userdata) = 0;
        virtual void OnBufferMapWriteAsyncCallback(WGPUBuffer buffer,
                                                   WGPUBufferMapWriteCallback callback,
                                                   void* userdata) = 0;
        virtual void OnBufferMapAsyncCallback(WGPUBuffer buffer,
                                              WGPUBufferMapCallback callback,
                                              void* userdata) = 0;
        virtual void OnFenceOnCompletionCallback(WGPUFence fence,
                                                 uint64_t value,
                                                 WGPUFenceOnCompletionCallback callback,
                                                 void* userdata) = 0;

        // Calls the stored callbacks
        void CallDeviceErrorCallback(WGPUDevice device, WGPUErrorType type, const char* message);
        void CallDeviceLostCallback(WGPUDevice device, const char* message);
        void CallMapReadCallback(WGPUBuffer buffer, WGPUBufferMapAsyncStatus status, const void* data, uint64_t dataLength);
        void CallMapWriteCallback(WGPUBuffer buffer, WGPUBufferMapAsyncStatus status, void* data, uint64_t dataLength);
        void CallMapAsyncCallback(WGPUBuffer buffer, WGPUBufferMapAsyncStatus status);
        void CallFenceOnCompletionCallback(WGPUFence fence, WGPUFenceCompletionStatus status);

        struct Object {
            ProcTableAsClass* procs = nullptr;
            WGPUErrorCallback deviceErrorCallback = nullptr;
            WGPUDeviceLostCallback deviceLostCallback = nullptr;
            WGPUBufferMapReadCallback mapReadCallback = nullptr;
            WGPUBufferMapWriteCallback mapWriteCallback = nullptr;
            WGPUBufferMapCallback mapAsyncCallback = nullptr;
            WGPUFenceOnCompletionCallback fenceOnCompletionCallback = nullptr;
            void* userdata = 0;
        };

    private:
        // Remembers the values returned by GetNew* so they can be freed.
        std::vector<std::unique_ptr<Object>> mObjects;
};

class MockProcTable : public ProcTableAsClass {
    public:
        MockProcTable();
        ~MockProcTable() override;

        void IgnoreAllReleaseCalls();

        {% for type in by_category["object"] %}
            {% for method in type.methods if len(method.arguments) < 10 and not has_callback_arguments(method) %}
                MOCK_METHOD({{as_cType(method.return_type.name)}},{{" "}}
                    {{-as_MethodSuffix(type.name, method.name)}}, (
                        {{-as_cType(type.name)}} {{as_varName(type.name)}}
                        {%- for arg in method.arguments -%}
                            , {{as_annotated_cType(arg)}}
                        {%- endfor -%}
                    ), (override));
            {% endfor %}

            MOCK_METHOD(void, {{as_MethodSuffix(type.name, Name("reference"))}}, ({{as_cType(type.name)}} self), (override));
            MOCK_METHOD(void, {{as_MethodSuffix(type.name, Name("release"))}}, ({{as_cType(type.name)}} self), (override));
        {% endfor %}

        MOCK_METHOD(void, OnDeviceSetUncapturedErrorCallback, (WGPUDevice device, WGPUErrorCallback callback, void* userdata), (override));
        MOCK_METHOD(void, OnDeviceSetDeviceLostCallback, (WGPUDevice device, WGPUDeviceLostCallback callback, void* userdata), (override));
        MOCK_METHOD(bool, OnDevicePopErrorScopeCallback, (WGPUDevice device, WGPUErrorCallback callback, void* userdata), (override));
        MOCK_METHOD(void, OnBufferMapReadAsyncCallback, (WGPUBuffer buffer, WGPUBufferMapReadCallback callback, void* userdata), (override));
        MOCK_METHOD(void, OnBufferMapWriteAsyncCallback, (WGPUBuffer buffer, WGPUBufferMapWriteCallback callback, void* userdata), (override));
        MOCK_METHOD(void,
                    OnBufferMapAsyncCallback,
                    (WGPUBuffer buffer, WGPUBufferMapCallback callback, void* userdata),
                    (override));
        MOCK_METHOD(void, OnFenceOnCompletionCallback, (WGPUFence fence, uint64_t value, WGPUFenceOnCompletionCallback callback, void* userdata), (override));
};

#endif  // MOCK_WEBGPU_H
