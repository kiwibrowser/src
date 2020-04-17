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

#include "common/Constants.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

class BindGroupValidationTest : public ValidationTest {
  public:
    void SetUp() override {
        // Create objects to use as resources inside test bind groups.
        {
            dawn::BufferDescriptor descriptor;
            descriptor.size = 1024;
            descriptor.usage = dawn::BufferUsageBit::Uniform;
            mUBO = device.CreateBuffer(&descriptor);
        }
        {
            dawn::BufferDescriptor descriptor;
            descriptor.size = 1024;
            descriptor.usage = dawn::BufferUsageBit::Storage;
            mSSBO = device.CreateBuffer(&descriptor);
        }
        {
            dawn::SamplerDescriptor descriptor = utils::GetDefaultSamplerDescriptor();
            mSampler = device.CreateSampler(&descriptor);
        }
        {
            dawn::TextureDescriptor descriptor;
            descriptor.dimension = dawn::TextureDimension::e2D;
            descriptor.size = {16, 16, 1};
            descriptor.arrayLayerCount = 1;
            descriptor.sampleCount = 1;
            descriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
            descriptor.mipLevelCount = 1;
            descriptor.usage = dawn::TextureUsageBit::Sampled;
            mSampledTexture = device.CreateTexture(&descriptor);
            mSampledTextureView = mSampledTexture.CreateDefaultView();
        }
    }

  protected:
    dawn::Buffer mUBO;
    dawn::Buffer mSSBO;
    dawn::Sampler mSampler;
    dawn::Texture mSampledTexture;
    dawn::TextureView mSampledTextureView;
};

// Test the validation of BindGroupDescriptor::nextInChain
TEST_F(BindGroupValidationTest, NextInChainNullptr) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {});

    dawn::BindGroupDescriptor descriptor;
    descriptor.layout = layout;
    descriptor.bindingCount = 0;
    descriptor.bindings = nullptr;

    // Control case: check that nextInChain = nullptr is valid
    descriptor.nextInChain = nullptr;
    device.CreateBindGroup(&descriptor);

    // Check that nextInChain != nullptr is an error.
    descriptor.nextInChain = static_cast<void*>(&descriptor);
    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
}

// Check constraints on bindingCount
TEST_F(BindGroupValidationTest, bindingCountMismatch) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::Sampler}
    });

    // Control case: check that a descriptor with one binding is ok
    utils::MakeBindGroup(device, layout, {{0, mSampler}});

    // Check that bindingCount != layout.bindingCount fails.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {}));
}

// Check constraints on BindGroupBinding::binding
TEST_F(BindGroupValidationTest, WrongBindings) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::Sampler}
    });

    // Control case: check that a descriptor with a binding matching the layout's is ok
    utils::MakeBindGroup(device, layout, {{0, mSampler}});

    // Check that binding must be present in the layout
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{1, mSampler}}));

    // Check that binding >= kMaxBindingsPerGroup fails.
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{kMaxBindingsPerGroup, mSampler}}));
}

// Check that the same binding cannot be set twice
TEST_F(BindGroupValidationTest, BindingSetTwice) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::Sampler},
        {1, dawn::ShaderStageBit::Fragment, dawn::BindingType::Sampler}
    });

    // Control case: check that different bindings work
    utils::MakeBindGroup(device, layout, {
        {0, mSampler},
        {1, mSampler}
    });

    // Check that setting the same binding twice is invalid
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {
        {0, mSampler},
        {0, mSampler}
    }));
}

// Check that a sampler binding must contain exactly one sampler
TEST_F(BindGroupValidationTest, SamplerBindingType) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::Sampler}
    });

    dawn::BindGroupBinding binding;
    binding.binding = 0;
    binding.sampler = nullptr;
    binding.textureView = nullptr;
    binding.buffer = nullptr;
    binding.offset = 0;
    binding.size = 0;

    dawn::BindGroupDescriptor descriptor;
    descriptor.layout = layout;
    descriptor.bindingCount = 1;
    descriptor.bindings = &binding;

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
        dawn::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
        samplerDesc.minFilter = static_cast<dawn::FilterMode>(0xFFFFFFFF);

        dawn::Sampler errorSampler;
        ASSERT_DEVICE_ERROR(errorSampler = device.CreateSampler(&samplerDesc));

        binding.sampler = errorSampler;
        ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
        binding.sampler = nullptr;
    }
}

