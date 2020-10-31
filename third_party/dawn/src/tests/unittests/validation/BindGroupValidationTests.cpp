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

#include "tests/unittests/validation/ValidationTest.h"

#include "common/Assert.h"
#include "common/Constants.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

class BindGroupValidationTest : public ValidationTest {
  public:
    wgpu::Texture CreateTexture(wgpu::TextureUsage usage,
                                wgpu::TextureFormat format,
                                uint32_t layerCount) {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size = {16, 16, layerCount};
        descriptor.sampleCount = 1;
        descriptor.mipLevelCount = 1;
        descriptor.usage = usage;
        descriptor.format = format;

        return device.CreateTexture(&descriptor);
    }

    void SetUp() override {
        // Create objects to use as resources inside test bind groups.
        {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = 1024;
            descriptor.usage = wgpu::BufferUsage::Uniform;
            mUBO = device.CreateBuffer(&descriptor);
        }
        {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = 1024;
            descriptor.usage = wgpu::BufferUsage::Storage;
            mSSBO = device.CreateBuffer(&descriptor);
        }
        {
            wgpu::SamplerDescriptor descriptor = utils::GetDefaultSamplerDescriptor();
            mSampler = device.CreateSampler(&descriptor);
        }
        {
            mSampledTexture =
                CreateTexture(wgpu::TextureUsage::Sampled, wgpu::TextureFormat::RGBA8Unorm, 1);
            mSampledTextureView = mSampledTexture.CreateView();
        }
    }

  protected:
    wgpu::Buffer mUBO;
    wgpu::Buffer mSSBO;
    wgpu::Sampler mSampler;
    wgpu::Texture mSampledTexture;
    wgpu::TextureView mSampledTextureView;
};

// Test the validation of BindGroupDescriptor::nextInChain
TEST_F(BindGroupValidationTest, NextInChainNullptr) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {});

    wgpu::BindGroupDescriptor descriptor;
    descriptor.layout = layout;
    descriptor.entryCount = 0;
    descriptor.entries = nullptr;

    // Control case: check that nextInChain = nullptr is valid
    descriptor.nextInChain = nullptr;
    device.CreateBindGroup(&descriptor);

    // Check that nextInChain != nullptr is an error.
    wgpu::ChainedStruct chainedDescriptor;
    descriptor.nextInChain = &chainedDescriptor;
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
}

// Check constraints on entryCount
TEST_F(BindGroupValidationTest, EntryCountMismatch) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler}});

    // Control case: check that a descriptor with one binding is ok
    utils::MakeBindGroup(device, layout, {{0, mSampler}});

    // Check that entryCount != layout.entryCount fails.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {}));
}

// Check constraints on BindGroupEntry::binding
TEST_F(BindGroupValidationTest, WrongBindings) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler}});

    // Control case: check that a descriptor with a binding matching the layout's is ok
    utils::MakeBindGroup(device, layout, {{0, mSampler}});

    // Check that binding must be present in the layout
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{1, mSampler}}));
}

// Check that the same binding cannot be set twice
TEST_F(BindGroupValidationTest, BindingSetTwice) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler},
                 {1, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler}});

    // Control case: check that different bindings work
    utils::MakeBindGroup(device, layout, {{0, mSampler}, {1, mSampler}});

    // Check that setting the same binding twice is invalid
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mSampler}, {0, mSampler}}));
}

// Check that a sampler binding must contain exactly one sampler
TEST_F(BindGroupValidationTest, SamplerBindingType) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler}});

    wgpu::BindGroupEntry binding;
    binding.binding = 0;
    binding.sampler = nullptr;
    binding.textureView = nullptr;
    binding.buffer = nullptr;
    binding.offset = 0;
    binding.size = 0;

    wgpu::BindGroupDescriptor descriptor;
    descriptor.layout = layout;
    descriptor.entryCount = 1;
    descriptor.entries = &binding;

    // Not setting anything fails
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));

    // Control case: setting just the sampler works
    binding.sampler = mSampler;
    device.CreateBindGroup(&descriptor);

    // Setting the texture view as well is an error
    binding.textureView = mSampledTextureView;
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
    binding.textureView = nullptr;

    // Setting the buffer as well is an error
    binding.buffer = mUBO;
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
    binding.buffer = nullptr;

    // Setting the sampler to an error sampler is an error.
    {
        wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
        samplerDesc.minFilter = static_cast<wgpu::FilterMode>(0xFFFFFFFF);

        wgpu::Sampler errorSampler;
        ASSERT_DEVICE_ERROR(errorSampler = device.CreateSampler(&samplerDesc));

        binding.sampler = errorSampler;
        ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
        binding.sampler = nullptr;
    }
}

// Check that a texture binding must contain exactly a texture view
TEST_F(BindGroupValidationTest, TextureBindingType) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture}});

    wgpu::BindGroupEntry binding;
    binding.binding = 0;
    binding.sampler = nullptr;
    binding.textureView = nullptr;
    binding.buffer = nullptr;
    binding.offset = 0;
    binding.size = 0;

    wgpu::BindGroupDescriptor descriptor;
    descriptor.layout = layout;
    descriptor.entryCount = 1;
    descriptor.entries = &binding;

    // Not setting anything fails
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));

    // Control case: setting just the texture view works
    binding.textureView = mSampledTextureView;
    device.CreateBindGroup(&descriptor);

    // Setting the sampler as well is an error
    binding.sampler = mSampler;
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
    binding.textureView = nullptr;

    // Setting the buffer as well is an error
    binding.buffer = mUBO;
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
    binding.buffer = nullptr;

    // Setting the texture view to an error texture view is an error.
    {
        wgpu::TextureViewDescriptor viewDesc;
        viewDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        viewDesc.dimension = wgpu::TextureViewDimension::e2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 0;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1000;

        wgpu::TextureView errorView;
        ASSERT_DEVICE_ERROR(errorView = mSampledTexture.CreateView(&viewDesc));

        binding.textureView = errorView;
        ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
        binding.textureView = nullptr;
    }
}

// Check that a buffer binding must contain exactly a buffer
TEST_F(BindGroupValidationTest, BufferBindingType) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer}});

    wgpu::BindGroupEntry binding;
    binding.binding = 0;
    binding.sampler = nullptr;
    binding.textureView = nullptr;
    binding.buffer = nullptr;
    binding.offset = 0;
    binding.size = 1024;

    wgpu::BindGroupDescriptor descriptor;
    descriptor.layout = layout;
    descriptor.entryCount = 1;
    descriptor.entries = &binding;

    // Not setting anything fails
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));

    // Control case: setting just the buffer works
    binding.buffer = mUBO;
    device.CreateBindGroup(&descriptor);

    // Setting the texture view as well is an error
    binding.textureView = mSampledTextureView;
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
    binding.textureView = nullptr;

    // Setting the sampler as well is an error
    binding.sampler = mSampler;
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
    binding.sampler = nullptr;

    // Setting the buffer to an error buffer is an error.
    {
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = 1024;
        bufferDesc.usage = static_cast<wgpu::BufferUsage>(0xFFFFFFFF);

        wgpu::Buffer errorBuffer;
        ASSERT_DEVICE_ERROR(errorBuffer = device.CreateBuffer(&bufferDesc));

        binding.buffer = errorBuffer;
        ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
        binding.buffer = nullptr;
    }
}

// Check that a texture must have the correct usage
TEST_F(BindGroupValidationTest, TextureUsage) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture}});

    // Control case: setting a sampleable texture view works.
    utils::MakeBindGroup(device, layout, {{0, mSampledTextureView}});

    // Make an output attachment texture and try to set it for a SampledTexture binding
    wgpu::Texture outputTexture =
        CreateTexture(wgpu::TextureUsage::OutputAttachment, wgpu::TextureFormat::RGBA8Unorm, 1);
    wgpu::TextureView outputTextureView = outputTexture.CreateView();
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, outputTextureView}}));
}

// Check that a texture must have the correct component type
TEST_F(BindGroupValidationTest, TextureComponentType) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false, 0,
                  false, wgpu::TextureViewDimension::e2D, wgpu::TextureComponentType::Float}});

    // Control case: setting a Float typed texture view works.
    utils::MakeBindGroup(device, layout, {{0, mSampledTextureView}});

    // Make a Uint component typed texture and try to set it to a Float component binding.
    wgpu::Texture uintTexture =
        CreateTexture(wgpu::TextureUsage::Sampled, wgpu::TextureFormat::RGBA8Uint, 1);
    wgpu::TextureView uintTextureView = uintTexture.CreateView();

    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, uintTextureView}}));
}

