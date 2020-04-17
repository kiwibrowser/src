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

#include "utils/DawnHelpers.h"

class CommandBufferValidationTest : public ValidationTest {
};

// Test for an empty command buffer
TEST_F(CommandBufferValidationTest, Empty) {
    device.CreateCommandEncoder().Finish();
}

// Test that a command buffer cannot be ended mid render pass
TEST_F(CommandBufferValidationTest, EndedMidRenderPass) {
    DummyRenderPass dummyRenderPass(device);

    // Control case, command buffer ended after the pass is ended.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
        pass.EndPass();
        encoder.Finish();
    }

    // Error case, command buffer ended mid-pass.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Error case, command buffer ended mid-pass. Trying to use encoders after Finish
    // should fail too.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
        ASSERT_DEVICE_ERROR(encoder.Finish());
        // TODO(cwallez@chromium.org) this should probably be a device error, but currently it
        // produces a encoder error.
        pass.EndPass();
    }
}

// Test that a command buffer cannot be ended mid compute pass
TEST_F(CommandBufferValidationTest, EndedMidComputePass) {
    // Control case, command buffer ended after the pass is ended.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.EndPass();
        encoder.Finish();
    }

    // Error case, command buffer ended mid-pass.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Error case, command buffer ended mid-pass. Trying to use encoders after Finish
    // should fail too.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
        // TODO(cwallez@chromium.org) this should probably be a device error, but currently it
        // produces a encoder error.
        pass.EndPass();
    }
}

// Test that a render pass cannot be ended twice
TEST_F(CommandBufferValidationTest, RenderPassEndedTwice) {
    DummyRenderPass dummyRenderPass(device);

    // Control case, pass is ended once
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
        pass.EndPass();
        encoder.Finish();
    }

    // Error case, pass ended twice
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
        pass.EndPass();
        // TODO(cwallez@chromium.org) this should probably be a device error, but currently it
        // produces a encoder error.
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test that a compute pass cannot be ended twice
TEST_F(CommandBufferValidationTest, ComputePassEndedTwice) {
    // Control case, pass is ended once.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.EndPass();
        encoder.Finish();
    }

    // Error case, pass ended twice
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.EndPass();
        // TODO(cwallez@chromium.org) this should probably be a device error, but currently it
        // produces a encoder error.
        pass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test that beginning a compute pass before ending the previous pass causes an error.