// Check that a texture binding must contain exactly a texture view
TEST_F(BindGroupValidationTest, TextureBindingType) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::SampledTexture}
    });

    dawn::BindGroupBinding binding;
    binding.binding = 0;
    binding.sampler = nullptr;
    binding.textureView = nullptr;
    binding.buffer = nullptr;
    binding.offset = 0;
    binding.size = 0;

    dawn::BindGroupDescriptor descriptor;
    descriptor.layout = layout;
    descriptor.bindingCount = 1;
    descriptor.bindings = &binding;

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
        dawn::TextureViewDescriptor viewDesc;
        viewDesc.format = dawn::TextureFormat::R8G8B8A8Unorm;
        viewDesc.dimension = dawn::TextureViewDimension::e2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 0;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 0;

        dawn::TextureView errorView;
        ASSERT_DEVICE_ERROR(errorView = mSampledTexture.CreateView(&viewDesc));

        binding.textureView = errorView;
        ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
        binding.textureView = nullptr;
    }
}

// Check that a buffer binding must contain exactly a buffer
TEST_F(BindGroupValidationTest, BufferBindingType) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::UniformBuffer}
    });

    dawn::BindGroupBinding binding;
    binding.binding = 0;
    binding.sampler = nullptr;
    binding.textureView = nullptr;
    binding.buffer = nullptr;
    binding.offset = 0;
    binding.size = 0;

    dawn::BindGroupDescriptor descriptor;
    descriptor.layout = layout;
    descriptor.bindingCount = 1;
    descriptor.bindings = &binding;

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
        dawn::BufferDescriptor bufferDesc;
        bufferDesc.size = 1024;
        bufferDesc.usage = static_cast<dawn::BufferUsageBit>(0xFFFFFFFF);

        dawn::Buffer errorBuffer;
        ASSERT_DEVICE_ERROR(errorBuffer = device.CreateBuffer(&bufferDesc));

        binding.buffer = errorBuffer;
        ASSERT_DEVICE_ERROR(device.CreateBindGroup(&descriptor));
        binding.buffer = nullptr;
    }
}

// Check that a texture must have the correct usage
TEST_F(BindGroupValidationTest, TextureUsage) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::SampledTexture}
    });

    // Control case: setting a sampleable texture view works.
    utils::MakeBindGroup(device, layout, {{0, mSampledTextureView}});

    // Make an output attachment texture and try to set it for a SampledTexture binding
    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dawn::TextureDimension::e2D;
    descriptor.size = {16, 16, 1};
    descriptor.arrayLayerCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = dawn::TextureUsageBit::OutputAttachment;
    dawn::Texture outputTexture = device.CreateTexture(&descriptor);
    dawn::TextureView outputTextureView = outputTexture.CreateDefaultView();
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, outputTextureView}}));
}

// Check that a UBO must have the correct usage
TEST_F(BindGroupValidationTest, BufferUsageUBO) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::UniformBuffer}
    });

    // Control case: using a buffer with the uniform usage works
    utils::MakeBindGroup(device, layout, {{0, mUBO, 0, 256}});

    // Using a buffer without the uniform usage fails
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mSSBO, 0, 256}}));
}

// Check that a SSBO must have the correct usage
TEST_F(BindGroupValidationTest, BufferUsageSSBO) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::StorageBuffer}
    });

    // Control case: using a buffer with the storage usage works
    utils::MakeBindGroup(device, layout, {{0, mSSBO, 0, 256}});

    // Using a buffer without the storage usage fails
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, mUBO, 0, 256}}));
}

