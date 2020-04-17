// Copyright 2017 The Dawn Authors
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

#include <gtest/gtest.h>

#include "dawn_native/RefCounted.h"
#include "dawn_native/ToBackend.h"

#include <type_traits>

// Make our own Base - Backend object pair, reusing the CommandBuffer name
namespace dawn_native {
    class CommandBufferBase : public RefCounted {
    };
}

using namespace dawn_native;

class MyCommandBuffer : public CommandBufferBase {
};

struct MyBackendTraits {
    using CommandBufferType = MyCommandBuffer;
};

// Instanciate ToBackend for our "backend"
template<typename T>
auto ToBackend(T&& common) -> decltype(ToBackendBase<MyBackendTraits>(common)) {
    return ToBackendBase<MyBackendTraits>(common);
}

// Test that ToBackend correctly converts pointers to base classes.
TEST(ToBackend, Pointers) {
    {
        MyCommandBuffer* cmdBuf = new MyCommandBuffer;
        const CommandBufferBase* base = cmdBuf;

        auto backendCmdBuf = ToBackend(base);
        static_assert(std::is_same<decltype(backendCmdBuf), const MyCommandBuffer*>::value, "");
        ASSERT_EQ(cmdBuf, backendCmdBuf);

        cmdBuf->Release();
    }
    {
        MyCommandBuffer* cmdBuf = new MyCommandBuffer;
        CommandBufferBase* base = cmdBuf;

        auto backendCmdBuf = ToBackend(base);
        static_assert(std::is_same<decltype(backendCmdBuf), MyCommandBuffer*>::value, "");
        ASSERT_EQ(cmdBuf, backendCmdBuf);

        cmdBuf->Release();
    }
}

// Test that ToBackend correctly converts Refs to base classes.
TEST(ToBackend, Ref) {
    {
        MyCommandBuffer* cmdBuf = new MyCommandBuffer;
        const Ref<CommandBufferBase> base(cmdBuf);

        const auto& backendCmdBuf = ToBackend(base);
        static_assert(std::is_same<decltype(ToBackend(base)), const Ref<MyCommandBuffer>&>::value, "");
        ASSERT_EQ(cmdBuf, backendCmdBuf.Get());

        cmdBuf->Release();
    }
    {
        MyCommandBuffer* cmdBuf = new MyCommandBuffer;
        Ref<CommandBufferBase> base(cmdBuf);

        auto backendCmdBuf = ToBackend(base);
        static_assert(std::is_same<decltype(ToBackend(base)), Ref<MyCommandBuffer>&>::value, "");
        ASSERT_EQ(cmdBuf, backendCmdBuf.Get());

        cmdBuf->Release();
    }
}
