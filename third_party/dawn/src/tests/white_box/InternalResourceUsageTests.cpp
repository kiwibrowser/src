// Copyright 2020 The Dawn Authors
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

#include "dawn_native/dawn_platform.h"

class InternalResourceUsageTests : public DawnTest {};

// Verify it is an error to create a buffer with a buffer usage that should only be used
// internally.
TEST_P(InternalResourceUsageTests, InternalBufferUsage) {
    DAWN_SKIP_TEST_IF(IsDawnValidationSkipped());

    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = dawn_native::kReadOnlyStorageBuffer;

    ASSERT_DEVICE_ERROR(device.CreateBuffer(&descriptor));
}

// Verify it is an error to create a texture with a texture usage that should only be used
// internally.
TEST_P(InternalResourceUsageTests, InternalTextureUsage) {
    DAWN_SKIP_TEST_IF(IsDawnValidationSkipped());

    wgpu::TextureDescriptor descriptor;
    descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    descriptor.size = {1, 1, 1};
    descriptor.usage = dawn_native::kReadonlyStorageTexture;
    ASSERT_DEVICE_ERROR(device.CreateTexture(&descriptor));
}

DAWN_INSTANTIATE_TEST(InternalResourceUsageTests, NullBackend());