TEST_F(CommandBufferValidationTest, BeginComputePassBeforeEndPreviousPass) {
    DummyRenderPass dummyRenderPass(device);

    // Beginning a compute pass before ending a render pass causes an error.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::RenderPassEncoder renderPass = encoder.BeginRenderPass(&dummyRenderPass);
        dawn::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.EndPass();
        renderPass.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Beginning a compute pass before ending a compute pass causes an error.
    {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder computePass1 = encoder.BeginComputePass();
        dawn::ComputePassEncoder computePass2 = encoder.BeginComputePass();
        computePass2.EndPass();
        computePass1.EndPass();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Test that encoding command after a successful finish produces an error
TEST_F(CommandBufferValidationTest, CallsAfterASuccessfulFinish) {
    // A buffer that can be used in CopyBufferToBuffer
    dawn::BufferDescriptor copyBufferDesc;
    copyBufferDesc.size = 16;
    copyBufferDesc.usage = dawn::BufferUsageBit::TransferSrc | dawn::BufferUsageBit::TransferDst;
    dawn::Buffer copyBuffer = device.CreateBuffer(&copyBufferDesc);

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.Finish();

    ASSERT_DEVICE_ERROR(encoder.CopyBufferToBuffer(copyBuffer, 0, copyBuffer, 0, 0));
}

// Test that encoding command after a failed finish produces an error
TEST_F(CommandBufferValidationTest, CallsAfterAFailedFinish) {
    // A buffer that can be used in CopyBufferToBuffer
    dawn::BufferDescriptor copyBufferDesc;
    copyBufferDesc.size = 16;
    copyBufferDesc.usage = dawn::BufferUsageBit::TransferSrc | dawn::BufferUsageBit::TransferDst;
    dawn::Buffer copyBuffer = device.CreateBuffer(&copyBufferDesc);

    // A buffer that can't be used in CopyBufferToBuffer
    dawn::BufferDescriptor bufferDesc;
    bufferDesc.size = 16;
    bufferDesc.usage = dawn::BufferUsageBit::Uniform;
    dawn::Buffer buffer = device.CreateBuffer(&bufferDesc);

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(buffer, 0, buffer, 0, 0);
    ASSERT_DEVICE_ERROR(encoder.Finish());

    ASSERT_DEVICE_ERROR(encoder.CopyBufferToBuffer(copyBuffer, 0, copyBuffer, 0, 0));
}

// Test that using a single buffer in multiple read usages in the same pass is allowed.
TEST_F(CommandBufferValidationTest, BufferWithMultipleReadUsage) {
    // Create a buffer used as both vertex and index buffer.
    dawn::BufferDescriptor bufferDescriptor;
    bufferDescriptor.usage = dawn::BufferUsageBit::Vertex | dawn::BufferUsageBit::Index;
    bufferDescriptor.size = 4;
    dawn::Buffer buffer = device.CreateBuffer(&bufferDescriptor);

    // Use the buffer as both index and vertex in the same pass
    uint64_t zero = 0;
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    DummyRenderPass dummyRenderPass(device);
    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
    pass.SetIndexBuffer(buffer, 0);
    pass.SetVertexBuffers(0, 1, &buffer, &zero);
    pass.EndPass();
    encoder.Finish();
}

// Test that using the same buffer as both readable and writable in the same pass is disallowed
TEST_F(CommandBufferValidationTest, BufferWithReadAndWriteUsage) {
    // Create a buffer that will be used as an index buffer and as a storage buffer
    dawn::BufferDescriptor bufferDescriptor;
    bufferDescriptor.usage = dawn::BufferUsageBit::Storage | dawn::BufferUsageBit::Index;
    bufferDescriptor.size = 4;
    dawn::Buffer buffer = device.CreateBuffer(&bufferDescriptor);

    // Create the bind group to use the buffer as storage
    dawn::BindGroupLayout bgl = utils::MakeBindGroupLayout(device, {{
        0, dawn::ShaderStageBit::Vertex, dawn::BindingType::StorageBuffer
    }});
    dawn::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer, 0, 4}});

    // Use the buffer as both index and storage in the same pass
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    DummyRenderPass dummyRenderPass(device);
    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
    pass.SetIndexBuffer(buffer, 0);
    pass.SetBindGroup(0, bg, 0, nullptr);
    pass.EndPass();
    ASSERT_DEVICE_ERROR(encoder.Finish());
}

// Test that using the same texture as both readable and writable in the same pass is disallowed
TEST_F(CommandBufferValidationTest, TextureWithReadAndWriteUsage) {
    // Create a texture that will be used both as a sampled texture and a render target
    dawn::TextureDescriptor textureDescriptor;
    textureDescriptor.usage = dawn::TextureUsageBit::Sampled | dawn::TextureUsageBit::OutputAttachment;
    textureDescriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
    textureDescriptor.dimension = dawn::TextureDimension::e2D;
    textureDescriptor.size = {1, 1, 1};
    textureDescriptor.arrayLayerCount = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.mipLevelCount = 1;
    dawn::Texture texture = device.CreateTexture(&textureDescriptor);
    dawn::TextureView view = texture.CreateDefaultView();

    // Create the bind group to use the texture as sampled
    dawn::BindGroupLayout bgl = utils::MakeBindGroupLayout(device, {{
        0, dawn::ShaderStageBit::Vertex, dawn::BindingType::SampledTexture
    }});
    dawn::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}});

    // Create the render pass that will use the texture as an output attachment
    utils::ComboRenderPassDescriptor renderPass({view});

    // Use the texture as both sampeld and output attachment in the same pass
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
    pass.SetBindGroup(0, bg, 0, nullptr);
    pass.EndPass();
    ASSERT_DEVICE_ERROR(encoder.Finish());
}
