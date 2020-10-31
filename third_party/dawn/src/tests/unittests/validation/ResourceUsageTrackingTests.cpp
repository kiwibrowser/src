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

#include "tests/unittests/validation/ValidationTest.h"

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

namespace {

    class ResourceUsageTrackingTest : public ValidationTest {
      protected:
        wgpu::Buffer CreateBuffer(uint64_t size, wgpu::BufferUsage usage) {
            wgpu::BufferDescriptor descriptor;
            descriptor.size = size;
            descriptor.usage = usage;

            return device.CreateBuffer(&descriptor);
        }

        wgpu::Texture CreateTexture(wgpu::TextureUsage usage) {
            wgpu::TextureDescriptor descriptor;
            descriptor.dimension = wgpu::TextureDimension::e2D;
            descriptor.size = {1, 1, 1};
            descriptor.sampleCount = 1;
            descriptor.mipLevelCount = 1;
            descriptor.usage = usage;
            descriptor.format = kFormat;

            return device.CreateTexture(&descriptor);
        }

        // Note that it is valid to bind any bind groups for indices that the pipeline doesn't use.
        // We create a no-op render or compute pipeline without any bindings, and set bind groups
        // in the caller, so it is always correct for binding validation between bind groups and
        // pipeline. But those bind groups in caller can be used for validation for other purposes.
        wgpu::RenderPipeline CreateNoOpRenderPipeline() {
            wgpu::ShaderModule vsModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                })");

