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

#ifndef MOCK_DAWN_H
#define MOCK_DAWN_H

#include <gmock/gmock.h>
#include <dawn/dawn.h>

#include <memory>

// An abstract base class representing a proc table so that API calls can be mocked. Most API calls
// are directly represented by a delete virtual method but others need minimal state tracking to be
// useful as mocks.
class ProcTableAsClass {
    public:
        virtual ~ProcTableAsClass();

        void GetProcTableAndDevice(DawnProcTable* table, DawnDevice* device);

        // Creates an object that can be returned by a mocked call as in WillOnce(Return(foo)).
        // It returns an object of the write type that isn't equal to any previously returned object.
        // Otherwise some mock expectation could be triggered by two different objects having the same
        // value.
        {% for type in by_category["object"] %}
            {{as_cType(type.name)}} GetNew{{type.name.CamelCase()}}();
        {% endfor %}

        {% for type in by_category["object"] %}
            {% for method in type.methods if len(method.arguments) < 10 %}
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
        void DeviceSetErrorCallback(DawnDevice self,
                                    DawnDeviceErrorCallback callback,
                                    void* userdata);
        void BufferMapReadAsync(DawnBuffer self,
                                DawnBufferMapReadCallback callback,
                                void* userdata);
        void BufferMapWriteAsync(DawnBuffer self,
                                 DawnBufferMapWriteCallback callback,
                                 void* userdata);
        void FenceOnCompletion(DawnFence self,
                               uint64_t value,
                               DawnFenceOnCompletionCallback callback,
                               void* userdata);

        // Special cased mockable methods
        virtual void OnDeviceSetErrorCallback(DawnDevice device,
                                              DawnDeviceErrorCallback callback,
                                              void* userdata) = 0;
        virtual void OnBufferMapReadAsyncCallback(DawnBuffer buffer,
                                                  DawnBufferMapReadCallback callback,
                                                  void* userdata) = 0;
        virtual void OnBufferMapWriteAsyncCallback(DawnBuffer buffer,
                                                   DawnBufferMapWriteCallback callback,
                                                   void* userdata) = 0;
        virtual void OnFenceOnCompletionCallback(DawnFence fence,
                                                 uint64_t value,
                                                 DawnFenceOnCompletionCallback callback,
                                                 void* userdata) = 0;

        // Calls the stored callbacks
        void CallDeviceErrorCallback(DawnDevice device, const char* message);
        void CallMapReadCallback(DawnBuffer buffer, DawnBufferMapAsyncStatus status, const void* data, uint32_t dataLength);
        void CallMapWriteCallback(DawnBuffer buffer, DawnBufferMapAsyncStatus status, void* data, uint32_t dataLength);
        void CallFenceOnCompletionCallback(DawnFence fence, DawnFenceCompletionStatus status);

        struct Object {
            ProcTableAsClass* procs = nullptr;
            DawnDeviceErrorCallback deviceErrorCallback = nullptr;
            DawnBufferMapReadCallback mapReadCallback = nullptr;
            DawnBufferMapWriteCallback mapWriteCallback = nullptr;
            DawnFenceOnCompletionCallback fenceOnCompletionCallback = nullptr;
            void* userdata1 = 0;
            void* userdata2 = 0;
        };

    private:
        // Remembers the values returned by GetNew* so they can be freed.
        std::vector<std::unique_ptr<Object>> mObjects;
};

class MockProcTable : public ProcTableAsClass {
    public:
        MockProcTable();

        void IgnoreAllReleaseCalls();

        {% for type in by_category["object"] %}
            {% for method in type.methods if len(method.arguments) < 10 %}
                MOCK_METHOD{{len(method.arguments) + 1}}(
                    {{-as_MethodSuffix(type.name, method.name)}},
                    {{as_cType(method.return_type.name)}}(
                        {{-as_cType(type.name)}} {{as_varName(type.name)}}
                        {%- for arg in method.arguments -%}
                            , {{as_annotated_cType(arg)}}
                        {%- endfor -%}
                    ));
            {% endfor %}

            MOCK_METHOD1({{as_MethodSuffix(type.name, Name("reference"))}}, void({{as_cType(type.name)}} self));
            MOCK_METHOD1({{as_MethodSuffix(type.name, Name("release"))}}, void({{as_cType(type.name)}} self));
        {% endfor %}

        MOCK_METHOD3(OnDeviceSetErrorCallback, void(DawnDevice device, DawnDeviceErrorCallback callback, void* userdata));
        MOCK_METHOD3(OnBufferMapReadAsyncCallback, void(DawnBuffer buffer, DawnBufferMapReadCallback callback, void* userdata));
        MOCK_METHOD3(OnBufferMapWriteAsyncCallback, void(DawnBuffer buffer, DawnBufferMapWriteCallback callback, void* userdata));
        MOCK_METHOD4(OnFenceOnCompletionCallback,
                     void(DawnFence fence,
                          uint64_t value,
                          DawnFenceOnCompletionCallback callback,
                          void* userdata));
};

#endif // MOCK_DAWN_H