// Tests constraints on the buffer offset for bind groups.
TEST_F(BindGroupValidationTest, BufferOffsetAlignment) {
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
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
    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
    });

    dawn::BufferDescriptor descriptor;
    descriptor.size = 1024;
    descriptor.usage = dawn::BufferUsageBit::Uniform;
    dawn::Buffer buffer = device.CreateBuffer(&descriptor);

    // Success case, touching the start of the buffer works
    utils::MakeBindGroup(device, layout, {{0, buffer, 0, 256}});

    // Success case, touching the end of the buffer works
    utils::MakeBindGroup(device, layout, {{0, buffer, 3*256, 256}});
    utils::MakeBindGroup(device, layout, {{0, buffer, 1024, 0}});

    // Success case, touching the full buffer works
    utils::MakeBindGroup(device, layout, {{0, buffer, 0, 1024}});

    // Error case, offset is OOB
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, buffer, 256*5, 0}}));

    // Error case, size is OOB
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, buffer, 0, 256*5}}));

    // Error case, offset+size is OOB
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, buffer, 1024, 1}}));

    // Error case, offset+size overflows to be 0
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, layout, {{0, buffer, 256, uint32_t(0) - uint32_t(256)}}));
}

// Test what happens when the layout is an error.
TEST_F(BindGroupValidationTest, ErrorLayout) {
    dawn::BindGroupLayout goodLayout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
    });

    dawn::BindGroupLayout errorLayout;
    ASSERT_DEVICE_ERROR(errorLayout = utils::MakeBindGroupLayout(device, {
        {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
        {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
    }));

    // Control case, creating with the good layout works
    utils::MakeBindGroup(device, goodLayout, {{0, mUBO, 0, 256}});

    // Control case, creating with the good layout works
    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(device, errorLayout, {{0, mUBO, 0, 256}}));
}

class BindGroupLayoutValidationTest : public ValidationTest {
};

// Tests setting OOB checks for kMaxBindingsPerGroup in bind group layouts.
TEST_F(BindGroupLayoutValidationTest, BindGroupLayoutBindingOOB) {
    // Checks that kMaxBindingsPerGroup - 1 is valid.
    utils::MakeBindGroupLayout(device, {
        {kMaxBindingsPerGroup - 1, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer}
    });

    // Checks that kMaxBindingsPerGroup is OOB
    ASSERT_DEVICE_ERROR(utils::MakeBindGroupLayout(device, {
        {kMaxBindingsPerGroup, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer}
    }));
}