// Check that a texture must have the correct dimension
TEST_F(BindGroupValidationTest, TextureDimension) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false, 0,
                  false, wgpu::TextureViewDimension::e2D, wgpu::TextureComponentType::Float}});

    // Control case: setting a 2D texture view works.
    utils::MakeBindGroup(device, layout, {{0, mSampledTextureView}});

    // Make a 2DArray texture and try to set it to a 2D binding.
    wgpu::Texture arrayTexture =
        CreateTexture(wgpu::TextureUsage::Sampled, wgpu::TextureFormat::RGBA8Uint, 2);
    wgpu::TextureView arrayTextureView = arrayTexture.CreateView();

    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, arrayTextureView}}));
}

// Check that a UBO must have the correct usage
TEST_F(BindGroupValidationTest, BufferUsageUBO) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer}});

    // Control case: using a buffer with the uniform usage works
    utils::MakeBindGroup(device, layout, {{0, mUBO, 0, 256}});

    // Using a buffer without the uniform usage fails
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mSSBO, 0, 256}}));
}

// Check that a SSBO must have the correct usage
TEST_F(BindGroupValidationTest, BufferUsageSSBO) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});

    // Control case: using a buffer with the storage usage works
    utils::MakeBindGroup(device, layout, {{0, mSSBO, 0, 256}});

    // Using a buffer without the storage usage fails
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mUBO, 0, 256}}));
}

// Check that a readonly SSBO must have the correct usage
TEST_F(BindGroupValidationTest, BufferUsageReadonlySSBO) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer}});

    // Control case: using a buffer with the storage usage works
    utils::MakeBindGroup(device, layout, {{0, mSSBO, 0, 256}});

    // Using a buffer without the storage usage fails
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mUBO, 0, 256}}));
}

// Tests constraints on the buffer offset for bind groups.
TEST_F(BindGroupValidationTest, BufferOffsetAlignment) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                });

    // Check that offset 0 is valid
    utils::MakeBindGroup(device, layout, {{0, mUBO, 0, 512}});

    // Check that offset 256 (aligned) is valid
    utils::MakeBindGroup(device, layout, {{0, mUBO, 256, 256}});

    // Check cases where unaligned buffer offset is invalid
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mUBO, 1, 256}}));
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mUBO, 128, 256}}));
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mUBO, 255, 256}}));
}

// Tests constraints to be sure the buffer binding fits in the buffer
TEST_F(BindGroupValidationTest, BufferBindingOOB) {
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                });

    wgpu::BufferDescriptor descriptor;
    descriptor.size = 1024;
    descriptor.usage = wgpu::BufferUsage::Uniform;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    // Success case, touching the start of the buffer works
    utils::MakeBindGroup(device, layout, {{0, buffer, 0, 256}});

    // Success case, touching the end of the buffer works
    utils::MakeBindGroup(device, layout, {{0, buffer, 3 * 256, 256}});

    // Error case, zero size is invalid.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, buffer, 1024, 0}}));

    // Success case, touching the full buffer works
    utils::MakeBindGroup(device, layout, {{0, buffer, 0, 1024}});
    utils::MakeBindGroup(device, layout, {{0, buffer, 0, wgpu::kWholeSize}});

    // Success case, whole size causes the rest of the buffer to be used but not beyond.
    utils::MakeBindGroup(device, layout, {{0, buffer, 256, wgpu::kWholeSize}});

    // Error case, offset is OOB
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, buffer, 256 * 5, 0}}));

    // Error case, size is OOB
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, buffer, 0, 256 * 5}}));

    // Error case, offset+size is OOB
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, buffer, 1024, 256}}));

    // Error case, offset+size overflows to be 0
    ASSERT_DEVICE_ERROR(
        utils::MakeBindGroup(device, layout, {{0, buffer, 256, uint32_t(0) - uint32_t(256)}}));
}

// Tests constraints to be sure the uniform buffer binding isn't too large
TEST_F(BindGroupValidationTest, MaxUniformBufferBindingSize) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 2 * kMaxUniformBufferBindingSize;
    descriptor.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::Storage;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    wgpu::BindGroupLayout uniformLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer}});

    // Success case, this is exactly the limit
    utils::MakeBindGroup(device, uniformLayout, {{0, buffer, 0, kMaxUniformBufferBindingSize}});

    wgpu::BindGroupLayout doubleUniformLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                 {1, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer}});

    // Success case, individual bindings don't exceed the limit
    utils::MakeBindGroup(device, doubleUniformLayout,
                         {{0, buffer, 0, kMaxUniformBufferBindingSize},
                          {1, buffer, kMaxUniformBufferBindingSize, kMaxUniformBufferBindingSize}});

    // Error case, this is above the limit
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, uniformLayout,
                                             {{0, buffer, 0, kMaxUniformBufferBindingSize + 1}}));

    // Making sure the constraint doesn't apply to storage buffers
    wgpu::BindGroupLayout readonlyStorageLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer}});
    wgpu::BindGroupLayout storageLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});

    // Success case, storage buffer can still be created.
    utils::MakeBindGroup(device, readonlyStorageLayout,
                         {{0, buffer, 0, 2 * kMaxUniformBufferBindingSize}});
    utils::MakeBindGroup(device, storageLayout, {{0, buffer, 0, 2 * kMaxUniformBufferBindingSize}});
}

// Test what happens when the layout is an error.
TEST_F(BindGroupValidationTest, ErrorLayout) {
    wgpu::BindGroupLayout goodLayout = utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                });

    wgpu::BindGroupLayout errorLayout;
    ASSERT_DEVICE_ERROR(
        errorLayout = utils::MakeBindGroupLayout(
            device, {
                        {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                        {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                    }));

    // Control case, creating with the good layout works
    utils::MakeBindGroup(device, goodLayout, {{0, mUBO, 0, 256}});

    // Creating with an error layout fails
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, errorLayout, {{0, mUBO, 0, 256}}));
}

class BindGroupLayoutValidationTest : public ValidationTest {
  public:
    wgpu::BindGroupLayout MakeBindGroupLayout(wgpu::BindGroupLayoutEntry* binding, uint32_t count) {
        wgpu::BindGroupLayoutDescriptor descriptor;
        descriptor.entryCount = count;
        descriptor.entries = binding;
        return device.CreateBindGroupLayout(&descriptor);
    }

    void TestCreateBindGroupLayout(wgpu::BindGroupLayoutEntry* binding,
                                   uint32_t count,
                                   bool expected) {
        wgpu::BindGroupLayoutDescriptor descriptor;

        descriptor.entryCount = count;
        descriptor.entries = binding;

        if (!expected) {
            ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&descriptor));
        } else {
            device.CreateBindGroupLayout(&descriptor);
        }
    }

    void TestCreatePipelineLayout(wgpu::BindGroupLayout* bgl, uint32_t count, bool expected) {
        wgpu::PipelineLayoutDescriptor descriptor;

        descriptor.bindGroupLayoutCount = count;
        descriptor.bindGroupLayouts = bgl;

        if (!expected) {
            ASSERT_DEVICE_ERROR(device.CreatePipelineLayout(&descriptor));
        } else {
            device.CreatePipelineLayout(&descriptor);
        }
    }
};

// Tests setting storage buffer and readonly storage buffer bindings in vertex and fragment shader.
TEST_F(BindGroupLayoutValidationTest, BindGroupLayoutStorageBindingsInVertexShader) {
    // Checks that storage buffer binding is not supported in vertex shader.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::StorageBuffer}}));

    utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::ReadonlyStorageBuffer}});

    utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});

    utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer}});
}

// Tests setting that bind group layout bindings numbers may be very large.
TEST_F(BindGroupLayoutValidationTest, BindGroupLayoutEntryNumberLarge) {
    // Checks that uint32_t max is valid.
    utils::MakeBindGroupLayout(device,
                               {{std::numeric_limits<uint32_t>::max(), wgpu::ShaderStage::Vertex,
                                 wgpu::BindingType::UniformBuffer}});
}

// This test verifies that the BindGroupLayout bindings are correctly validated, even if the
// binding ids are out-of-order.
TEST_F(BindGroupLayoutValidationTest, BindGroupEntry) {
    utils::MakeBindGroupLayout(device,
                               {
                                   {1, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                                   {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                               });
}

// Check that dynamic = true is only allowed with buffer bindings.
TEST_F(BindGroupLayoutValidationTest, DynamicAndTypeCompatibility) {
    utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer, true},
                });

    utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer, true},
                });

    utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer, true},
                });

    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, true},
                }));

    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::Sampler, true},
                }));
}

