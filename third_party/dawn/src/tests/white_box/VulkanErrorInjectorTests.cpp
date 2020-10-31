// Copyright 2019 The Dawn Authors
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

#include "tests/DawnTest.h"

#include "common/Math.h"
#include "common/vulkan_platform.h"
#include "dawn_native/ErrorData.h"
#include "dawn_native/VulkanBackend.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace {

    class VulkanErrorInjectorTests : public DawnTest {
      public:
        void SetUp() override {
            DawnTest::SetUp();
            DAWN_SKIP_TEST_IF(UsesWire());

            mDeviceVk = reinterpret_cast<dawn_native::vulkan::Device*>(device.Get());
        }

      protected:
        dawn_native::vulkan::Device* mDeviceVk;
    };

}  // anonymous namespace

TEST_P(VulkanErrorInjectorTests, InjectErrorOnCreateBuffer) {
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.size = 16;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    // Check that making a buffer works.
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        EXPECT_EQ(
            mDeviceVk->fn.CreateBuffer(mDeviceVk->GetVkDevice(), &createInfo, nullptr, &buffer),
            VK_SUCCESS);
        mDeviceVk->fn.DestroyBuffer(mDeviceVk->GetVkDevice(), buffer, nullptr);
    }

    auto CreateTestBuffer = [&]() -> bool {
        VkBuffer buffer = VK_NULL_HANDLE;
        dawn_native::MaybeError err = CheckVkSuccess(
            mDeviceVk->fn.CreateBuffer(mDeviceVk->GetVkDevice(), &createInfo, nullptr, &buffer),
            "vkCreateBuffer");
        if (err.IsError()) {
            // The handle should never be written to, even for mock failures.
            EXPECT_EQ(buffer, VK_NULL_HANDLE);
            err.AcquireError();
            return false;
        }
        EXPECT_NE(buffer, VK_NULL_HANDLE);

        // We never use the buffer, only test mocking errors on creation. Cleanup now.
        mDeviceVk->fn.DestroyBuffer(mDeviceVk->GetVkDevice(), buffer, nullptr);

        return true;
    };

    // Check that making a buffer inside CheckVkSuccess works.
    {
        EXPECT_TRUE(CreateTestBuffer());

        // The error injector call count should be empty
        EXPECT_EQ(dawn_native::AcquireErrorInjectorCallCount(), 0u);
    }

    // Test error injection works.
    dawn_native::EnableErrorInjector();
    {
        EXPECT_TRUE(CreateTestBuffer());
        EXPECT_TRUE(CreateTestBuffer());

        // The error injector call count should be two.
        EXPECT_EQ(dawn_native::AcquireErrorInjectorCallCount(), 2u);

        // Inject an error at index 0. The first should fail, the second succeed.
        {
            dawn_native::InjectErrorAt(0u);
            EXPECT_FALSE(CreateTestBuffer());
            EXPECT_TRUE(CreateTestBuffer());

            dawn_native::ClearErrorInjector();
        }

        // Inject an error at index 1. The second should fail, the first succeed.
        {
            dawn_native::InjectErrorAt(1u);
            EXPECT_TRUE(CreateTestBuffer());
            EXPECT_FALSE(CreateTestBuffer());

            dawn_native::ClearErrorInjector();
        }

        // Inject an error and then clear the injector. Calls should be successful.
        {
            dawn_native::InjectErrorAt(0u);
            dawn_native::DisableErrorInjector();

            EXPECT_TRUE(CreateTestBuffer());
            EXPECT_TRUE(CreateTestBuffer());

            dawn_native::ClearErrorInjector();
        }
    }
}

DAWN_INSTANTIATE_TEST(VulkanErrorInjectorTests, VulkanBackend());