// This test verifies that the BindGroupLayout bindings are correctly validated, even if the
// binding ids are out-of-order.
TEST_F(BindGroupLayoutValidationTest, BindGroupBinding) {
    auto layout = utils::MakeBindGroupLayout(
        device, {
                    {1, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
                    {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
                });
}


// This test verifies that the BindGroupLayout cache is successfully caching/deduplicating objects.
//
// NOTE: This test only works currently because unittests are run without the wire - so the returned
// BindGroupLayout pointers are actually visibly equivalent. With the wire, this would not be true.
TEST_F(BindGroupLayoutValidationTest, BindGroupLayoutCache) {
    auto layout1 = utils::MakeBindGroupLayout(
        device, {
                    {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
                });
    auto layout2 = utils::MakeBindGroupLayout(
        device, {
                    {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
                });

    // Caching should cause these to be the same.
    ASSERT_EQ(layout1.Get(), layout2.Get());
}

constexpr uint32_t kBufferElementsCount = kMinDynamicBufferOffsetAlignment / sizeof(uint32_t) + 2;
constexpr uint32_t kBufferSize = kBufferElementsCount * sizeof(uint32_t);

class SetBindGroupValidationTest : public ValidationTest {
  public:
    void SetUp() override {
        std::array<float, kBufferElementsCount> uniformData = {0};

        uniformData[0] = 1.0;
        uniformData[1] = 2.0;
        uniformData[uniformData.size() - 2] = 5.0;
        uniformData[uniformData.size() - 1] = 6.0;

        dawn::BufferDescriptor bufferDescriptor;
        bufferDescriptor.size = kBufferSize;
        bufferDescriptor.usage = dawn::BufferUsageBit::Storage;

        mUniformBuffer = utils::CreateBufferFromData(device, uniformData.data(), kBufferSize,
                                                     dawn::BufferUsageBit::Uniform);
        mStorageBuffer = device.CreateBuffer(&bufferDescriptor);

        mBindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, dawn::ShaderStageBit::Compute | dawn::ShaderStageBit::Fragment,
                      dawn::BindingType::DynamicUniformBuffer},
                     {1, dawn::ShaderStageBit::Compute | dawn::ShaderStageBit::Fragment,
                      dawn::BindingType::DynamicStorageBuffer}});

        mBindGroup = utils::MakeBindGroup(
            device, mBindGroupLayout,
            {{0, mUniformBuffer, 0, kBufferSize}, {1, mStorageBuffer, 0, kBufferSize}});
    }
    // Create objects to use as resources inside test bind groups.

    dawn::BindGroup mBindGroup;
    dawn::BindGroupLayout mBindGroupLayout;
    dawn::Buffer mUniformBuffer;
    dawn::Buffer mStorageBuffer;

    dawn::RenderPipeline CreateRenderPipeline() {
        dawn::ShaderModule vsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
                #version 450
                void main() {
                })");

        dawn::ShaderModule fsModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(std140, set = 0, binding = 0) uniform uBuffer {
                     vec2 value1;
                };
                layout(std140, set = 0, binding = 1) buffer SBuffer {
                     vec2 value2;
                } sBuffer;
                layout(location = 0) out uvec4 fragColor;
                void main() {
                })");

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.cVertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        dawn::PipelineLayout pipelineLayout =
            utils::MakeBasicPipelineLayout(device, &mBindGroupLayout);
        pipelineDescriptor.layout = pipelineLayout;
        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    dawn::ComputePipeline CreateComputePipeline() {
        dawn::ShaderModule csModule =
            utils::CreateShaderModule(device, dawn::ShaderStage::Compute, R"(
                #version 450
                const uint kTileSize = 4;
                const uint kInstances = 11;

                layout(local_size_x = kTileSize, local_size_y = kTileSize, local_size_z = 1) in;
                layout(std140, set = 0, binding = 0) uniform UniformBuffer {
                    float value1;
                };
                layout(std140, set = 0, binding = 1) buffer SBuffer {
                    float value2;
                } dst;

        void main() {
        })");

        dawn::ComputePipelineDescriptor csDesc;
        dawn::PipelineLayout pipelineLayout =
            utils::MakeBasicPipelineLayout(device, &mBindGroupLayout);
        csDesc.layout = pipelineLayout;

        dawn::PipelineStageDescriptor computeStage;
        computeStage.module = csModule;
        computeStage.entryPoint = "main";
        csDesc.computeStage = &computeStage;

        return device.CreateComputePipeline(&csDesc);
    }
};

// This is the test case that should work.
TEST_F(SetBindGroupValidationTest, Basic) {
    std::array<uint64_t, 2> offsets = {256, 0};

    // RenderPipeline SetBindGroup
    {
        dawn::RenderPipeline renderPipeline = CreateRenderPipeline();
        DummyRenderPass renderPass(device);

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.SetPipeline(renderPipeline);
        renderPassEncoder.SetBindGroup(0, mBindGroup, 2, offsets.data());
        renderPassEncoder.Draw(3, 1, 0, 0);
        renderPassEncoder.EndPass();
        commandEncoder.Finish();
    }

    {
        dawn::ComputePipeline computePipeline = CreateComputePipeline();

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetPipeline(computePipeline);
        computePassEncoder.SetBindGroup(0, mBindGroup, 2, offsets.data());
        computePassEncoder.Dispatch(1, 1, 1);
        computePassEncoder.EndPass();
        commandEncoder.Finish();
    }
}