// This test verifies that visibility of bindings in BindGroupLayout can be none
TEST_F(BindGroupLayoutValidationTest, BindGroupLayoutVisibilityNone) {
    utils::MakeBindGroupLayout(device,
                               {
                                   {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                               });

    wgpu::BindGroupLayoutEntry binding = {0, wgpu::ShaderStage::None,
                                          wgpu::BindingType::UniformBuffer};
    wgpu::BindGroupLayoutDescriptor descriptor;
    descriptor.entryCount = 1;
    descriptor.entries = &binding;
    device.CreateBindGroupLayout(&descriptor);
}

// This test verifies that binding with none visibility in bind group layout can be supported in
// bind group
TEST_F(BindGroupLayoutValidationTest, BindGroupLayoutVisibilityNoneExpectsBindGroupEntry) {
    wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                    {1, wgpu::ShaderStage::None, wgpu::BindingType::UniformBuffer},
                });
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::Uniform;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    utils::MakeBindGroup(device, bgl, {{0, buffer}, {1, buffer}});

    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, bgl, {{0, buffer}}));
}

TEST_F(BindGroupLayoutValidationTest, PerStageLimits) {
    struct TestInfo {
        uint32_t maxCount;
        wgpu::BindingType bindingType;
        wgpu::BindingType otherBindingType;
    };

    constexpr TestInfo kTestInfos[] = {
        {kMaxSampledTexturesPerShaderStage, wgpu::BindingType::SampledTexture,
         wgpu::BindingType::UniformBuffer},
        {kMaxSamplersPerShaderStage, wgpu::BindingType::Sampler, wgpu::BindingType::UniformBuffer},
        {kMaxSamplersPerShaderStage, wgpu::BindingType::ComparisonSampler,
         wgpu::BindingType::UniformBuffer},
        {kMaxStorageBuffersPerShaderStage, wgpu::BindingType::StorageBuffer,
         wgpu::BindingType::UniformBuffer},
        {kMaxStorageTexturesPerShaderStage, wgpu::BindingType::ReadonlyStorageTexture,
         wgpu::BindingType::UniformBuffer},
        {kMaxStorageTexturesPerShaderStage, wgpu::BindingType::WriteonlyStorageTexture,
         wgpu::BindingType::UniformBuffer},
        {kMaxUniformBuffersPerShaderStage, wgpu::BindingType::UniformBuffer,
         wgpu::BindingType::SampledTexture},
    };

    for (TestInfo info : kTestInfos) {
        wgpu::BindGroupLayout bgl[2];
        std::vector<wgpu::BindGroupLayoutEntry> maxBindings;

        auto PopulateEntry = [](wgpu::BindGroupLayoutEntry entry) {
            switch (entry.type) {
                case wgpu::BindingType::ReadonlyStorageTexture:
                case wgpu::BindingType::WriteonlyStorageTexture:
                    entry.storageTextureFormat = wgpu::TextureFormat::RGBA8Unorm;
                    break;
                default:
                    break;
            }
            return entry;
        };

        for (uint32_t i = 0; i < info.maxCount; ++i) {
            maxBindings.push_back(PopulateEntry({i, wgpu::ShaderStage::Compute, info.bindingType}));
        }

        // Creating with the maxes works.
        bgl[0] = MakeBindGroupLayout(maxBindings.data(), maxBindings.size());

        // Adding an extra binding of a different type works.
        {
            std::vector<wgpu::BindGroupLayoutEntry> bindings = maxBindings;
            bindings.push_back(
                PopulateEntry({info.maxCount, wgpu::ShaderStage::Compute, info.otherBindingType}));
            MakeBindGroupLayout(bindings.data(), bindings.size());
        }

        // Adding an extra binding of the maxed type in a different stage works
        {
            std::vector<wgpu::BindGroupLayoutEntry> bindings = maxBindings;
            bindings.push_back(
                PopulateEntry({info.maxCount, wgpu::ShaderStage::Fragment, info.bindingType}));
            MakeBindGroupLayout(bindings.data(), bindings.size());
        }

        // Adding an extra binding of the maxed type and stage exceeds the per stage limit.
        {
            std::vector<wgpu::BindGroupLayoutEntry> bindings = maxBindings;
            bindings.push_back(
                PopulateEntry({info.maxCount, wgpu::ShaderStage::Compute, info.bindingType}));
            ASSERT_DEVICE_ERROR(MakeBindGroupLayout(bindings.data(), bindings.size()));
        }

        // Creating a pipeline layout from the valid BGL works.
        TestCreatePipelineLayout(bgl, 1, true);

        // Adding an extra binding of a different type in a different BGL works
        bgl[1] = utils::MakeBindGroupLayout(
            device, {PopulateEntry({0, wgpu::ShaderStage::Compute, info.otherBindingType})});
        TestCreatePipelineLayout(bgl, 2, true);

        // Adding an extra binding of the maxed type in a different stage works
        bgl[1] = utils::MakeBindGroupLayout(
            device, {PopulateEntry({0, wgpu::ShaderStage::Fragment, info.bindingType})});
        TestCreatePipelineLayout(bgl, 2, true);

        // Adding an extra binding of the maxed type in a different BGL exceeds the per stage limit.
        bgl[1] = utils::MakeBindGroupLayout(
            device, {PopulateEntry({0, wgpu::ShaderStage::Compute, info.bindingType})});
        TestCreatePipelineLayout(bgl, 2, false);
    }
}

// Check that dynamic buffer numbers exceed maximum value in one bind group layout.
TEST_F(BindGroupLayoutValidationTest, DynamicBufferNumberLimit) {
    wgpu::BindGroupLayout bgl[2];
    std::vector<wgpu::BindGroupLayoutEntry> maxUniformDB;
    std::vector<wgpu::BindGroupLayoutEntry> maxStorageDB;
    std::vector<wgpu::BindGroupLayoutEntry> maxReadonlyStorageDB;

    // In this test, we use all the same shader stage. Ensure that this does not exceed the
    // per-stage limit.
    static_assert(kMaxDynamicUniformBuffersPerPipelineLayout <= kMaxUniformBuffersPerShaderStage,
                  "");
    static_assert(kMaxDynamicStorageBuffersPerPipelineLayout <= kMaxStorageBuffersPerShaderStage,
                  "");

    for (uint32_t i = 0; i < kMaxDynamicUniformBuffersPerPipelineLayout; ++i) {
        maxUniformDB.push_back(
            {i, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer, true});
    }

    for (uint32_t i = 0; i < kMaxDynamicStorageBuffersPerPipelineLayout; ++i) {
        maxStorageDB.push_back(
            {i, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer, true});
    }

    for (uint32_t i = 0; i < kMaxDynamicStorageBuffersPerPipelineLayout; ++i) {
        maxReadonlyStorageDB.push_back(
            {i, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer, true});
    }

    // Test creating with the maxes works
    {
        bgl[0] = MakeBindGroupLayout(maxUniformDB.data(), maxUniformDB.size());
        TestCreatePipelineLayout(bgl, 1, true);

        bgl[0] = MakeBindGroupLayout(maxStorageDB.data(), maxStorageDB.size());
        TestCreatePipelineLayout(bgl, 1, true);

        bgl[0] = MakeBindGroupLayout(maxReadonlyStorageDB.data(), maxReadonlyStorageDB.size());
        TestCreatePipelineLayout(bgl, 1, true);
    }

    // The following tests exceed the per-pipeline layout limits. We use the Fragment stage to
    // ensure we don't hit the per-stage limit.

    // Check dynamic uniform buffers exceed maximum in pipeline layout.
    {
        bgl[0] = MakeBindGroupLayout(maxUniformDB.data(), maxUniformDB.size());
        bgl[1] = utils::MakeBindGroupLayout(
            device, {
                        {0, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer, true},
                    });

        TestCreatePipelineLayout(bgl, 2, false);
    }

    // Check dynamic storage buffers exceed maximum in pipeline layout
    {
        bgl[0] = MakeBindGroupLayout(maxStorageDB.data(), maxStorageDB.size());
        bgl[1] = utils::MakeBindGroupLayout(
            device, {
                        {0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer, true},
                    });

        TestCreatePipelineLayout(bgl, 2, false);
    }

    // Check dynamic readonly storage buffers exceed maximum in pipeline layout
    {
        bgl[0] = MakeBindGroupLayout(maxReadonlyStorageDB.data(), maxReadonlyStorageDB.size());
        bgl[1] = utils::MakeBindGroupLayout(
            device,
            {
                {0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer, true},
            });

        TestCreatePipelineLayout(bgl, 2, false);
    }

    // Check dynamic storage buffers + dynamic readonly storage buffers exceed maximum storage
    // buffers in pipeline layout
    {
        bgl[0] = MakeBindGroupLayout(maxStorageDB.data(), maxStorageDB.size());
        bgl[1] = utils::MakeBindGroupLayout(
            device,
            {
                {0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer, true},
            });

        TestCreatePipelineLayout(bgl, 2, false);
    }

    // Check dynamic uniform buffers exceed maximum in bind group layout.
    {
        maxUniformDB.push_back({kMaxDynamicUniformBuffersPerPipelineLayout,
                                wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer,
                                true});
        TestCreateBindGroupLayout(maxUniformDB.data(), maxUniformDB.size(), false);
    }

    // Check dynamic storage buffers exceed maximum in bind group layout.
    {
        maxStorageDB.push_back({kMaxDynamicStorageBuffersPerPipelineLayout,
                                wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer,
                                true});
        TestCreateBindGroupLayout(maxStorageDB.data(), maxStorageDB.size(), false);
    }

    // Check dynamic readonly storage buffers exceed maximum in bind group layout.
    {
        maxReadonlyStorageDB.push_back({kMaxDynamicStorageBuffersPerPipelineLayout,
                                        wgpu::ShaderStage::Fragment,
                                        wgpu::BindingType::ReadonlyStorageBuffer, true});
        TestCreateBindGroupLayout(maxReadonlyStorageDB.data(), maxReadonlyStorageDB.size(), false);
    }
}

