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

#include <vector>

class NonzeroBufferCreationTests : public DawnTest {};

// Verify that each byte of the buffer has all been initialized to 1 with the toggle enabled when it
// is created with CopyDst usage.
TEST_P(NonzeroBufferCreationTests, BufferCreationWithCopyDstUsage) {
    constexpr uint32_t kSize = 32u;

    wgpu::BufferDescriptor descriptor;
    descriptor.size = kSize;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;

    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    std::vector<uint8_t> expectedData(kSize, static_cast<uint8_t>(1u));
    EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<uint32_t*>(expectedData.data()), buffer, 0,
                               kSize / sizeof(uint32_t));
}

// Verify that each byte of the buffer has all been initialized to 1 with the toggle enabled when it
// is created with MapWrite without CopyDst usage.
TEST_P(NonzeroBufferCreationTests, BufferCreationWithMapWriteWithoutCopyDstUsage) {
    constexpr uint32_t kSize = 32u;

    wgpu::BufferDescriptor descriptor;
    descriptor.size = kSize;
    descriptor.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;

    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    std::vector<uint8_t> expectedData(kSize, static_cast<uint8_t>(1u));
    EXPECT_BUFFER_U32_RANGE_EQ(reinterpret_cast<uint32_t*>(expectedData.data()), buffer, 0,
                               kSize / sizeof(uint32_t));
}

DAWN_INSTANTIATE_TEST(NonzeroBufferCreationTests,
                      D3D12Backend({"nonzero_clear_resources_on_creation_for_testing"}),
                      MetalBackend({"nonzero_clear_resources_on_creation_for_testing"}),
                      OpenGLBackend({"nonzero_clear_resources_on_creation_for_testing"}),
                      VulkanBackend({"nonzero_clear_resources_on_creation_for_testing"}));