// Test cases that test dynamic offsets count mismatch with bind group layout.
TEST_F(SetBindGroupValidationTest, DynamicOffsetsMismatch) {
    std::array<uint64_t, 1> mismatchOffsets = {0};

    // RenderPipeline SetBindGroup
    {
        dawn::RenderPipeline pipeline = CreateRenderPipeline();
        DummyRenderPass renderPass(device);

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.SetPipeline(pipeline);
        renderPassEncoder.SetBindGroup(0, mBindGroup, 1, mismatchOffsets.data());
        renderPassEncoder.Draw(3, 1, 0, 0);
        renderPassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }

    {
        dawn::ComputePipeline computePipeline = CreateComputePipeline();

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetPipeline(computePipeline);
        computePassEncoder.SetBindGroup(0, mBindGroup, 1, mismatchOffsets.data());
        computePassEncoder.Dispatch(1, 1, 1);
        computePassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
}

// Test cases that test dynamic offsets not aligned
TEST_F(SetBindGroupValidationTest, DynamicOffsetsNotAligned) {
    std::array<uint64_t, 2> notAlignedOffsets = {1, 2};

    // RenderPipeline SetBindGroup
    {
        dawn::RenderPipeline pipeline = CreateRenderPipeline();
        DummyRenderPass renderPass(device);

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.SetPipeline(pipeline);
        renderPassEncoder.SetBindGroup(0, mBindGroup, 2, notAlignedOffsets.data());
        renderPassEncoder.Draw(3, 1, 0, 0);
        renderPassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }

    // ComputePipeline SetBindGroup
    {
        dawn::ComputePipeline pipeline = CreateComputePipeline();

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetPipeline(pipeline);
        computePassEncoder.SetBindGroup(0, mBindGroup, 2, notAlignedOffsets.data());
        computePassEncoder.Dispatch(1, 1, 1);
        computePassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
}

// Test cases that test dynamic uniform buffer out of bound situation.
TEST_F(SetBindGroupValidationTest, OutOfBoundDynamicUniformBuffer) {
    std::array<uint64_t, 2> overFlowOffsets = {512, 0};

    // RenderPipeline SetBindGroup
    {
        dawn::RenderPipeline pipeline = CreateRenderPipeline();
        DummyRenderPass renderPass(device);

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.SetPipeline(pipeline);
        renderPassEncoder.SetBindGroup(0, mBindGroup, 2, overFlowOffsets.data());
        renderPassEncoder.Draw(3, 1, 0, 0);
        renderPassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }

    // ComputePipeline SetBindGroup
    {
        dawn::ComputePipeline pipeline = CreateComputePipeline();

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetPipeline(pipeline);
        computePassEncoder.SetBindGroup(0, mBindGroup, 2, overFlowOffsets.data());
        computePassEncoder.Dispatch(1, 1, 1);
        computePassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
}

// Test cases that test dynamic storage buffer out of bound situation.
TEST_F(SetBindGroupValidationTest, OutOfBoundDynamicStorageBuffer) {
    std::array<uint64_t, 2> overFlowOffsets = {0, 512};

    // RenderPipeline SetBindGroup
    {
        dawn::RenderPipeline pipeline = CreateRenderPipeline();
        DummyRenderPass renderPass(device);

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.SetPipeline(pipeline);
        renderPassEncoder.SetBindGroup(0, mBindGroup, 2, overFlowOffsets.data());
        renderPassEncoder.Draw(3, 1, 0, 0);
        renderPassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }

    // ComputePipeline SetBindGroup
    {
        dawn::ComputePipeline pipeline = CreateComputePipeline();

        dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
        computePassEncoder.SetPipeline(pipeline);
        computePassEncoder.SetBindGroup(0, mBindGroup, 2, overFlowOffsets.data());
        computePassEncoder.Dispatch(1, 1, 1);
        computePassEncoder.EndPass();
        ASSERT_DEVICE_ERROR(commandEncoder.Finish());
    }
}