// Test that multisampled textures must be 2D sampled textures
TEST_F(BindGroupLayoutValidationTest, MultisampledTextures) {
    // Multisampled 2D texture works.
    utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                     true, wgpu::TextureViewDimension::e2D},
                });

    // Multisampled 2D (defaulted) texture works.
    utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                     true, wgpu::TextureViewDimension::Undefined},
                });

    // Multisampled 2D storage texture is invalid.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture,
                     false, 0, true, wgpu::TextureViewDimension::e2D},
                }));

    // Multisampled 2D array texture is invalid.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                     true, wgpu::TextureViewDimension::e2DArray},
                }));

    // Multisampled cube texture is invalid.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                     true, wgpu::TextureViewDimension::Cube},
                }));

    // Multisampled cube array texture is invalid.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                     true, wgpu::TextureViewDimension::CubeArray},
                }));

    // Multisampled 3D texture is invalid.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                     true, wgpu::TextureViewDimension::e3D},
                }));

    // Multisampled 1D texture is invalid.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                     true, wgpu::TextureViewDimension::e1D},
                }));
}

// Test that it is an error to pass multisampled=true for non-texture bindings
TEST_F(BindGroupLayoutValidationTest, MultisampledMustBeTexture) {
    // Base: Multisampled 2D texture works.
    utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                     true, wgpu::TextureViewDimension::e2D},
                });

    // Multisampled uniform buffer binding is invalid
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device,
        {
            {0, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer, false, 0, true},
        }));

    // Multisampled storage buffer binding is invalid
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device,
        {
            {0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer, false, 0, true},
        }));

    // Multisampled sampler binding is invalid
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::Sampler, false, 0, true},
                }));
}

constexpr uint64_t kBufferSize = 3 * kMinDynamicBufferOffsetAlignment + 8;
constexpr uint32_t kBindingSize = 9;

class SetBindGroupValidationTest : public ValidationTest {
  public:
    void SetUp() override {
        mBindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                      wgpu::BindingType::UniformBuffer, true},
                     {1, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                      wgpu::BindingType::UniformBuffer, false},
                     {2, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                      wgpu::BindingType::StorageBuffer, true},
                     {3, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                      wgpu::BindingType::ReadonlyStorageBuffer, true}});
    }

    wgpu::Buffer CreateBuffer(uint64_t bufferSize, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor bufferDescriptor;
        bufferDescriptor.size = bufferSize;
        bufferDescriptor.usage = usage;

        return device.CreateBuffer(&bufferDescriptor);
    }

    wgpu::BindGroupLayout mBindGroupLayout;

    wgpu::RenderPipeline CreateRenderPipeline() {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                })");

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(std140, set = 0, binding = 0) uniform uBufferDynamic {
                    vec2 value0;
                };
                layout(std140, set = 0, binding = 1) uniform uBuffer {
                    vec2 value1;
                };
                layout(std140, set = 0, binding = 2) buffer SBufferDynamic {
                    vec2 value2;
                } sBuffer;
                layout(std140, set = 0, binding = 3) readonly buffer RBufferDynamic {
                    vec2 value3;
                } rBuffer;
                layout(location = 0) out vec4 fragColor;
                void main() {
                })");

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.vertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        wgpu::PipelineLayout pipelineLayout =
            utils::MakeBasicPipelineLayout(device, &mBindGroupLayout);
        pipelineDescriptor.layout = pipelineLayout;
        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    wgpu::ComputePipeline CreateComputePipeline() {
        wgpu::ShaderModule csModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
                #version 450
                const uint kTileSize = 4;
                const uint kInstances = 11;

                layout(local_size_x = kTileSize, local_size_y = kTileSize, local_size_z = 1) in;
                layout(std140, set = 0, binding = 0) uniform UniformBufferDynamic {
                    float value0;
                };
                layout(std140, set = 0, binding = 1) uniform UniformBuffer {
                    float value1;
                };
                layout(std140, set = 0, binding = 2) buffer SBufferDynamic {
                    float value2;
                } dst;
                layout(std140, set = 0, binding = 3) readonly buffer RBufferDynamic {
                    readonly float value3;
                } rdst;
                void main() {
                })");

        wgpu::PipelineLayout pipelineLayout =
            utils::MakeBasicPipelineLayout(device, &mBindGroupLayout);

        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.layout = pipelineLayout;
        csDesc.computeStage.module = csModule;
        csDesc.computeStage.entryPoint = "main";

        return device.CreateComputePipeline(&csDesc);
    }

    void TestRenderPassBindGroup(wgpu::BindGroup bindGroup,
                                 uint32_t* offsets,
                                 uint32_t count,
                                 bool expectation) {
        wgpu::RenderPipeline renderPipeline = CreateRenderPipeline();
        DummyRenderPass renderPass(device);

        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.SetPipeline(renderPipeline);
        if (bindGroup != nullptr) {
            renderPassEncoder.SetBindGroup(0, bindGroup, count, offsets);
        }
        renderPassEncoder.Draw(3);
        renderPassEncoder.EndPass();
        if (!expectation) {
            ASSERT_DEVICE_ERROR(commandEncoder.Finish());
        } else {
            commandEncoder.Finish();
        }
    }

    void TestComputePassBindGroup(wgpu::BindGroup bindGroup,
                                  uint32_t* offsets,
                                  uint32_t count,
                                  bool expectation) {
        wgpu::ComputePipeline computePipeline = CreateComputePipeline();

        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetPipeline(computePipeline);
        if (bindGroup != nullptr) {
            computePassEncoder.SetBindGroup(0, bindGroup, count, offsets);
        }
        computePassEncoder.Dispatch(1);
        computePassEncoder.EndPass();
        if (!expectation) {
            ASSERT_DEVICE_ERROR(commandEncoder.Finish());
        } else {
            commandEncoder.Finish();
        }
    }
};

// This is the test case that should work.
TEST_F(SetBindGroupValidationTest, Basic) {
    // Set up the bind group.
    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::Buffer readonlyStorageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout,
                                                     {{0, uniformBuffer, 0, kBindingSize},
                                                      {1, uniformBuffer, 0, kBindingSize},
                                                      {2, storageBuffer, 0, kBindingSize},
                                                      {3, readonlyStorageBuffer, 0, kBindingSize}});

    std::array<uint32_t, 3> offsets = {512, 256, 0};

    TestRenderPassBindGroup(bindGroup, offsets.data(), 3, true);

    TestComputePassBindGroup(bindGroup, offsets.data(), 3, true);
}

// Draw/dispatch with a bind group missing is invalid
TEST_F(SetBindGroupValidationTest, MissingBindGroup) {
    TestRenderPassBindGroup(nullptr, nullptr, 0, false);
    TestComputePassBindGroup(nullptr, nullptr, 0, false);
}

