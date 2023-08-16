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

#include "tests/DawnTest.h"

#include "utils/WGPUHelpers.h"

class BasicTests : public DawnTest {};

// Test adapter filter by vendor id.
TEST_P(BasicTests, VendorIdFilter) {
    DAWN_SKIP_TEST_IF(!HasVendorIdFilter());

    ASSERT_EQ(GetAdapterProperties().vendorID, GetVendorIdFilter());
}

// Test Queue::WriteBuffer changes the content of the buffer, but really this is the most
// basic test possible, and tests the test harness
TEST_P(BasicTests, QueueWriteBuffer) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint32_t value = 0x01020304;
    queue.WriteBuffer(buffer, 0, &value, sizeof(value));

    EXPECT_BUFFER_U32_EQ(value, buffer, 0);
}

// Test a validation error for Queue::WriteBuffer but really this is the most basic test possible
// for ASSERT_DEVICE_ERROR
TEST_P(BasicTests, QueueWriteBufferError) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    uint8_t value = 187;
    ASSERT_DEVICE_ERROR(queue.WriteBuffer(buffer, 1000, &value, sizeof(value)));
}

DAWN_INSTANTIATE_TEST(BasicTests, D3D12Backend(), MetalBackend(), OpenGLBackend(), VulkanBackend());