            wgpu::ShaderModule fsModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                void main() {
                })");
            utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
            pipelineDescriptor.vertexStage.module = vsModule;
            pipelineDescriptor.cFragmentStage.module = fsModule;
            pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, nullptr);
            return device.CreateRenderPipeline(&pipelineDescriptor);
        }

        wgpu::ComputePipeline CreateNoOpComputePipeline() {
            wgpu::ShaderModule csModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
                #version 450
                void main() {
                })");
            wgpu::ComputePipelineDescriptor pipelineDescriptor;
            pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, nullptr);
            pipelineDescriptor.computeStage.module = csModule;
            pipelineDescriptor.computeStage.entryPoint = "main";
            return device.CreateComputePipeline(&pipelineDescriptor);
        }

        static constexpr wgpu::TextureFormat kFormat = wgpu::TextureFormat::RGBA8Unorm;
    };

    // Test that using a single buffer in multiple read usages in the same pass is allowed.
    TEST_F(ResourceUsageTrackingTest, BufferWithMultipleReadUsage) {
        // Test render pass
        {
            // Create a buffer, and use the buffer as both vertex and index buffer.
            wgpu::Buffer buffer =
                CreateBuffer(4, wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Index);

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetIndexBuffer(buffer);
            pass.SetVertexBuffer(0, buffer);
            pass.EndPass();
            encoder.Finish();
        }

        // Test compute pass
        {
            // Create buffer and bind group
            wgpu::Buffer buffer =
                CreateBuffer(4, wgpu::BufferUsage::Uniform | wgpu::BufferUsage::Storage);

            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer},
                 {1, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer}, {1, buffer}});

            // Use the buffer as both uniform and readonly storage buffer in compute pass.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            encoder.Finish();
        }
    }

    // Test that it is invalid to use the same buffer as both readable and writable in the same
    // render pass. It is invalid in the same dispatch in compute pass.
    TEST_F(ResourceUsageTrackingTest, BufferWithReadAndWriteUsage) {
        // test render pass
        {
            // Create buffer and bind group
            wgpu::Buffer buffer =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);

            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer}});

            // It is invalid to use the buffer as both index and storage in render pass
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetIndexBuffer(buffer);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // test compute pass
        {
            // Create buffer and bind group
            wgpu::Buffer buffer = CreateBuffer(512, wgpu::BufferUsage::Storage);

            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer},
                 {1, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroup bg =
                utils::MakeBindGroup(device, bgl, {{0, buffer, 0, 4}, {1, buffer, 256, 4}});

            // Create a no-op compute pipeline
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // It is valid to use the buffer as both storage and readonly storage in a single
            // compute pass if dispatch command is not called.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetBindGroup(0, bg);
                pass.EndPass();
                encoder.Finish();
            }

            // It is invalid to use the buffer as both storage and readonly storage in a single
            // dispatch.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetPipeline(cp);
                pass.SetBindGroup(0, bg);
                pass.Dispatch(1);
                pass.EndPass();
                // TODO (yunchao.he@intel.com): add buffer usage tracking for compute
                // ASSERT_DEVICE_ERROR(encoder.Finish());
                encoder.Finish();
            }
        }
    }

    // Test using multiple writable usages on the same buffer in a single pass/dispatch
    TEST_F(ResourceUsageTrackingTest, BufferWithMultipleWriteUsage) {
        // Create buffer and bind group
        wgpu::Buffer buffer = CreateBuffer(512, wgpu::BufferUsage::Storage);

        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
                      wgpu::BindingType::StorageBuffer},
                     {1, wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
                      wgpu::BindingType::StorageBuffer}});
        wgpu::BindGroup bg =
            utils::MakeBindGroup(device, bgl, {{0, buffer, 0, 4}, {1, buffer, 256, 4}});

        // test render pass
        {
            // It is valid to use multiple storage usages on the same buffer in render pass
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            encoder.Finish();
        }

        // test compute pass
        {
            // Create a no-op compute pipeline
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // It is valid to use the same buffer as multiple writeable usages in a single compute
            // pass if dispatch command is not called.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetBindGroup(0, bg);
                pass.EndPass();
                encoder.Finish();
            }

            // It is invalid to use the same buffer as multiple writeable usages in a single
            // dispatch
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetPipeline(cp);
                pass.SetBindGroup(0, bg);
                pass.Dispatch(1);
                pass.EndPass();
                // TODO (yunchao.he@intel.com): add buffer usage tracking for compute
                // ASSERT_DEVICE_ERROR(encoder.Finish());
                encoder.Finish();
            }
        }
    }

    // Test that using the same buffer as both readable and writable in different passes is allowed
    TEST_F(ResourceUsageTrackingTest, BufferWithReadAndWriteUsageInDifferentPasses) {
        // Test render pass
        {
            // Create buffers that will be used as index and storage buffers
            wgpu::Buffer buffer0 =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);
            wgpu::Buffer buffer1 =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);

            // Create bind groups to use the buffer as storage
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl, {{0, buffer0}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl, {{0, buffer1}});

            // Use these two buffers as both index and storage in different render passes
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);

            wgpu::RenderPassEncoder pass0 = encoder.BeginRenderPass(&dummyRenderPass);
            pass0.SetIndexBuffer(buffer0);
            pass0.SetBindGroup(0, bg1);
            pass0.EndPass();

            wgpu::RenderPassEncoder pass1 = encoder.BeginRenderPass(&dummyRenderPass);
            pass1.SetIndexBuffer(buffer1);
            pass1.SetBindGroup(0, bg0);
            pass1.EndPass();

            encoder.Finish();
        }

        // Test compute pass
        {
            // Create buffer and bind groups that will be used as storage and uniform bindings
            wgpu::Buffer buffer =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Uniform);

            wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl0, {{0, buffer}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl1, {{0, buffer}});

            // Use the buffer as both storage and uniform in different compute passes
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

            wgpu::ComputePassEncoder pass0 = encoder.BeginComputePass();
            pass0.SetBindGroup(0, bg0);
            pass0.EndPass();

            wgpu::ComputePassEncoder pass1 = encoder.BeginComputePass();
            pass1.SetBindGroup(1, bg1);
            pass1.EndPass();

            encoder.Finish();
        }

        // Test render pass and compute pass mixed together with resource dependency.
        {
            // Create buffer and bind groups that will be used as storage and uniform bindings
            wgpu::Buffer buffer = CreateBuffer(4, wgpu::BufferUsage::Storage);

            wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl0, {{0, buffer}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl1, {{0, buffer}});

            // Use the buffer as storage and uniform in render pass and compute pass respectively
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

            wgpu::ComputePassEncoder pass0 = encoder.BeginComputePass();
            pass0.SetBindGroup(0, bg0);
            pass0.EndPass();

            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass1 = encoder.BeginRenderPass(&dummyRenderPass);
            pass1.SetBindGroup(1, bg1);
            pass1.EndPass();

            encoder.Finish();
        }
    }

    // Test that it is invalid to use the same buffer as both readable and writable in different
    // draws in a single render pass. But it is valid in different dispatches in a single compute
    // pass.
    TEST_F(ResourceUsageTrackingTest, BufferWithReadAndWriteUsageInDifferentDrawsOrDispatches) {
        // Test render pass
        {
            // Create a buffer and a bind group
            wgpu::Buffer buffer =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer}});

            // Create a no-op render pipeline.
            wgpu::RenderPipeline rp = CreateNoOpRenderPipeline();

            // It is not allowed to use the same buffer as both readable and writable in different
            // draws within the same render pass.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetPipeline(rp);

            pass.SetIndexBuffer(buffer);
            pass.Draw(3);

            pass.SetBindGroup(0, bg);
            pass.Draw(3);

            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // test compute pass
        {
            // Create a buffer and bind groups
            wgpu::Buffer buffer = CreateBuffer(4, wgpu::BufferUsage::Storage);

            wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl0, {{0, buffer}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl1, {{0, buffer}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // It is valid to use the same buffer as both readable and writable in different
            // dispatches within the same compute pass.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(cp);

            pass.SetBindGroup(0, bg0);
            pass.Dispatch(1);

            pass.SetBindGroup(0, bg1);
            pass.Dispatch(1);

            pass.EndPass();
            encoder.Finish();
        }
    }

    // Test that it is invalid to use the same buffer as both readable and writable in a single
    // draw or dispatch.
    TEST_F(ResourceUsageTrackingTest, BufferWithReadAndWriteUsageInSingleDrawOrDispatch) {
        // Test render pass
        {
            // Create a buffer and a bind group
            wgpu::Buffer buffer =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, buffer}});

            // Create a no-op render pipeline.
            wgpu::RenderPipeline rp = CreateNoOpRenderPipeline();

            // It is invalid to use the same buffer as both readable and writable usages in a single
            // draw
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetPipeline(rp);

            pass.SetIndexBuffer(buffer);
            pass.SetBindGroup(0, writeBG);
            pass.Draw(3);

            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // test compute pass
        {
            // Create a buffer and bind groups
            wgpu::Buffer buffer = CreateBuffer(4, wgpu::BufferUsage::Storage);

            wgpu::BindGroupLayout readBGL = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup readBG = utils::MakeBindGroup(device, readBGL, {{0, buffer}});
            wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, buffer}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // It is invalid to use the same buffer as both readable and writable usages in a single
            // dispatch
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(cp);

            pass.SetBindGroup(0, readBG);
            pass.SetBindGroup(1, writeBG);
            pass.Dispatch(1);

            pass.EndPass();
            // TODO (yunchao.he@intel.com): add buffer usage tracking for compute
            // ASSERT_DEVICE_ERROR(encoder.Finish());
            encoder.Finish();
        }
    }

    // Test that using the same buffer as copy src/dst and writable/readable usage is allowed.
    TEST_F(ResourceUsageTrackingTest, BufferCopyAndBufferUsageInPass) {
        // Create buffers that will be used as both a copy src/dst buffer and a storage buffer
        wgpu::Buffer bufferSrc =
            CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
        wgpu::Buffer bufferDst =
            CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

        // Create the bind group to use the buffer as storage
        wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});
        wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl0, {{0, bufferSrc}});
        wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer}});
        wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl1, {{0, bufferDst}});

        // Use the buffer as both copy src and storage in render pass
        {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            encoder.CopyBufferToBuffer(bufferSrc, 0, bufferDst, 0, 4);
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetBindGroup(0, bg0);
            pass.EndPass();
            encoder.Finish();
        }

        // Use the buffer as both copy dst and readonly storage in compute pass
        {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            encoder.CopyBufferToBuffer(bufferSrc, 0, bufferDst, 0, 4);
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetBindGroup(0, bg1);
            pass.EndPass();
            encoder.Finish();
        }
    }

    // Test that all index buffers and vertex buffers take effect even though some buffers are
    // not used because they are overwritten by another consecutive call.
    TEST_F(ResourceUsageTrackingTest, BufferWithMultipleSetIndexOrVertexBuffer) {
        // Create buffers that will be used as both vertex and index buffer.
        wgpu::Buffer buffer0 = CreateBuffer(
            4, wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Index | wgpu::BufferUsage::Storage);
        wgpu::Buffer buffer1 =
            CreateBuffer(4, wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Index);

        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});
        wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer0}});

        DummyRenderPass dummyRenderPass(device);

        // Set index buffer twice. The second one overwrites the first one. No buffer is used as
        // both read and write in the same pass. But the overwritten index buffer (buffer0) still
        // take effect during resource tracking.
        {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetIndexBuffer(buffer0);
            pass.SetIndexBuffer(buffer1);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Set index buffer twice. The second one overwrites the first one. buffer0 is used as both
        // read and write in the same pass
        {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetIndexBuffer(buffer1);
            pass.SetIndexBuffer(buffer0);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Set vertex buffer on the same index twice. The second one overwrites the first one. No
        // buffer is used as both read and write in the same pass. But the overwritten vertex buffer
        // (buffer0) still take effect during resource tracking.
        {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetVertexBuffer(0, buffer0);
            pass.SetVertexBuffer(0, buffer1);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Set vertex buffer on the same index twice. The second one overwrites the first one.
        // buffer0 is used as both read and write in the same pass
        {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetVertexBuffer(0, buffer1);
            pass.SetVertexBuffer(0, buffer0);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }

    // Test that all consecutive SetBindGroup()s take effect even though some bind groups are not
    // used because they are overwritten by a consecutive call.
    TEST_F(ResourceUsageTrackingTest, BufferWithMultipleSetBindGroupsOnSameIndex) {
        // test render pass
        {
            // Create buffers that will be used as index and storage buffers
            wgpu::Buffer buffer0 =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);
            wgpu::Buffer buffer1 =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);

            // Create the bind group to use the buffer as storage
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl, {{0, buffer0}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl, {{0, buffer1}});

            DummyRenderPass dummyRenderPass(device);

            // Set bind group on the same index twice. The second one overwrites the first one.
            // No buffer is used as both read and write in the same pass. But the overwritten
            // bind group still take effect during resource tracking.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
                pass.SetIndexBuffer(buffer0);
                pass.SetBindGroup(0, bg0);
                pass.SetBindGroup(0, bg1);
                pass.EndPass();
                ASSERT_DEVICE_ERROR(encoder.Finish());
            }

            // Set bind group on the same index twice. The second one overwrites the first one.
            // buffer0 is used as both read and write in the same pass
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
                pass.SetIndexBuffer(buffer0);
                pass.SetBindGroup(0, bg1);
                pass.SetBindGroup(0, bg0);
                pass.EndPass();
                ASSERT_DEVICE_ERROR(encoder.Finish());
            }
        }

        // test compute pass
        {
            // Create buffers that will be used as index and storage buffers
            wgpu::Buffer buffer0 = CreateBuffer(512, wgpu::BufferUsage::Storage);
            wgpu::Buffer buffer1 = CreateBuffer(4, wgpu::BufferUsage::Storage);

            // Create the bind group to use the buffer as storage
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroupLayout readBGL = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroup writeBG0 = utils::MakeBindGroup(device, writeBGL, {{0, buffer0, 0, 4}});
            wgpu::BindGroup readBG0 = utils::MakeBindGroup(device, readBGL, {{0, buffer0, 256, 4}});
            wgpu::BindGroup readBG1 = utils::MakeBindGroup(device, readBGL, {{0, buffer1, 0, 4}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // Set bind group against the same index twice. The second one overwrites the first one.
            // Then no buffer is used as both read and write in the same pass. But the overwritten
            // bind group still take effect.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetBindGroup(0, writeBG0);
                pass.SetBindGroup(1, readBG0);
                pass.SetBindGroup(1, readBG1);
                pass.SetPipeline(cp);
                pass.Dispatch(1);
                pass.EndPass();
                // TODO (yunchao.he@intel.com): add buffer usage tracking for compute
                // ASSERT_DEVICE_ERROR(encoder.Finish());
                encoder.Finish();
            }

            // Set bind group against the same index twice. The second one overwrites the first one.
            // Then buffer0 is used as both read and write in the same pass
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetBindGroup(0, writeBG0);
                pass.SetBindGroup(1, readBG1);
                pass.SetBindGroup(1, readBG0);
                pass.SetPipeline(cp);
                pass.Dispatch(1);
                pass.EndPass();
                // TODO (yunchao.he@intel.com): add buffer usage tracking for compute
                // ASSERT_DEVICE_ERROR(encoder.Finish());
                encoder.Finish();
            }
        }
    }

    // Test that it is invalid to have resource usage conflicts even when all bindings are not
    // visible to the programmable pass where it is used.
    TEST_F(ResourceUsageTrackingTest, BufferUsageConflictBetweenInvisibleStagesInBindGroup) {
        wgpu::Buffer buffer = CreateBuffer(4, wgpu::BufferUsage::Storage);

        // Test render pass for bind group. The conflict of readonly storage and storage usage
        // doesn't reside in render related stages at all
        {
            // Create a bind group whose bindings are not visible in render pass
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer},
                         {1, wgpu::ShaderStage::None, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer}, {1, buffer}});

            // These two bindings are invisible in render pass. But we still track these bindings.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass for bind group. The conflict of readonly storage and storage usage
        // doesn't reside in compute related stage at all
        {
            // Create a bind group whose bindings are not visible in compute pass
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer},
                         {1, wgpu::ShaderStage::None, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer}, {1, buffer}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // These two bindings are invisible in compute pass. But we still track these bindings.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(cp);
            pass.SetBindGroup(0, bg);
            pass.Dispatch(1);
            pass.EndPass();
            // TODO (yunchao.he@intel.com): add buffer usage tracking for compute
            // ASSERT_DEVICE_ERROR(encoder.Finish());
            encoder.Finish();
        }
    }

    // Test that it is invalid to have resource usage conflicts even when one of the bindings is not
    // visible to the programmable pass where it is used.
    TEST_F(ResourceUsageTrackingTest, BufferUsageConflictWithInvisibleStageInBindGroup) {
        // Test render pass for bind group and index buffer. The conflict of storage and index
        // buffer usage resides between fragment stage and compute stage. But the compute stage
        // binding is not visible in render pass.
        {
            wgpu::Buffer buffer =
                CreateBuffer(4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer}});

            // Buffer usage in compute stage in bind group conflicts with index buffer. And binding
            // for compute stage is not visible in render pass. But we still track this binding.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetIndexBuffer(buffer);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass for bind group. The conflict of readonly storage and storage buffer
        // usage resides between compute stage and fragment stage. But the fragment stage binding is
        // not visible in compute pass.
        {
            wgpu::Buffer buffer = CreateBuffer(4, wgpu::BufferUsage::Storage);
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer},
                         {1, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, buffer}, {1, buffer}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // Buffer usage in compute stage conflicts with buffer usage in fragment stage. And
            // binding for fragment stage is not visible in compute pass. But we still track this
            // invisible binding.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(cp);
            pass.SetBindGroup(0, bg);
            pass.Dispatch(1);
            pass.EndPass();
            // TODO (yunchao.he@intel.com): add buffer usage tracking for compute
            // ASSERT_DEVICE_ERROR(encoder.Finish());
            encoder.Finish();
        }
    }

    // Test that it is invalid to have resource usage conflicts even when one of the bindings is not
    // used in the pipeline.
    TEST_F(ResourceUsageTrackingTest, BufferUsageConflictWithUnusedPipelineBindings) {
        wgpu::Buffer buffer = CreateBuffer(4, wgpu::BufferUsage::Storage);

        // Test render pass for bind groups with unused bindings. The conflict of readonly storage
        // and storage usages resides in different bind groups, although some bindings may not be
        // used because its bind group layout is not designated in pipeline layout.
        {
            // Create bind groups. The bindings are visible for render pass.
            wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl0, {{0, buffer}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl1, {{0, buffer}});

            // Create a passthrough render pipeline with a readonly buffer
            wgpu::ShaderModule vsModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                })");

            wgpu::ShaderModule fsModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(std140, set = 0, binding = 0) readonly buffer RBuffer {
                    readonly float value;
                } rBuffer;
                void main() {
                })");
            utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
            pipelineDescriptor.vertexStage.module = vsModule;
            pipelineDescriptor.cFragmentStage.module = fsModule;
            pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl0);
            wgpu::RenderPipeline rp = device.CreateRenderPipeline(&pipelineDescriptor);

            // Resource in bg1 conflicts with resources used in bg0. However, bindings in bg1 is
            // not used in pipeline. But we still track this binding.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetBindGroup(0, bg0);
            pass.SetBindGroup(1, bg1);
            pass.SetPipeline(rp);
            pass.Draw(3);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass for bind groups with unused bindings. The conflict of readonly storage
        // and storage usages resides in different bind groups, although some bindings may not be
        // used because its bind group layout is not designated in pipeline layout.
        {
            // Create bind groups. The bindings are visible for compute pass.
            wgpu::BindGroupLayout bgl0 = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer}});
            wgpu::BindGroupLayout bgl1 = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl0, {{0, buffer}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl1, {{0, buffer}});

            // Create a passthrough compute pipeline with a readonly buffer
            wgpu::ShaderModule csModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
                #version 450
                layout(std140, set = 0, binding = 0) readonly buffer RBuffer {
                    readonly float value;
                } rBuffer;
                void main() {
                })");
            wgpu::ComputePipelineDescriptor pipelineDescriptor;
            pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl0);
            pipelineDescriptor.computeStage.module = csModule;
            pipelineDescriptor.computeStage.entryPoint = "main";
            wgpu::ComputePipeline cp = device.CreateComputePipeline(&pipelineDescriptor);

            // Resource in bg1 conflicts with resources used in bg0. However, the binding in bg1 is
            // not used in pipeline. But we still track this binding and read/write usage in one
            // dispatch is not allowed.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetBindGroup(0, bg0);
            pass.SetBindGroup(1, bg1);
            pass.SetPipeline(cp);
            pass.Dispatch(1);
            pass.EndPass();
            // TODO (yunchao.he@intel.com): add resource tracking per dispatch for compute pass
            // ASSERT_DEVICE_ERROR(encoder.Finish());
            encoder.Finish();
        }
    }

    // Test that using a single texture in multiple read usages in the same pass is allowed.
    TEST_F(ResourceUsageTrackingTest, TextureWithMultipleReadUsages) {
        // Create a texture that will be used as both sampled and readonly storage texture
        wgpu::Texture texture =
            CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::Storage);
        wgpu::TextureView view = texture.CreateView();

        // Create a bind group to use the texture as sampled and readonly storage bindings
        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
            device,
            {{0, wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
              wgpu::BindingType::SampledTexture},
             {1, wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
              wgpu::BindingType::ReadonlyStorageTexture, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat}});
        wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}, {1, view}});

        // Test render pass
        {
            // Use the texture as both sampled and readonly storage in the same render pass
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            encoder.Finish();
        }

        // Test compute pass
        {
            // Use the texture as both sampled and readonly storage in the same compute pass
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            encoder.Finish();
        }
    }

    // Test that it is invalid to use the same texture as both readable and writable in the same
    // render pass. It is invalid in the same dispatch in compute pass.
    TEST_F(ResourceUsageTrackingTest, TextureWithReadAndWriteUsage) {
        // Test render pass
        {
            // Create a texture
            wgpu::Texture texture =
                CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment);
            wgpu::TextureView view = texture.CreateView();

            // Create a bind group to use the texture as sampled binding
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::SampledTexture}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}});

            // Create a render pass to use the texture as a render target
            utils::ComboRenderPassDescriptor renderPass({view});

            // It is invalid to use the texture as both sampled and render target in the same pass
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass
        {
            // Create a texture
            wgpu::Texture texture =
                CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::Storage);
            wgpu::TextureView view = texture.CreateView();

            // Create a bind group to use the texture as sampled and writeonly bindings
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture},
                         {1, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}, {1, view}});

            // Create a no-op compute pipeline
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // It is valid to use the texture as both sampled and writeonly storage in a single
            // compute pass if dispatch command is not called.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetBindGroup(0, bg);
                pass.EndPass();
                encoder.Finish();
            }

            // It is invalid to use the texture as both sampled and writeonly storage in a single
            // dispatch
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetPipeline(cp);
                pass.SetBindGroup(0, bg);
                pass.Dispatch(1);
                pass.EndPass();
                // TODO (yunchao.he@intel.com): add texture usage tracking for compute
                // ASSERT_DEVICE_ERROR(encoder.Finish());
                encoder.Finish();
            }
        }
    }

    // Test using multiple writable usages on the same texture in a single pass/dispatch
    TEST_F(ResourceUsageTrackingTest, TextureWithMultipleWriteUsage) {
        // Test render pass
        {
            // Create a texture
            wgpu::Texture texture =
                CreateTexture(wgpu::TextureUsage::Storage | wgpu::TextureUsage::OutputAttachment);
            wgpu::TextureView view = texture.CreateView();

            // Create a bind group to use the texture as writeonly storage binding
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::WriteonlyStorageTexture, false,
                  0, false, wgpu::TextureViewDimension::Undefined,
                  wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}});

            // It is invalid to use the texture as both writeonly storage and render target in
            // the same pass
            {
                utils::ComboRenderPassDescriptor renderPass({view});

                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
                pass.SetBindGroup(0, bg);
                pass.EndPass();
                ASSERT_DEVICE_ERROR(encoder.Finish());
            }

            // It is valid to use multiple writeonly storage usages on the same texture in render
            // pass
            {
                wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl, {{0, view}});

                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                DummyRenderPass dummyRenderPass(device);
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
                pass.SetBindGroup(0, bg);
                pass.SetBindGroup(1, bg1);
                pass.EndPass();
                encoder.Finish();
            }
        }

        // Test compute pass
        {
            // Create a texture
            wgpu::Texture texture = CreateTexture(wgpu::TextureUsage::Storage);
            wgpu::TextureView view = texture.CreateView();

            // Create a bind group to use the texture as sampled and writeonly bindings
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat},
                         {1, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}, {1, view}});

            // Create a no-op compute pipeline
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // It is valid to use the texture as multiple writeonly storage usages in a single
            // compute pass if dispatch command is not called.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetBindGroup(0, bg);
                pass.EndPass();
                encoder.Finish();
            }

            // It is invalid to use the texture as multiple writeonly storage usages in a single
            // dispatch
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetPipeline(cp);
                pass.SetBindGroup(0, bg);
                pass.Dispatch(1);
                pass.EndPass();
                // TODO (yunchao.he@intel.com): add texture usage tracking for compute
                // ASSERT_DEVICE_ERROR(encoder.Finish());
                encoder.Finish();
            }
        }
    }

    // Test that using the same texture as both readable and writable in different passes is
    // allowed
    TEST_F(ResourceUsageTrackingTest, TextureWithReadAndWriteUsageInDifferentPasses) {
        // Test render pass
        {
            // Create textures that will be used as both a sampled texture and a render target
            wgpu::Texture t0 =
                CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment);
            wgpu::TextureView v0 = t0.CreateView();
            wgpu::Texture t1 =
                CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment);
            wgpu::TextureView v1 = t1.CreateView();

            // Create bind groups to use the texture as sampled
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::SampledTexture}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl, {{0, v0}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl, {{0, v1}});

            // Create render passes that will use the textures as output attachments
            utils::ComboRenderPassDescriptor renderPass0({v1});
            utils::ComboRenderPassDescriptor renderPass1({v0});

            // Use the textures as both sampled and output attachments in different passes
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

            wgpu::RenderPassEncoder pass0 = encoder.BeginRenderPass(&renderPass0);
            pass0.SetBindGroup(0, bg0);
            pass0.EndPass();

            wgpu::RenderPassEncoder pass1 = encoder.BeginRenderPass(&renderPass1);
            pass1.SetBindGroup(0, bg1);
            pass1.EndPass();

            encoder.Finish();
        }

        // Test compute pass
        {
            // Create a texture that will be used storage texture
            wgpu::Texture texture = CreateTexture(wgpu::TextureUsage::Storage);
            wgpu::TextureView view = texture.CreateView();

            // Create bind groups to use the texture as readonly and writeonly bindings
            wgpu::BindGroupLayout readBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup readBG = utils::MakeBindGroup(device, readBGL, {{0, view}});
            wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, view}});

            // Use the textures as both readonly and writeonly storages in different passes
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

            wgpu::ComputePassEncoder pass0 = encoder.BeginComputePass();
            pass0.SetBindGroup(0, readBG);
            pass0.EndPass();

            wgpu::ComputePassEncoder pass1 = encoder.BeginComputePass();
            pass1.SetBindGroup(0, writeBG);
            pass1.EndPass();

            encoder.Finish();
        }

        // Test compute pass and render pass mixed together with resource dependency
        {
            // Create a texture that will be used a storage texture
            wgpu::Texture texture = CreateTexture(wgpu::TextureUsage::Storage);
            wgpu::TextureView view = texture.CreateView();

            // Create bind groups to use the texture as readonly and writeonly bindings
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroupLayout readBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, view}});
            wgpu::BindGroup readBG = utils::MakeBindGroup(device, readBGL, {{0, view}});

            // Use the texture as writeonly and readonly storage in compute pass and render
            // pass respectively
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

            wgpu::ComputePassEncoder pass0 = encoder.BeginComputePass();
            pass0.SetBindGroup(0, writeBG);
            pass0.EndPass();

            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass1 = encoder.BeginRenderPass(&dummyRenderPass);
            pass1.SetBindGroup(0, readBG);
            pass1.EndPass();

            encoder.Finish();
        }
    }

    // Test that it is invalid to use the same texture as both readable and writable in different
    // draws in a single render pass. But it is valid in different dispatches in a single compute
    // pass.
    TEST_F(ResourceUsageTrackingTest, TextureWithReadAndWriteUsageOnDifferentDrawsOrDispatches) {
        // Create a texture that will be used both as a sampled texture and a storage texture
        wgpu::Texture texture =
            CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::Storage);
        wgpu::TextureView view = texture.CreateView();

        // Test render pass
        {
            // Create bind groups to use the texture as sampled and writeonly storage bindings
            wgpu::BindGroupLayout sampledBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture}});
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::WriteonlyStorageTexture, false,
                  0, false, wgpu::TextureViewDimension::Undefined,
                  wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup sampledBG = utils::MakeBindGroup(device, sampledBGL, {{0, view}});
            wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, view}});

            // Create a no-op render pipeline.
            wgpu::RenderPipeline rp = CreateNoOpRenderPipeline();

            // It is not allowed to use the same texture as both readable and writable in different
            // draws within the same render pass.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetPipeline(rp);

            pass.SetBindGroup(0, sampledBG);
            pass.Draw(3);

            pass.SetBindGroup(0, writeBG);
            pass.Draw(3);

            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass
        {
            // Create bind groups to use the texture as readonly and writeonly storage bindings
            wgpu::BindGroupLayout readBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup readBG = utils::MakeBindGroup(device, readBGL, {{0, view}});
            wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, view}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // It is valid to use the same texture as both readable and writable in different
            // dispatches within the same compute pass.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(cp);

            pass.SetBindGroup(0, readBG);
            pass.Dispatch(1);

            pass.SetBindGroup(0, writeBG);
            pass.Dispatch(1);

            pass.EndPass();
            encoder.Finish();
        }
    }

    // Test that it is invalid to use the same texture as both readable and writable in a single
    // draw or dispatch.
    TEST_F(ResourceUsageTrackingTest, TextureWithReadAndWriteUsageInSingleDrawOrDispatch) {
        // Create a texture that will be used both as a sampled texture and a storage texture
        wgpu::Texture texture =
            CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::Storage);
        wgpu::TextureView view = texture.CreateView();

        // Test render pass
        {
            // Create the bind group to use the texture as sampled and writeonly storage bindings
            wgpu::BindGroupLayout sampledBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture}});
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device,
                {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::WriteonlyStorageTexture, false,
                  0, false, wgpu::TextureViewDimension::Undefined,
                  wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup sampledBG = utils::MakeBindGroup(device, sampledBGL, {{0, view}});
            wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, view}});

            // Create a no-op render pipeline.
            wgpu::RenderPipeline rp = CreateNoOpRenderPipeline();

            // It is invalid to use the same texture as both readable and writable usages in a
            // single draw
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetPipeline(rp);

            pass.SetBindGroup(0, sampledBG);
            pass.SetBindGroup(1, writeBG);
            pass.Draw(3);

            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass
        {
            // Create the bind group to use the texture as readonly and writeonly storage bindings
            wgpu::BindGroupLayout readBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup readBG = utils::MakeBindGroup(device, readBGL, {{0, view}});
            wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, view}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // It is invalid to use the same texture as both readable and writable usages in a
            // single dispatch
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(cp);

            pass.SetBindGroup(0, readBG);
            pass.SetBindGroup(1, writeBG);
            pass.Dispatch(1);

            pass.EndPass();
            // TODO (yunchao.he@intel.com): add texture usage tracking for compute
            // ASSERT_DEVICE_ERROR(encoder.Finish());
            encoder.Finish();
        }
    }

    // Test that using a single texture as copy src/dst and writable/readable usage in pass is
    // allowed.
    TEST_F(ResourceUsageTrackingTest, TextureCopyAndTextureUsageInPass) {
        // Create textures that will be used as both a sampled texture and a render target
        wgpu::Texture texture0 = CreateTexture(wgpu::TextureUsage::CopySrc);
        wgpu::Texture texture1 =
            CreateTexture(wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled |
                          wgpu::TextureUsage::OutputAttachment);
        wgpu::TextureView view0 = texture0.CreateView();
        wgpu::TextureView view1 = texture1.CreateView();

        wgpu::TextureCopyView srcView = utils::CreateTextureCopyView(texture0, 0, {0, 0, 0});
        wgpu::TextureCopyView dstView = utils::CreateTextureCopyView(texture1, 0, {0, 0, 0});
        wgpu::Extent3D copySize = {1, 1, 1};

        // Use the texture as both copy dst and output attachment in render pass
        {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            encoder.CopyTextureToTexture(&srcView, &dstView, &copySize);
            utils::ComboRenderPassDescriptor renderPass({view1});
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
            pass.EndPass();
            encoder.Finish();
        }

        // Use the texture as both copy dst and readable usage in compute pass
        {
            // Create the bind group to use the texture as sampled
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view1}});

            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            encoder.CopyTextureToTexture(&srcView, &dstView, &copySize);
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            encoder.Finish();
        }
    }

    // Test that all consecutive SetBindGroup()s take effect even though some bind groups are not
    // used because they are overwritten by a consecutive call.
    TEST_F(ResourceUsageTrackingTest, TextureWithMultipleSetBindGroupsOnSameIndex) {
        // Test render pass
        {
            // Create textures that will be used as both a sampled texture and a render target
            wgpu::Texture texture0 =
                CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment);
            wgpu::TextureView view0 = texture0.CreateView();
            wgpu::Texture texture1 =
                CreateTexture(wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment);
            wgpu::TextureView view1 = texture1.CreateView();

            // Create the bind group to use the texture as sampled
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::SampledTexture}});
            wgpu::BindGroup bg0 = utils::MakeBindGroup(device, bgl, {{0, view0}});
            wgpu::BindGroup bg1 = utils::MakeBindGroup(device, bgl, {{0, view1}});

            // Create the render pass that will use the texture as an output attachment
            utils::ComboRenderPassDescriptor renderPass({view0});

            // Set bind group on the same index twice. The second one overwrites the first one.
            // No texture is used as both sampled and output attachment in the same pass. But the
            // overwritten texture still take effect during resource tracking.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
                pass.SetBindGroup(0, bg0);
                pass.SetBindGroup(0, bg1);
                pass.EndPass();
                ASSERT_DEVICE_ERROR(encoder.Finish());
            }

            // Set bind group on the same index twice. The second one overwrites the first one.
            // texture0 is used as both sampled and output attachment in the same pass
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
                pass.SetBindGroup(0, bg1);
                pass.SetBindGroup(0, bg0);
                pass.EndPass();
                ASSERT_DEVICE_ERROR(encoder.Finish());
            }
        }

        // Test compute pass
        {
            // Create a texture that will be used both as storage texture
            wgpu::Texture texture0 = CreateTexture(wgpu::TextureUsage::Storage);
            wgpu::TextureView view0 = texture0.CreateView();
            wgpu::Texture texture1 = CreateTexture(wgpu::TextureUsage::Storage);
            wgpu::TextureView view1 = texture1.CreateView();

            // Create the bind group to use the texture as readonly and writeonly bindings
            wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});

            wgpu::BindGroupLayout readBGL = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});

            wgpu::BindGroup writeBG0 = utils::MakeBindGroup(device, writeBGL, {{0, view0}});
            wgpu::BindGroup readBG0 = utils::MakeBindGroup(device, readBGL, {{0, view0}});
            wgpu::BindGroup readBG1 = utils::MakeBindGroup(device, readBGL, {{0, view1}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // Set bind group on the same index twice. The second one overwrites the first one.
            // No texture is used as both readonly and writeonly storage in the same pass. But the
            // overwritten texture still take effect during resource tracking.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetBindGroup(0, writeBG0);
                pass.SetBindGroup(1, readBG0);
                pass.SetBindGroup(1, readBG1);
                pass.SetPipeline(cp);
                pass.Dispatch(1);
                pass.EndPass();
                // TODO (yunchao.he@intel.com): add texture usage tracking for compute
                // ASSERT_DEVICE_ERROR(encoder.Finish());
                encoder.Finish();
            }

            // Set bind group on the same index twice. The second one overwrites the first one.
            // texture0 is used as both writeonly and readonly storage in the same pass.
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                pass.SetBindGroup(0, writeBG0);
                pass.SetBindGroup(1, readBG1);
                pass.SetBindGroup(1, readBG0);
                pass.SetPipeline(cp);
                pass.Dispatch(1);
                pass.EndPass();
                // TODO (yunchao.he@intel.com): add texture usage tracking for compute
                // ASSERT_DEVICE_ERROR(encoder.Finish());
                encoder.Finish();
            }
        }
    }

    // Test that it is invalid to have resource usage conflicts even when all bindings are not
    // visible to the programmable pass where it is used.
    TEST_F(ResourceUsageTrackingTest, TextureUsageConflictBetweenInvisibleStagesInBindGroup) {
        // Create texture and texture view
        wgpu::Texture texture = CreateTexture(wgpu::TextureUsage::Storage);
        wgpu::TextureView view = texture.CreateView();

        // Test render pass for bind group. The conflict of readonly storage and writeonly storage
        // usage doesn't reside in render related stages at all
        {
            // Create a bind group whose bindings are not visible in render pass
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat},
                         {1, wgpu::ShaderStage::None, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}, {1, view}});

            // These two bindings are invisible in render pass. But we still track these bindings.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass for bind group. The conflict of readonly storage and writeonly storage
        // usage doesn't reside in compute related stage at all
        {
            // Create a bind group whose bindings are not visible in compute pass
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat},
                         {1, wgpu::ShaderStage::None, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}, {1, view}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // These two bindings are invisible in compute pass. But we still track these bindings.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(cp);
            pass.SetBindGroup(0, bg);
            pass.Dispatch(1);
            pass.EndPass();
            // TODO (yunchao.he@intel.com): add texture usage tracking for compute
            // ASSERT_DEVICE_ERROR(encoder.Finish());
            encoder.Finish();
        }
    }

    // Test that it is invalid to have resource usage conflicts even when one of the bindings is not
    // visible to the programmable pass where it is used.
    TEST_F(ResourceUsageTrackingTest, TextureUsageConflictWithInvisibleStageInBindGroup) {
        // Create texture and texture view
        wgpu::Texture texture =
            CreateTexture(wgpu::TextureUsage::Storage | wgpu::TextureUsage::OutputAttachment);
        wgpu::TextureView view = texture.CreateView();

        // Test render pass
        {
            // Create the render pass that will use the texture as an output attachment
            utils::ComboRenderPassDescriptor renderPass({view});

            // Create a bind group which use the texture as readonly storage in compute stage
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}});

            // Texture usage in compute stage in bind group conflicts with render target. And
            // binding for compute stage is not visible in render pass. But we still track this
            // binding.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
            pass.SetBindGroup(0, bg);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass
        {
            // Create a bind group which contains both fragment and compute stages
            wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ReadonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat},
                         {1, wgpu::ShaderStage::Compute, wgpu::BindingType::WriteonlyStorageTexture,
                          false, 0, false, wgpu::TextureViewDimension::Undefined,
                          wgpu::TextureComponentType::Float, kFormat}});
            wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, view}, {1, view}});

            // Create a no-op compute pipeline.
            wgpu::ComputePipeline cp = CreateNoOpComputePipeline();

            // Texture usage in compute stage conflicts with texture usage in fragment stage. And
            // binding for fragment stage is not visible in compute pass. But we still track this
            // invisible binding.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(cp);
            pass.SetBindGroup(0, bg);
            pass.Dispatch(1);
            pass.EndPass();
            // TODO (yunchao.he@intel.com): add texture usage tracking for compute
            // ASSERT_DEVICE_ERROR(encoder.Finish());
            encoder.Finish();
        }
    }

    // Test that it is invalid to have resource usage conflicts even when one of the bindings is not
    // used in the pipeline.
    TEST_F(ResourceUsageTrackingTest, TextureUsageConflictWithUnusedPipelineBindings) {
        // Create texture and texture view
        wgpu::Texture texture = CreateTexture(wgpu::TextureUsage::Storage);
        wgpu::TextureView view = texture.CreateView();

        // Create bind groups.
        wgpu::BindGroupLayout readBGL = utils::MakeBindGroupLayout(
            device,
            {{0, wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
              wgpu::BindingType::ReadonlyStorageTexture, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat}});
        wgpu::BindGroupLayout writeBGL = utils::MakeBindGroupLayout(
            device,
            {{0, wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
              wgpu::BindingType::WriteonlyStorageTexture, false, 0, false,
              wgpu::TextureViewDimension::Undefined, wgpu::TextureComponentType::Float, kFormat}});
        wgpu::BindGroup readBG = utils::MakeBindGroup(device, readBGL, {{0, view}});
        wgpu::BindGroup writeBG = utils::MakeBindGroup(device, writeBGL, {{0, view}});

        // Test render pass
        {
            // Create a passthrough render pipeline with a readonly storage texture
            wgpu::ShaderModule vsModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                })");

            wgpu::ShaderModule fsModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(set = 0, binding = 0, rgba8) uniform readonly image2D image;
                void main() {
                })");
            utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
            pipelineDescriptor.vertexStage.module = vsModule;
            pipelineDescriptor.cFragmentStage.module = fsModule;
            pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, &readBGL);
            wgpu::RenderPipeline rp = device.CreateRenderPipeline(&pipelineDescriptor);

            // Texture binding in readBG conflicts with texture binding in writeBG. The binding
            // in writeBG is not used in pipeline. But we still track this binding.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            DummyRenderPass dummyRenderPass(device);
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&dummyRenderPass);
            pass.SetBindGroup(0, readBG);
            pass.SetBindGroup(1, writeBG);
            pass.SetPipeline(rp);
            pass.Draw(3);
            pass.EndPass();
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        // Test compute pass
        {
            // Create a passthrough compute pipeline with a readonly storage texture
            wgpu::ShaderModule csModule =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
                #version 450
                layout(set = 0, binding = 0, rgba8) uniform readonly image2D image;
                void main() {
                })");
            wgpu::ComputePipelineDescriptor pipelineDescriptor;
            pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, &readBGL);
            pipelineDescriptor.computeStage.module = csModule;
            pipelineDescriptor.computeStage.entryPoint = "main";
            wgpu::ComputePipeline cp = device.CreateComputePipeline(&pipelineDescriptor);

            // Texture binding in readBG conflicts with texture binding in writeBG. The binding
            // in writeBG is not used in pipeline. But we still track this binding.
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetBindGroup(0, readBG);
            pass.SetBindGroup(1, writeBG);
            pass.SetPipeline(cp);
            pass.Dispatch(1);
            pass.EndPass();
            // TODO (yunchao.he@intel.com): add resource tracking per dispatch for compute pass
            // ASSERT_DEVICE_ERROR(encoder.Finish());
            encoder.Finish();
        }
    }

    // TODO (yunchao.he@intel.com):
    //
    //	* Add tests for multiple encoders upon the same resource simultaneously. This situation fits
    //	some cases like VR, multi-threading, etc.
    //
    //	* Add tests for indirect buffer
    //
    //	* Add tests for bundle

}  // anonymous namespace