// Setting bind group after a draw / dispatch should re-verify the layout is compatible
TEST_F(SetBindGroupValidationTest, VerifyGroupIfChangedAfterAction) {
    // Set up the bind group
    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::Buffer readonlyStorageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout,
                                                     {{0, uniformBuffer, 0, kBindingSize},
                                                      {1, uniformBuffer, 0, kBindingSize},
                                                      {2, storageBuffer, 0, kBindingSize},
                                                      {3, readonlyStorageBuffer, 0, kBindingSize}});

    std::array<uint32_t, 3> offsets = {512, 256, 0};

    // Set up bind group that is incompatible
    wgpu::BindGroupLayout invalidLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::StorageBuffer}});
    wgpu::BindGroup invalidGroup =
        utils::MakeBindGroup(device, invalidLayout, {{0, storageBuffer, 0, kBindingSize}});

    {
        wgpu::ComputePipeline computePipeline = CreateComputePipeline();
        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetPipeline(computePipeline);
        computePassEncoder.SetBindGroup(0, bindGroup, 3, offsets.data());
        computePassEncoder.Dispatch(1);
        computePassEncoder.SetBindGroup(0, invalidGroup, 0, nullptr);
        computePassEncoder.Dispatch(1);
        computePassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
    {
        wgpu::RenderPipeline renderPipeline = CreateRenderPipeline();
        DummyRenderPass renderPass(device);

        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.SetPipeline(renderPipeline);
        renderPassEncoder.SetBindGroup(0, bindGroup, 3, offsets.data());
        renderPassEncoder.Draw(3);
        renderPassEncoder.SetBindGroup(0, invalidGroup, 0, nullptr);
        renderPassEncoder.Draw(3);
        renderPassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
}

// Test cases that test dynamic offsets count mismatch with bind group layout.
TEST_F(SetBindGroupValidationTest, DynamicOffsetsMismatch) {
    // Set up bind group.
    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::Buffer readonlyStorageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout,
                                                     {{0, uniformBuffer, 0, kBindingSize},
                                                      {1, uniformBuffer, 0, kBindingSize},
                                                      {2, storageBuffer, 0, kBindingSize},
                                                      {3, readonlyStorageBuffer, 0, kBindingSize}});

    // Number of offsets mismatch.
    std::array<uint32_t, 4> mismatchOffsets = {768, 512, 256, 0};

    TestRenderPassBindGroup(bindGroup, mismatchOffsets.data(), 1, false);
    TestRenderPassBindGroup(bindGroup, mismatchOffsets.data(), 2, false);
    TestRenderPassBindGroup(bindGroup, mismatchOffsets.data(), 4, false);

    TestComputePassBindGroup(bindGroup, mismatchOffsets.data(), 1, false);
    TestComputePassBindGroup(bindGroup, mismatchOffsets.data(), 2, false);
    TestComputePassBindGroup(bindGroup, mismatchOffsets.data(), 4, false);
}

// Test cases that test dynamic offsets not aligned
TEST_F(SetBindGroupValidationTest, DynamicOffsetsNotAligned) {
    // Set up bind group.
    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::Buffer readonlyStorageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout,
                                                     {{0, uniformBuffer, 0, kBindingSize},
                                                      {1, uniformBuffer, 0, kBindingSize},
                                                      {2, storageBuffer, 0, kBindingSize},
                                                      {3, readonlyStorageBuffer, 0, kBindingSize}});

    // Dynamic offsets are not aligned.
    std::array<uint32_t, 3> notAlignedOffsets = {512, 128, 0};

    TestRenderPassBindGroup(bindGroup, notAlignedOffsets.data(), 3, false);

    TestComputePassBindGroup(bindGroup, notAlignedOffsets.data(), 3, false);
}

// Test cases that test dynamic uniform buffer out of bound situation.
TEST_F(SetBindGroupValidationTest, OffsetOutOfBoundDynamicUniformBuffer) {
    // Set up bind group.
    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::Buffer readonlyStorageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout,
                                                     {{0, uniformBuffer, 0, kBindingSize},
                                                      {1, uniformBuffer, 0, kBindingSize},
                                                      {2, storageBuffer, 0, kBindingSize},
                                                      {3, readonlyStorageBuffer, 0, kBindingSize}});

    // Dynamic offset + offset is larger than buffer size.
    std::array<uint32_t, 3> overFlowOffsets = {1024, 256, 0};

    TestRenderPassBindGroup(bindGroup, overFlowOffsets.data(), 3, false);

    TestComputePassBindGroup(bindGroup, overFlowOffsets.data(), 3, false);
}

// Test cases that test dynamic storage buffer out of bound situation.
TEST_F(SetBindGroupValidationTest, OffsetOutOfBoundDynamicStorageBuffer) {
    // Set up bind group.
    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::Buffer readonlyStorageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout,
                                                     {{0, uniformBuffer, 0, kBindingSize},
                                                      {1, uniformBuffer, 0, kBindingSize},
                                                      {2, storageBuffer, 0, kBindingSize},
                                                      {3, readonlyStorageBuffer, 0, kBindingSize}});

    // Dynamic offset + offset is larger than buffer size.
    std::array<uint32_t, 3> overFlowOffsets = {0, 256, 1024};

    TestRenderPassBindGroup(bindGroup, overFlowOffsets.data(), 3, false);

    TestComputePassBindGroup(bindGroup, overFlowOffsets.data(), 3, false);
}

// Test cases that test dynamic uniform buffer out of bound situation because of binding size.
TEST_F(SetBindGroupValidationTest, BindingSizeOutOfBoundDynamicUniformBuffer) {
    // Set up bind group, but binding size is larger than
    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::Buffer readonlyStorageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout,
                                                     {{0, uniformBuffer, 0, kBindingSize},
                                                      {1, uniformBuffer, 0, kBindingSize},
                                                      {2, storageBuffer, 0, kBindingSize},
                                                      {3, readonlyStorageBuffer, 0, kBindingSize}});

    // Dynamic offset + offset isn't larger than buffer size.
    // But with binding size, it will trigger OOB error.
    std::array<uint32_t, 3> offsets = {768, 256, 0};

    TestRenderPassBindGroup(bindGroup, offsets.data(), 3, false);

    TestComputePassBindGroup(bindGroup, offsets.data(), 3, false);
}

// Test cases that test dynamic storage buffer out of bound situation because of binding size.
TEST_F(SetBindGroupValidationTest, BindingSizeOutOfBoundDynamicStorageBuffer) {
    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::Buffer readonlyStorageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, mBindGroupLayout,
                                                     {{0, uniformBuffer, 0, kBindingSize},
                                                      {1, uniformBuffer, 0, kBindingSize},
                                                      {2, storageBuffer, 0, kBindingSize},
                                                      {3, readonlyStorageBuffer, 0, kBindingSize}});
    // Dynamic offset + offset isn't larger than buffer size.
    // But with binding size, it will trigger OOB error.
    std::array<uint32_t, 3> offsets = {0, 256, 768};

    TestRenderPassBindGroup(bindGroup, offsets.data(), 3, false);

    TestComputePassBindGroup(bindGroup, offsets.data(), 3, false);
}

// Regression test for crbug.com/dawn/408 where dynamic offsets were applied in the wrong order.
// Dynamic offsets should be applied in increasing order of binding number.
TEST_F(SetBindGroupValidationTest, DynamicOffsetOrder) {
    // Note: The order of the binding numbers of the bind group and bind group layout are
    // intentionally different and not in increasing order.
    // This test uses both storage and uniform buffers to ensure buffer bindings are sorted first by
    // binding number before type.
    wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device, {
                    {3, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer, true},
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer, true},
                    {2, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer, true},
                });

    // Create buffers which are 3x, 2x, and 1x the size of the minimum buffer offset, plus 4 bytes
    // to spare (to avoid zero-sized bindings). We will offset the bindings so they reach the very
    // end of the buffer. Any mismatch applying too-large of an offset to a smaller buffer will hit
    // the out-of-bounds condition during validation.
    wgpu::Buffer buffer3x =
        CreateBuffer(3 * kMinDynamicBufferOffsetAlignment + 4, wgpu::BufferUsage::Storage);
    wgpu::Buffer buffer2x =
        CreateBuffer(2 * kMinDynamicBufferOffsetAlignment + 4, wgpu::BufferUsage::Storage);
    wgpu::Buffer buffer1x =
        CreateBuffer(1 * kMinDynamicBufferOffsetAlignment + 4, wgpu::BufferUsage::Uniform);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, bgl,
                                                     {
                                                         {0, buffer3x, 0, 4},
                                                         {3, buffer2x, 0, 4},
                                                         {2, buffer1x, 0, 4},
                                                     });

    std::array<uint32_t, 3> offsets;
    {
        // Base case works.
        offsets = {/* binding 0 */ 0,
                   /* binding 2 */ 0,
                   /* binding 3 */ 0};
        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetBindGroup(0, bindGroup, offsets.size(), offsets.data());
        computePassEncoder.EndPass();
        commandEncoder.Finish();
    }
    {
        // Offset the first binding to touch the end of the buffer. Should succeed.
        // Will fail if the offset is applied to the first or second bindings since their buffers
        // are too small.
        offsets = {/* binding 0 */ 3 * kMinDynamicBufferOffsetAlignment,
                   /* binding 2 */ 0,
                   /* binding 3 */ 0};
        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetBindGroup(0, bindGroup, offsets.size(), offsets.data());
        computePassEncoder.EndPass();
        commandEncoder.Finish();
    }
    {
        // Offset the second binding to touch the end of the buffer. Should succeed.
        offsets = {/* binding 0 */ 0,
                   /* binding 2 */ 1 * kMinDynamicBufferOffsetAlignment,
                   /* binding 3 */ 0};
        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetBindGroup(0, bindGroup, offsets.size(), offsets.data());
        computePassEncoder.EndPass();
        commandEncoder.Finish();
    }
    {
        // Offset the third binding to touch the end of the buffer. Should succeed.
        // Will fail if the offset is applied to the second binding since its buffer
        // is too small.
        offsets = {/* binding 0 */ 0,
                   /* binding 2 */ 0,
                   /* binding 3 */ 2 * kMinDynamicBufferOffsetAlignment};
        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetBindGroup(0, bindGroup, offsets.size(), offsets.data());
        computePassEncoder.EndPass();
        commandEncoder.Finish();
    }
    {
        // Offset each binding to touch the end of their buffer. Should succeed.
        offsets = {/* binding 0 */ 3 * kMinDynamicBufferOffsetAlignment,
                   /* binding 2 */ 1 * kMinDynamicBufferOffsetAlignment,
                   /* binding 3 */ 2 * kMinDynamicBufferOffsetAlignment};
        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetBindGroup(0, bindGroup, offsets.size(), offsets.data());
        computePassEncoder.EndPass();
        commandEncoder.Finish();
    }
}

