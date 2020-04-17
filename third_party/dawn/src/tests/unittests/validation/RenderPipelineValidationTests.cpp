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

class RenderPipelineValidationTest : public ValidationTest {
    protected:
        void SetUp() override {
            ValidationTest::SetUp();

            vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
                #version 450
                void main() {
                    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
                })"
            );

            fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                })");
        }

        dawn::ShaderModule vsModule;
        dawn::ShaderModule fsModule;
};

// Test cases where creation should succeed
TEST_F(RenderPipelineValidationTest, CreationSuccess) {
    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.cVertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    device.CreateRenderPipeline(&descriptor);
}

TEST_F(RenderPipelineValidationTest, ColorState) {
    {
        // This one succeeds because attachment 0 is the color attachment
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.cVertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.colorStateCount = 1;

        device.CreateRenderPipeline(&descriptor);
    }

    {  // Fail because lack of color states (and depth/stencil state)
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.cVertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.colorStateCount = 0;

        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }
}

/// Tests that the sample count of the render pipeline must be valid.
TEST_F(RenderPipelineValidationTest, SampleCount) {
    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.cVertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.sampleCount = 4;

        device.CreateRenderPipeline(&descriptor);
    }

    {
        utils::ComboRenderPipelineDescriptor descriptor(device);
        descriptor.cVertexStage.module = vsModule;
        descriptor.cFragmentStage.module = fsModule;
        descriptor.sampleCount = 3;

        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&descriptor));
    }
}

// Tests that the sample count of the render pipeline must be equal to the one of every attachments
// in the render pass.
TEST_F(RenderPipelineValidationTest, SampleCountCompatibilityWithRenderPass) {
    constexpr uint32_t kMultisampledCount = 4;
    constexpr dawn::TextureFormat kColorFormat = dawn::TextureFormat::R8G8B8A8Unorm;
    constexpr dawn::TextureFormat kDepthStencilFormat = dawn::TextureFormat::D32FloatS8Uint;

    dawn::TextureDescriptor baseTextureDescriptor;
    baseTextureDescriptor.size.width = 4;
    baseTextureDescriptor.size.height = 4;
    baseTextureDescriptor.size.depth = 1;
    baseTextureDescriptor.arrayLayerCount = 1;
    baseTextureDescriptor.mipLevelCount = 1;
    baseTextureDescriptor.dimension = dawn::TextureDimension::e2D;
    baseTextureDescriptor.usage = dawn::TextureUsageBit::OutputAttachment;

    utils::ComboRenderPipelineDescriptor nonMultisampledPipelineDescriptor(device);
    nonMultisampledPipelineDescriptor.sampleCount = 1;
    nonMultisampledPipelineDescriptor.cVertexStage.module = vsModule;
    nonMultisampledPipelineDescriptor.cFragmentStage.module = fsModule;
    dawn::RenderPipeline nonMultisampledPipeline =
        device.CreateRenderPipeline(&nonMultisampledPipelineDescriptor);

    nonMultisampledPipelineDescriptor.colorStateCount = 0;
    nonMultisampledPipelineDescriptor.depthStencilState =
        &nonMultisampledPipelineDescriptor.cDepthStencilState;
    dawn::RenderPipeline nonMultisampledPipelineWithDepthStencilOnly =
        device.CreateRenderPipeline(&nonMultisampledPipelineDescriptor);

    utils::ComboRenderPipelineDescriptor multisampledPipelineDescriptor(device);
    multisampledPipelineDescriptor.sampleCount = kMultisampledCount;
    multisampledPipelineDescriptor.cVertexStage.module = vsModule;
    multisampledPipelineDescriptor.cFragmentStage.module = fsModule;
    dawn::RenderPipeline multisampledPipeline =
        device.CreateRenderPipeline(&multisampledPipelineDescriptor);

    multisampledPipelineDescriptor.colorStateCount = 0;
    multisampledPipelineDescriptor.depthStencilState =
        &multisampledPipelineDescriptor.cDepthStencilState;
    dawn::RenderPipeline multisampledPipelineWithDepthStencilOnly =
        device.CreateRenderPipeline(&multisampledPipelineDescriptor);

    // It is not allowed to use multisampled render pass and non-multisampled render pipeline.
    {
        {
            dawn::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.format = kColorFormat;
            textureDescriptor.sampleCount = kMultisampledCount;
            dawn::Texture multisampledColorTexture = device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {multisampledColorTexture.CreateDefaultView()});

            dawn::CommandEncoder encoder = device.CreateCommandEncoder();
            dawn::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(nonMultisampledPipeline);
            renderPass.EndPass();

            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        {
            dawn::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.sampleCount = kMultisampledCount;
            textureDescriptor.format = kDepthStencilFormat;
            dawn::Texture multisampledDepthStencilTexture =
                device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {}, multisampledDepthStencilTexture.CreateDefaultView());

            dawn::CommandEncoder encoder = device.CreateCommandEncoder();
            dawn::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(nonMultisampledPipelineWithDepthStencilOnly);
            renderPass.EndPass();

            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }

    // It is allowed to use multisampled render pass and multisampled render pipeline.
    {
        {
            dawn::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.format = kColorFormat;
            textureDescriptor.sampleCount = kMultisampledCount;
            dawn::Texture multisampledColorTexture = device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {multisampledColorTexture.CreateDefaultView()});

            dawn::CommandEncoder encoder = device.CreateCommandEncoder();
            dawn::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(multisampledPipeline);
            renderPass.EndPass();

            encoder.Finish();
        }

        {
            dawn::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.sampleCount = kMultisampledCount;
            textureDescriptor.format = kDepthStencilFormat;
            dawn::Texture multisampledDepthStencilTexture =
                device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {}, multisampledDepthStencilTexture.CreateDefaultView());

            dawn::CommandEncoder encoder = device.CreateCommandEncoder();
            dawn::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(multisampledPipelineWithDepthStencilOnly);
            renderPass.EndPass();

            encoder.Finish();
        }
    }

    // It is not allowed to use non-multisampled render pass and multisampled render pipeline.
    {
        {
            dawn::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.format = kColorFormat;
            textureDescriptor.sampleCount = 1;
            dawn::Texture nonMultisampledColorTexture = device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor nonMultisampledRenderPassDescriptor(
                { nonMultisampledColorTexture.CreateDefaultView() });

            dawn::CommandEncoder encoder = device.CreateCommandEncoder();
            dawn::RenderPassEncoder renderPass =
                encoder.BeginRenderPass(&nonMultisampledRenderPassDescriptor);
            renderPass.SetPipeline(multisampledPipeline);
            renderPass.EndPass();

            ASSERT_DEVICE_ERROR(encoder.Finish());
        }

        {
            dawn::TextureDescriptor textureDescriptor = baseTextureDescriptor;
            textureDescriptor.sampleCount = 1;
            textureDescriptor.format = kDepthStencilFormat;
            dawn::Texture multisampledDepthStencilTexture =
                device.CreateTexture(&textureDescriptor);
            utils::ComboRenderPassDescriptor renderPassDescriptor(
                {}, multisampledDepthStencilTexture.CreateDefaultView());

            dawn::CommandEncoder encoder = device.CreateCommandEncoder();
            dawn::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
            renderPass.SetPipeline(multisampledPipelineWithDepthStencilOnly);
            renderPass.EndPass();

            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }
}