// Test that an error is produced (and no ASSERTs fired) when using an error bindgroup in
// SetBindGroup
TEST_F(SetBindGroupValidationTest, ErrorBindGroup) {
    // Bindgroup creation fails because not all bindings are specified.
    wgpu::BindGroup bindGroup;
    ASSERT_DEVICE_ERROR(bindGroup = utils::MakeBindGroup(device, mBindGroupLayout, {}));

    TestRenderPassBindGroup(bindGroup, nullptr, 0, false);

    TestComputePassBindGroup(bindGroup, nullptr, 0, false);
}

class SetBindGroupPersistenceValidationTest : public ValidationTest {
  protected:
    void SetUp() override {
        mVsModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            void main() {
            })");
    }

    wgpu::Buffer CreateBuffer(uint64_t bufferSize, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor bufferDescriptor;
        bufferDescriptor.size = bufferSize;
        bufferDescriptor.usage = usage;

        return device.CreateBuffer(&bufferDescriptor);
    }

    // Generates bind group layouts and a pipeline from a 2D list of binding types.
    std::tuple<std::vector<wgpu::BindGroupLayout>, wgpu::RenderPipeline> SetUpLayoutsAndPipeline(
        std::vector<std::vector<wgpu::BindingType>> layouts) {
        std::vector<wgpu::BindGroupLayout> bindGroupLayouts(layouts.size());

        // Iterate through the desired bind group layouts.
        for (uint32_t l = 0; l < layouts.size(); ++l) {
            const auto& layout = layouts[l];
            std::vector<wgpu::BindGroupLayoutEntry> bindings(layout.size());

            // Iterate through binding types and populate a list of BindGroupLayoutEntrys.
            for (uint32_t b = 0; b < layout.size(); ++b) {
                bindings[b] = {b, wgpu::ShaderStage::Fragment, layout[b], false};
            }

            // Create the bind group layout.
            wgpu::BindGroupLayoutDescriptor bglDescriptor;
            bglDescriptor.entryCount = static_cast<uint32_t>(bindings.size());
            bglDescriptor.entries = bindings.data();
            bindGroupLayouts[l] = device.CreateBindGroupLayout(&bglDescriptor);
        }

        // Create a pipeline layout from the list of bind group layouts.
        wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor;
        pipelineLayoutDescriptor.bindGroupLayoutCount =
            static_cast<uint32_t>(bindGroupLayouts.size());
        pipelineLayoutDescriptor.bindGroupLayouts = bindGroupLayouts.data();

        wgpu::PipelineLayout pipelineLayout =
            device.CreatePipelineLayout(&pipelineLayoutDescriptor);

        std::stringstream ss;
        ss << "#version 450\n";

        // Build a shader which has bindings that match the pipeline layout.
        for (uint32_t l = 0; l < layouts.size(); ++l) {
            const auto& layout = layouts[l];

            for (uint32_t b = 0; b < layout.size(); ++b) {
                wgpu::BindingType binding = layout[b];
                ss << "layout(std140, set = " << l << ", binding = " << b << ") ";
                switch (binding) {
                    case wgpu::BindingType::StorageBuffer:
                        ss << "buffer SBuffer";
                        break;
                    case wgpu::BindingType::UniformBuffer:
                        ss << "uniform UBuffer";
                        break;
                    default:
                        UNREACHABLE();
                }
                ss << l << "_" << b << " { vec2 set" << l << "_binding" << b << "; };\n";
            }
        }

        ss << "layout(location = 0) out vec4 fragColor;\n";
        ss << "void main() { fragColor = vec4(0.0, 1.0, 0.0, 1.0); }\n";

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, ss.str().c_str());

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.vertexStage.module = mVsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        pipelineDescriptor.layout = pipelineLayout;
        wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

        return std::make_tuple(bindGroupLayouts, pipeline);
    }

  private:
    wgpu::ShaderModule mVsModule;
};

// Test it is valid to set bind groups before setting the pipeline.
TEST_F(SetBindGroupPersistenceValidationTest, BindGroupBeforePipeline) {
    std::vector<wgpu::BindGroupLayout> bindGroupLayouts;
    wgpu::RenderPipeline pipeline;
    std::tie(bindGroupLayouts, pipeline) = SetUpLayoutsAndPipeline({{
        {{
            wgpu::BindingType::UniformBuffer,
            wgpu::BindingType::UniformBuffer,
        }},
        {{
            wgpu::BindingType::StorageBuffer,
            wgpu::BindingType::UniformBuffer,
        }},
    }});

    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);

    wgpu::BindGroup bindGroup0 = utils::MakeBindGroup(
        device, bindGroupLayouts[0],
        {{0, uniformBuffer, 0, kBindingSize}, {1, uniformBuffer, 0, kBindingSize}});

    wgpu::BindGroup bindGroup1 = utils::MakeBindGroup(
        device, bindGroupLayouts[1],
        {{0, storageBuffer, 0, kBindingSize}, {1, uniformBuffer, 0, kBindingSize}});

    DummyRenderPass renderPass(device);
    wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);

    renderPassEncoder.SetBindGroup(0, bindGroup0);
    renderPassEncoder.SetBindGroup(1, bindGroup1);
    renderPassEncoder.SetPipeline(pipeline);
    renderPassEncoder.Draw(3);

    renderPassEncoder.EndPass();
    commandEncoder.Finish();
}

// Dawn does not have a concept of bind group inheritance though the backing APIs may.
// Test that it is valid to draw with bind groups that are not "inherited". They persist
// after a pipeline change.
TEST_F(SetBindGroupPersistenceValidationTest, NotVulkanInheritance) {
    std::vector<wgpu::BindGroupLayout> bindGroupLayoutsA;
    wgpu::RenderPipeline pipelineA;
    std::tie(bindGroupLayoutsA, pipelineA) = SetUpLayoutsAndPipeline({{
        {{
            wgpu::BindingType::UniformBuffer,
            wgpu::BindingType::StorageBuffer,
        }},
        {{
            wgpu::BindingType::UniformBuffer,
            wgpu::BindingType::UniformBuffer,
        }},
    }});

    std::vector<wgpu::BindGroupLayout> bindGroupLayoutsB;
    wgpu::RenderPipeline pipelineB;
    std::tie(bindGroupLayoutsB, pipelineB) = SetUpLayoutsAndPipeline({{
        {{
            wgpu::BindingType::StorageBuffer,
            wgpu::BindingType::UniformBuffer,
        }},
        {{
            wgpu::BindingType::UniformBuffer,
            wgpu::BindingType::UniformBuffer,
        }},
    }});

    wgpu::Buffer uniformBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    wgpu::Buffer storageBuffer = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);

    wgpu::BindGroup bindGroupA0 = utils::MakeBindGroup(
        device, bindGroupLayoutsA[0],
        {{0, uniformBuffer, 0, kBindingSize}, {1, storageBuffer, 0, kBindingSize}});

    wgpu::BindGroup bindGroupA1 = utils::MakeBindGroup(
        device, bindGroupLayoutsA[1],
        {{0, uniformBuffer, 0, kBindingSize}, {1, uniformBuffer, 0, kBindingSize}});

    wgpu::BindGroup bindGroupB0 = utils::MakeBindGroup(
        device, bindGroupLayoutsB[0],
        {{0, storageBuffer, 0, kBindingSize}, {1, uniformBuffer, 0, kBindingSize}});

    DummyRenderPass renderPass(device);
    wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);

    renderPassEncoder.SetPipeline(pipelineA);
    renderPassEncoder.SetBindGroup(0, bindGroupA0);
    renderPassEncoder.SetBindGroup(1, bindGroupA1);
    renderPassEncoder.Draw(3);

    renderPassEncoder.SetPipeline(pipelineB);
    renderPassEncoder.SetBindGroup(0, bindGroupB0);
    // This draw is valid.
    // Bind group 1 persists even though it is not "inherited".
    renderPassEncoder.Draw(3);

    renderPassEncoder.EndPass();
    commandEncoder.Finish();
}

class BindGroupLayoutCompatibilityTest : public ValidationTest {
  public:
    wgpu::Buffer CreateBuffer(uint64_t bufferSize, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor bufferDescriptor;
        bufferDescriptor.size = bufferSize;
        bufferDescriptor.usage = usage;

        return device.CreateBuffer(&bufferDescriptor);
    }

    wgpu::RenderPipeline CreateFSRenderPipeline(
        const char* fsShader,
        std::vector<wgpu::BindGroupLayout> bindGroupLayout) {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                })");

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, fsShader);

        wgpu::PipelineLayoutDescriptor descriptor;
        descriptor.bindGroupLayoutCount = bindGroupLayout.size();
        descriptor.bindGroupLayouts = bindGroupLayout.data();
        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.vertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&descriptor);
        pipelineDescriptor.layout = pipelineLayout;
        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    wgpu::RenderPipeline CreateRenderPipeline(std::vector<wgpu::BindGroupLayout> bindGroupLayout) {
        return CreateFSRenderPipeline(R"(
                #version 450
                layout(std140, set = 0, binding = 0) buffer SBuffer {
                    vec2 value2;
                } sBuffer;
                layout(std140, set = 1, binding = 0) readonly buffer RBuffer {
                    vec2 value3;
                } rBuffer;
                layout(location = 0) out vec4 fragColor;
                void main() {
                })",
                                      std::move(bindGroupLayout));
    }

    wgpu::ComputePipeline CreateComputePipeline(
        const char* shader,
        std::vector<wgpu::BindGroupLayout> bindGroupLayout) {
        wgpu::ShaderModule csModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, shader);

        wgpu::PipelineLayoutDescriptor descriptor;
        descriptor.bindGroupLayoutCount = bindGroupLayout.size();
        descriptor.bindGroupLayouts = bindGroupLayout.data();
        wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&descriptor);

        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.layout = pipelineLayout;
        csDesc.computeStage.module = csModule;
        csDesc.computeStage.entryPoint = "main";

        return device.CreateComputePipeline(&csDesc);
    }

    wgpu::ComputePipeline CreateComputePipeline(
        std::vector<wgpu::BindGroupLayout> bindGroupLayout) {
        return CreateComputePipeline(R"(
                #version 450
                const uint kTileSize = 4;
                const uint kInstances = 11;

                layout(local_size_x = kTileSize, local_size_y = kTileSize, local_size_z = 1) in;
                layout(std140, set = 0, binding = 0) buffer SBuffer {
                    float value2;
                } dst;
                layout(std140, set = 1, binding = 0) readonly buffer RBuffer {
                    readonly float value3;
                } rdst;
                void main() {
                })",
                                     std::move(bindGroupLayout));
    }
};

// Test that it is valid to pass a writable storage buffer in the pipeline layout when the shader
// uses the binding as a readonly storage buffer.
TEST_F(BindGroupLayoutCompatibilityTest, RWStorageInBGLWithROStorageInShader) {
    // Set up the bind group layout.
    wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::StorageBuffer}});
    wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::StorageBuffer}});

    CreateRenderPipeline({bgl0, bgl1});

    CreateComputePipeline({bgl0, bgl1});
}

// Test that it is invalid to pass a readonly storage buffer in the pipeline layout when the shader
// uses the binding as a writable storage buffer.
TEST_F(BindGroupLayoutCompatibilityTest, ROStorageInBGLWithRWStorageInShader) {
    // Set up the bind group layout.
    wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::ReadonlyStorageBuffer}});
    wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::ReadonlyStorageBuffer}});

    ASSERT_DEVICE_ERROR(CreateRenderPipeline({bgl0, bgl1}));

    ASSERT_DEVICE_ERROR(CreateComputePipeline({bgl0, bgl1}));
}

TEST_F(BindGroupLayoutCompatibilityTest, TextureViewDimension) {
    constexpr char kTexture2DShader[] = R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2D texture;
        void main() {
        })";

    // Render: Test that 2D texture with 2D view dimension works
    CreateFSRenderPipeline(
        kTexture2DShader,
        {utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false, 0,
                      false, wgpu::TextureViewDimension::e2D}})});

    // Render: Test that 2D texture with 2D array view dimension is invalid
    ASSERT_DEVICE_ERROR(CreateFSRenderPipeline(
        kTexture2DShader,
        {utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false, 0,
                      false, wgpu::TextureViewDimension::e2DArray}})}));

    // Compute: Test that 2D texture with 2D view dimension works
    CreateComputePipeline(
        kTexture2DShader,
        {utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                      false, wgpu::TextureViewDimension::e2D}})});

    // Compute: Test that 2D texture with 2D array view dimension is invalid
    ASSERT_DEVICE_ERROR(CreateComputePipeline(
        kTexture2DShader,
        {utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                      false, wgpu::TextureViewDimension::e2DArray}})}));

    constexpr char kTexture2DArrayShader[] = R"(
        #version 450
        layout(set = 0, binding = 0) uniform texture2DArray texture;
        void main() {
        })";

    // Render: Test that 2D texture array with 2D array view dimension works
    CreateFSRenderPipeline(
        kTexture2DArrayShader,
        {utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false, 0,
                      false, wgpu::TextureViewDimension::e2DArray}})});

    // Render: Test that 2D texture array with 2D view dimension is invalid
    ASSERT_DEVICE_ERROR(CreateFSRenderPipeline(
        kTexture2DArrayShader,
        {utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture, false, 0,
                      false, wgpu::TextureViewDimension::e2D}})}));

    // Compute: Test that 2D texture array with 2D array view dimension works
    CreateComputePipeline(
        kTexture2DArrayShader,
        {utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                      false, wgpu::TextureViewDimension::e2DArray}})});

    // Compute: Test that 2D texture array with 2D view dimension is invalid
    ASSERT_DEVICE_ERROR(CreateComputePipeline(
        kTexture2DArrayShader,
        {utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture, false, 0,
                      false, wgpu::TextureViewDimension::e2D}})}));
}

class BindingsValidationTest : public BindGroupLayoutCompatibilityTest {
  public:
    void TestRenderPassBindings(const wgpu::BindGroup* bg,
                                uint32_t count,
                                wgpu::RenderPipeline pipeline,
                                bool expectation) {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        DummyRenderPass dummyRenderPass(device);
        wgpu::RenderPassEncoder rp = encoder.BeginRenderPass(&dummyRenderPass);
        for (uint32_t i = 0; i < count; ++i) {
            rp.SetBindGroup(i, bg[i]);
        }
        rp.SetPipeline(pipeline);
        rp.Draw(3);
        rp.EndPass();
        if (!expectation) {
            ASSERT_DEVICE_ERROR(encoder.Finish());
        } else {
            encoder.Finish();
        }
    }

    void TestComputePassBindings(const wgpu::BindGroup* bg,
                                 uint32_t count,
                                 wgpu::ComputePipeline pipeline,
                                 bool expectation) {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder cp = encoder.BeginComputePass();
        for (uint32_t i = 0; i < count; ++i) {
            cp.SetBindGroup(i, bg[i]);
        }
        cp.SetPipeline(pipeline);
        cp.Dispatch(1);
        cp.EndPass();
        if (!expectation) {
            ASSERT_DEVICE_ERROR(encoder.Finish());
        } else {
            encoder.Finish();
        }
    }

    static constexpr uint32_t kBindingNum = 3;
};

// Test that it is valid to set a pipeline layout with bindings unused by the pipeline.
TEST_F(BindingsValidationTest, PipelineLayoutWithMoreBindingsThanPipeline) {
    // Set up bind group layouts.
    wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::StorageBuffer},
                 {1, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::UniformBuffer}});
    wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::ReadonlyStorageBuffer}});
    wgpu::BindGroupLayout bgl2 = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::StorageBuffer}});

    // pipelineLayout has unused binding set (bgl2) and unused entry in a binding set (bgl0).
    CreateRenderPipeline({bgl0, bgl1, bgl2});

    CreateComputePipeline({bgl0, bgl1, bgl2});
}

// Test that it is invalid to set a pipeline layout that doesn't have all necessary bindings
// required by the pipeline.
TEST_F(BindingsValidationTest, PipelineLayoutWithLessBindingsThanPipeline) {
    // Set up bind group layout.
    wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::StorageBuffer}});

    // missing a binding set (bgl1) in pipeline layout
    {
        ASSERT_DEVICE_ERROR(CreateRenderPipeline({bgl0}));

        ASSERT_DEVICE_ERROR(CreateComputePipeline({bgl0}));
    }

    // bgl1 is not missing, but it is empty
    {
        wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(device, {});

        ASSERT_DEVICE_ERROR(CreateRenderPipeline({bgl0, bgl1}));

        ASSERT_DEVICE_ERROR(CreateComputePipeline({bgl0, bgl1}));
    }

    // bgl1 is neither missing nor empty, but it doesn't contain the necessary binding
    {
        wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
            device, {{1, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                      wgpu::BindingType::UniformBuffer}});

        ASSERT_DEVICE_ERROR(CreateRenderPipeline({bgl0, bgl1}));

        ASSERT_DEVICE_ERROR(CreateComputePipeline({bgl0, bgl1}));
    }
}

// Test that it is valid to set bind groups whose layout is not set in the pipeline layout.
// But it's invalid to set extra entry for a given bind group's layout if that layout is set in
// the pipeline layout.
TEST_F(BindingsValidationTest, BindGroupsWithMoreBindingsThanPipelineLayout) {
    // Set up bind group layouts, buffers, bind groups, pipeline layouts and pipelines.
    std::array<wgpu::BindGroupLayout, kBindingNum + 1> bgl;
    std::array<wgpu::BindGroup, kBindingNum + 1> bg;
    std::array<wgpu::Buffer, kBindingNum + 1> buffer;
    for (uint32_t i = 0; i < kBindingNum + 1; ++i) {
        bgl[i] = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                      wgpu::BindingType::StorageBuffer}});
        buffer[i] = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
        bg[i] = utils::MakeBindGroup(device, bgl[i], {{0, buffer[i]}});
    }

    // Set 3 bindings (and 3 pipeline layouts) in pipeline.
    wgpu::RenderPipeline renderPipeline = CreateRenderPipeline({bgl[0], bgl[1], bgl[2]});
    wgpu::ComputePipeline computePipeline = CreateComputePipeline({bgl[0], bgl[1], bgl[2]});

    // Comprared to pipeline layout, there is an extra bind group (bg[3])
    TestRenderPassBindings(bg.data(), kBindingNum + 1, renderPipeline, true);

    TestComputePassBindings(bg.data(), kBindingNum + 1, computePipeline, true);

    // If a bind group has entry (like bgl1_1 below) unused by the pipeline layout, it is invalid.
    // Bind groups associated layout should exactly match bind group layout if that layout is
    // set in pipeline layout.
    bgl[1] = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::ReadonlyStorageBuffer},
                 {1, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::UniformBuffer}});
    buffer[1] = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Uniform);
    bg[1] = utils::MakeBindGroup(device, bgl[1], {{0, buffer[1]}, {1, buffer[1]}});

    TestRenderPassBindings(bg.data(), kBindingNum, renderPipeline, false);

    TestComputePassBindings(bg.data(), kBindingNum, computePipeline, false);
}

// Test that it is invalid to set bind groups that don't have all necessary bindings required
// by the pipeline layout. Note that both pipeline layout and bind group have enough bindings for
// pipeline in the following test.
TEST_F(BindingsValidationTest, BindGroupsWithLessBindingsThanPipelineLayout) {
    // Set up bind group layouts, buffers, bind groups, pipeline layouts and pipelines.
    std::array<wgpu::BindGroupLayout, kBindingNum> bgl;
    std::array<wgpu::BindGroup, kBindingNum> bg;
    std::array<wgpu::Buffer, kBindingNum> buffer;
    for (uint32_t i = 0; i < kBindingNum; ++i) {
        bgl[i] = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                      wgpu::BindingType::StorageBuffer}});
        buffer[i] = CreateBuffer(kBufferSize, wgpu::BufferUsage::Storage);
        bg[i] = utils::MakeBindGroup(device, bgl[i], {{0, buffer[i]}});
    }

    wgpu::RenderPipeline renderPipeline = CreateRenderPipeline({bgl[0], bgl[1], bgl[2]});
    wgpu::ComputePipeline computePipeline = CreateComputePipeline({bgl[0], bgl[1], bgl[2]});

    // Compared to pipeline layout, a binding set (bgl2) related bind group is missing
    TestRenderPassBindings(bg.data(), kBindingNum - 1, renderPipeline, false);

    TestComputePassBindings(bg.data(), kBindingNum - 1, computePipeline, false);

    // bgl[2] related bind group is not missing, but its bind group is empty
    bgl[2] = utils::MakeBindGroupLayout(device, {});
    bg[2] = utils::MakeBindGroup(device, bgl[2], {});

    TestRenderPassBindings(bg.data(), kBindingNum, renderPipeline, false);

    TestComputePassBindings(bg.data(), kBindingNum, computePipeline, false);

    // bgl[2] related bind group is neither missing nor empty, but it doesn't contain the necessary
    // binding
    bgl[2] = utils::MakeBindGroupLayout(
        device, {{1, wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
                  wgpu::BindingType::UniformBuffer}});
    buffer[2] = CreateBuffer(kBufferSize, wgpu::BufferUsage::Uniform);
    bg[2] = utils::MakeBindGroup(device, bgl[2], {{1, buffer[2]}});

    TestRenderPassBindings(bg.data(), kBindingNum, renderPipeline, false);

    TestComputePassBindings(bg.data(), kBindingNum, computePipeline, false);
}

class ComparisonSamplerBindingTest : public ValidationTest {
  protected:
    wgpu::RenderPipeline CreateFragmentPipeline(wgpu::BindGroupLayout* bindGroupLayout,
                                                const char* fragmentSource) {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                })");

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, fragmentSource);

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.vertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        wgpu::PipelineLayout pipelineLayout =
            utils::MakeBasicPipelineLayout(device, bindGroupLayout);
        pipelineDescriptor.layout = pipelineLayout;
        return device.CreateRenderPipeline(&pipelineDescriptor);
    }
};

// TODO(crbug.com/dawn/367): Disabled until we can perform shader analysis
// of which samplers are comparison samplers.
TEST_F(ComparisonSamplerBindingTest, DISABLED_ShaderAndBGLMatches) {
    // Test that sampler binding works with normal sampler in the shader.
    {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler}});

        CreateFragmentPipeline(&bindGroupLayout, R"(
        #version 450
        layout(set = 0, binding = 0) uniform sampler samp;

        void main() {
        })");
    }

    // Test that comparison sampler binding works with shadow sampler in the shader.
    {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ComparisonSampler}});

        CreateFragmentPipeline(&bindGroupLayout, R"(
        #version 450
        layout(set = 0, binding = 0) uniform samplerShadow samp;

        void main() {
        })");
    }

    // Test that sampler binding does not work with comparison sampler in the shader.
    {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler}});

        ASSERT_DEVICE_ERROR(CreateFragmentPipeline(&bindGroupLayout, R"(
        #version 450
        layout(set = 0, binding = 0) uniform samplerShadow samp;

        void main() {
        })"));
    }

    // Test that comparison sampler binding does not work with normal sampler in the shader.
    {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ComparisonSampler}});

        ASSERT_DEVICE_ERROR(CreateFragmentPipeline(&bindGroupLayout, R"(
        #version 450
        layout(set = 0, binding = 0) uniform sampler samp;

        void main() {
        })"));
    }
}

TEST_F(ComparisonSamplerBindingTest, SamplerAndBindGroupMatches) {
    // Test that sampler binding works with normal sampler.
    {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler}});

        wgpu::SamplerDescriptor desc = {};
        utils::MakeBindGroup(device, bindGroupLayout, {{0, device.CreateSampler(&desc)}});
    }

    // Test that comparison sampler binding works with sampler w/ compare function.
    {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ComparisonSampler}});

        wgpu::SamplerDescriptor desc = {};
        desc.compare = wgpu::CompareFunction::Never;
        utils::MakeBindGroup(device, bindGroupLayout, {{0, device.CreateSampler(&desc)}});
    }

    // Test that sampler binding does not work with sampler w/ compare function.
    {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::Sampler}});

        wgpu::SamplerDescriptor desc;
        desc.compare = wgpu::CompareFunction::Never;
        ASSERT_DEVICE_ERROR(
            utils::MakeBindGroup(device, bindGroupLayout, {{0, device.CreateSampler(&desc)}}));
    }

    // Test that comparison sampler binding does not work with normal sampler.
    {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ComparisonSampler}});

        wgpu::SamplerDescriptor desc = {};
        ASSERT_DEVICE_ERROR(
            utils::MakeBindGroup(device, bindGroupLayout, {{0, device.CreateSampler(&desc)}}));
    }
}
