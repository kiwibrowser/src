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

#include "common/Assert.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

class MultisampledRenderingTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        InitTexturesForTest();
    }

    void InitTexturesForTest() {
        mMultisampledColorView =
            CreateTextureForOutputAttachment(kColorFormat, kSampleCount).CreateDefaultView();
        mResolveTexture = CreateTextureForOutputAttachment(kColorFormat, 1);
        mResolveView = mResolveTexture.CreateDefaultView();

        mDepthStencilTexture = CreateTextureForOutputAttachment(kDepthStencilFormat, kSampleCount);
        mDepthStencilView = mDepthStencilTexture.CreateDefaultView();
    }

    dawn::RenderPipeline CreateRenderPipelineWithOneOutputForTest(bool testDepth) {
        const char* kFsOneOutputWithDepth =
            R"(#version 450
            layout(location = 0) out vec4 fragColor;
            layout (std140, set = 0, binding = 0) uniform uBuffer {
                vec4 color;
                float depth;
            };
            void main() {
                fragColor = color;
                gl_FragDepth = depth;
            })";

        const char* kFsOneOutputWithoutDepth =
            R"(#version 450
            layout(location = 0) out vec4 fragColor;
            layout (std140, set = 0, binding = 0) uniform uBuffer {
                vec4 color;
            };
            void main() {
                fragColor = color;
            })";

        const char* fs = testDepth ? kFsOneOutputWithDepth : kFsOneOutputWithoutDepth;


        return CreateRenderPipelineForTest(fs, 1, testDepth);
    }

    dawn::RenderPipeline CreateRenderPipelineWithTwoOutputsForTest() {
        const char* kFsTwoOutputs =
            R"(#version 450
            layout(location = 0) out vec4 fragColor1;
            layout(location = 1) out vec4 fragColor2;
            layout (std140, set = 0, binding = 0) uniform uBuffer {
                vec4 color1;
                vec4 color2;
            };
            void main() {
                fragColor1 = color1;
                fragColor2 = color2;
            })";

        return CreateRenderPipelineForTest(kFsTwoOutputs, 2, false);
    }

    dawn::Texture CreateTextureForOutputAttachment(dawn::TextureFormat format,
                                                   uint32_t sampleCount,
                                                   uint32_t mipLevelCount = 1,
                                                   uint32_t arrayLayerCount = 1) {
        dawn::TextureDescriptor descriptor;
        descriptor.dimension = dawn::TextureDimension::e2D;
        descriptor.size.width = kWidth << (mipLevelCount - 1);
        descriptor.size.height = kHeight << (mipLevelCount - 1);
        descriptor.size.depth = 1;
        descriptor.arrayLayerCount = arrayLayerCount;
        descriptor.sampleCount = sampleCount;
        descriptor.format = format;
        descriptor.mipLevelCount = mipLevelCount;
        descriptor.usage =
            dawn::TextureUsageBit::OutputAttachment | dawn::TextureUsageBit::TransferSrc;
        return device.CreateTexture(&descriptor);
    }

    void EncodeRenderPassForTest(dawn::CommandEncoder commandEncoder,
                                 const dawn::RenderPassDescriptor& renderPass,
                                 const dawn::RenderPipeline& pipeline,
                                 const float* uniformData,
                                 uint32_t uniformDataSize) {
        dawn::Buffer uniformBuffer =
            utils::CreateBufferFromData(device, uniformData, uniformDataSize,
                                        dawn::BufferUsageBit::Uniform);
        dawn::BindGroup bindGroup =
            utils::MakeBindGroup(device, mBindGroupLayout,
                                 {{0, uniformBuffer, 0, uniformDataSize}});

        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.SetPipeline(pipeline);
        renderPassEncoder.SetBindGroup(0, bindGroup, 0, nullptr);
        renderPassEncoder.Draw(3, 1, 0, 0);
        renderPassEncoder.EndPass();
    }

    utils::ComboRenderPassDescriptor CreateComboRenderPassDescriptorForTest(
        std::initializer_list<dawn::TextureView> colorViews,
        std::initializer_list<dawn::TextureView> resolveTargetViews,
        dawn::LoadOp colorLoadOp,
        dawn::LoadOp depthStencilLoadOp,
        bool hasDepthStencilAttachment) {
        ASSERT(colorViews.size() == resolveTargetViews.size());

        constexpr dawn::Color kClearColor = {0.0f, 0.0f, 0.0f, 0.0f};
        constexpr float kClearDepth = 1.0f;

        utils::ComboRenderPassDescriptor renderPass(colorViews);
        uint32_t i = 0;
        for (const dawn::TextureView& resolveTargetView : resolveTargetViews) {
            renderPass.cColorAttachmentsInfoPtr[i]->loadOp = colorLoadOp;
            renderPass.cColorAttachmentsInfoPtr[i]->clearColor = kClearColor;
            renderPass.cColorAttachmentsInfoPtr[i]->resolveTarget = resolveTargetView;
            ++i;
        }

        renderPass.cDepthStencilAttachmentInfo.clearDepth = kClearDepth;
        renderPass.cDepthStencilAttachmentInfo.depthLoadOp = depthStencilLoadOp;

        if (hasDepthStencilAttachment) {
            renderPass.cDepthStencilAttachmentInfo.attachment = mDepthStencilView;
            renderPass.depthStencilAttachment = &renderPass.cDepthStencilAttachmentInfo;
        }

        return renderPass;
    }

    void VerifyResolveTarget(const dawn::Color& inputColor,
                             dawn::Texture resolveTexture,
                             uint32_t mipmapLevel = 0,
                             uint32_t arrayLayer = 0) {
        constexpr float kMSAACoverage = 0.5f;

        // In this test we only check the pixel in the middle of the texture.
        constexpr uint32_t kMiddleX = (kWidth - 1) / 2;
        constexpr uint32_t kMiddleY = (kHeight - 1) / 2;

        RGBA8 expectedColor;
        expectedColor.r = static_cast<uint8_t>(0xFF * inputColor.r * kMSAACoverage);
        expectedColor.g = static_cast<uint8_t>(0xFF * inputColor.g * kMSAACoverage);
        expectedColor.b = static_cast<uint8_t>(0xFF * inputColor.b * kMSAACoverage);
        expectedColor.a = static_cast<uint8_t>(0xFF * inputColor.a * kMSAACoverage);

        EXPECT_TEXTURE_RGBA8_EQ(&expectedColor, resolveTexture, kMiddleX, kMiddleY, 1, 1,
                                mipmapLevel, arrayLayer);
    }

    constexpr static uint32_t kWidth = 3;
    constexpr static uint32_t kHeight = 3;
    constexpr static uint32_t kSampleCount = 4;
    constexpr static dawn::TextureFormat kColorFormat = dawn::TextureFormat::R8G8B8A8Unorm;
    constexpr static dawn::TextureFormat kDepthStencilFormat = dawn::TextureFormat::D32FloatS8Uint;

    dawn::TextureView mMultisampledColorView;
    dawn::Texture mResolveTexture;
    dawn::TextureView mResolveView;
    dawn::Texture mDepthStencilTexture;
    dawn::TextureView mDepthStencilView;
    dawn::BindGroupLayout mBindGroupLayout;

  private:
    dawn::RenderPipeline CreateRenderPipelineForTest(const char* fs,
                                                     uint32_t numColorAttachments,
                                                     bool hasDepthStencilAttachment) {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);

        // Draw a bottom-right triangle. In standard 4xMSAA pattern, for the pixels on diagonal,
        // only two of the samples will be touched.
        const char* vs =
            R"(#version 450
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(1.f, -1.f));
            void main() {
                gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
            })";
        pipelineDescriptor.cVertexStage.module =
            utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, vs);

        pipelineDescriptor.cFragmentStage.module =
            utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, fs);


        mBindGroupLayout = utils::MakeBindGroupLayout(
            device, {
                {0, dawn::ShaderStageBit::Fragment, dawn::BindingType::UniformBuffer},
            });
        dawn::PipelineLayout pipelineLayout =
            utils::MakeBasicPipelineLayout(device, &mBindGroupLayout);
        pipelineDescriptor.layout = pipelineLayout;

        if (hasDepthStencilAttachment) {
            pipelineDescriptor.cDepthStencilState.format = kDepthStencilFormat;
            pipelineDescriptor.cDepthStencilState.depthWriteEnabled = true;
            pipelineDescriptor.cDepthStencilState.depthCompare = dawn::CompareFunction::Less;
            pipelineDescriptor.depthStencilState = &pipelineDescriptor.cDepthStencilState;
        }

        pipelineDescriptor.sampleCount = kSampleCount;

        pipelineDescriptor.colorStateCount = numColorAttachments;
        for (uint32_t i = 0; i < numColorAttachments; ++i) {
            pipelineDescriptor.cColorStates[i]->format = kColorFormat;
        }

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }
};

// Test using one multisampled color attachment with resolve target can render correctly.
TEST_P(MultisampledRenderingTest, ResolveInto2DTexture) {
    constexpr bool kTestDepth = false;
    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::RenderPipeline pipeline = CreateRenderPipelineWithOneOutputForTest(kTestDepth);

    constexpr dawn::Color kGreen = {0.0f, 0.8f, 0.0f, 0.8f};
    constexpr uint32_t kSize = sizeof(kGreen);

    // Draw a green triangle.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView}, {mResolveView}, dawn::LoadOp::Clear, dawn::LoadOp::Clear,
            kTestDepth);

        EncodeRenderPassForTest(commandEncoder, renderPass, pipeline, &kGreen.r, kSize);
    }

    dawn::CommandBuffer commandBuffer = commandEncoder.Finish();
    dawn::Queue queue = device.CreateQueue();
    queue.Submit(1, &commandBuffer);

    VerifyResolveTarget(kGreen, mResolveTexture);
}

// Test multisampled rendering with depth test works correctly.
TEST_P(MultisampledRenderingTest, MultisampledRenderingWithDepthTest) {
    constexpr bool kTestDepth = true;
    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::RenderPipeline pipeline = CreateRenderPipelineWithOneOutputForTest(kTestDepth);

    constexpr dawn::Color kGreen = {0.0f, 0.8f, 0.0f, 0.8f};
    constexpr dawn::Color kRed = {0.8f, 0.0f, 0.0f, 0.8f};

    // In first render pass we draw a green triangle with depth value == 0.2f.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView}, {mResolveView}, dawn::LoadOp::Clear, dawn::LoadOp::Clear,
            true);
        std::array<float, 5> kUniformData = {kGreen.r, kGreen.g, kGreen.b, kGreen.a, // Color
                                             0.2f};                                  // depth
        constexpr uint32_t kSize = sizeof(kUniformData);
        EncodeRenderPassForTest(commandEncoder, renderPass, pipeline, kUniformData.data(), kSize);
    }

    // In second render pass we draw a red triangle with depth value == 0.5f.
    // This red triangle should not be displayed because it is behind the green one that is drawn in
    // the last render pass.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView}, {mResolveView}, dawn::LoadOp::Load, dawn::LoadOp::Load,
            kTestDepth);

        std::array<float, 8> kUniformData = {kRed.r, kRed.g, kRed.b, kRed.a, // color
                                             0.5f};                          // depth
        constexpr uint32_t kSize = sizeof(kUniformData);
        EncodeRenderPassForTest(commandEncoder, renderPass, pipeline, kUniformData.data(), kSize);
    }

    dawn::CommandBuffer commandBuffer = commandEncoder.Finish();
    dawn::Queue queue = device.CreateQueue();
    queue.Submit(1, &commandBuffer);

    // The color of the pixel in the middle of mResolveTexture should be green if MSAA resolve runs
    // correctly with depth test.
    VerifyResolveTarget(kGreen, mResolveTexture);
}

// Test rendering into a multisampled color attachment and doing MSAA resolve in another render pass
// works correctly.
TEST_P(MultisampledRenderingTest, ResolveInAnotherRenderPass) {
    constexpr bool kTestDepth = false;
    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::RenderPipeline pipeline = CreateRenderPipelineWithOneOutputForTest(kTestDepth);

    constexpr dawn::Color kGreen = {0.0f, 0.8f, 0.0f, 0.8f};
    constexpr uint32_t kSize = sizeof(kGreen);

    // In first render pass we draw a green triangle and do not set the resolve target.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView}, {nullptr}, dawn::LoadOp::Clear, dawn::LoadOp::Clear,
            kTestDepth);

        EncodeRenderPassForTest(commandEncoder, renderPass, pipeline, &kGreen.r, kSize);
    }

    // In second render pass we ony do MSAA resolve with no draw call.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView}, {mResolveView}, dawn::LoadOp::Load, dawn::LoadOp::Load,
            kTestDepth);

        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.EndPass();
    }

    dawn::CommandBuffer commandBuffer = commandEncoder.Finish();
    dawn::Queue queue = device.CreateQueue();
    queue.Submit(1, &commandBuffer);

    VerifyResolveTarget(kGreen, mResolveTexture);
}

// Test doing MSAA resolve into multiple resolve targets works correctly.
TEST_P(MultisampledRenderingTest, ResolveIntoMultipleResolveTargets) {
    dawn::TextureView multisampledColorView2 =
        CreateTextureForOutputAttachment(kColorFormat, kSampleCount).CreateDefaultView();
    dawn::Texture resolveTexture2 = CreateTextureForOutputAttachment(kColorFormat, 1);
    dawn::TextureView resolveView2 = resolveTexture2.CreateDefaultView();

    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::RenderPipeline pipeline = CreateRenderPipelineWithTwoOutputsForTest();

    constexpr dawn::Color kGreen = {0.0f, 0.8f, 0.0f, 0.8f};
    constexpr dawn::Color kRed = {0.8f, 0.0f, 0.0f, 0.8f};
    constexpr bool kTestDepth = false;

    // Draw a red triangle to the first color attachment, and a blue triangle to the second color
    // attachment, and do MSAA resolve on two render targets in one render pass.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView, multisampledColorView2}, {mResolveView, resolveView2},
            dawn::LoadOp::Clear, dawn::LoadOp::Clear, kTestDepth);

        std::array<float, 8> kUniformData = {kRed.r, kRed.g, kRed.b, kRed.a,          // color1
                                             kGreen.r, kGreen.g, kGreen.b, kGreen.a}; // color2
        constexpr uint32_t kSize = sizeof(kUniformData);
        EncodeRenderPassForTest(commandEncoder, renderPass, pipeline, kUniformData.data(), kSize);
    }

    dawn::CommandBuffer commandBuffer = commandEncoder.Finish();
    dawn::Queue queue = device.CreateQueue();
    queue.Submit(1, &commandBuffer);

    VerifyResolveTarget(kRed, mResolveTexture);
    VerifyResolveTarget(kGreen, resolveTexture2);
}

// Test doing MSAA resolve on one multisampled texture twice works correctly.
TEST_P(MultisampledRenderingTest, ResolveOneMultisampledTextureTwice) {
    constexpr bool kTestDepth = false;
    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::RenderPipeline pipeline = CreateRenderPipelineWithOneOutputForTest(kTestDepth);

    constexpr dawn::Color kGreen = {0.0f, 0.8f, 0.0f, 0.8f};
    constexpr uint32_t kSize = sizeof(kGreen);

    dawn::Texture resolveTexture2 = CreateTextureForOutputAttachment(kColorFormat, 1);

    // In first render pass we draw a green triangle and specify mResolveView as the resolve target.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView}, {mResolveView}, dawn::LoadOp::Clear, dawn::LoadOp::Clear,
            kTestDepth);

        EncodeRenderPassForTest(commandEncoder, renderPass, pipeline, &kGreen.r, kSize);
    }

    // In second render pass we do MSAA resolve into resolveTexture2.
    {
        dawn::TextureView resolveView2 = resolveTexture2.CreateDefaultView();
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView}, {resolveView2}, dawn::LoadOp::Load, dawn::LoadOp::Load,
            kTestDepth);

        dawn::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
        renderPassEncoder.EndPass();
    }

    dawn::CommandBuffer commandBuffer = commandEncoder.Finish();
    dawn::Queue queue = device.CreateQueue();
    queue.Submit(1, &commandBuffer);

    VerifyResolveTarget(kGreen, mResolveTexture);
    VerifyResolveTarget(kGreen, resolveTexture2);
}

// Test using a layer of a 2D texture as resolve target works correctly.
TEST_P(MultisampledRenderingTest, ResolveIntoOneMipmapLevelOf2DTexture) {
    constexpr uint32_t kBaseMipLevel = 2;

    dawn::TextureViewDescriptor textureViewDescriptor;
    textureViewDescriptor.dimension = dawn::TextureViewDimension::e2D;
    textureViewDescriptor.format = kColorFormat;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseMipLevel = kBaseMipLevel;

    dawn::Texture resolveTexture =
        CreateTextureForOutputAttachment(kColorFormat, 1, kBaseMipLevel + 1, 1);
    dawn::TextureView resolveView = resolveTexture.CreateView(&textureViewDescriptor);

    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    constexpr dawn::Color kGreen = {0.0f, 0.8f, 0.0f, 0.8f};
    constexpr uint32_t kSize = sizeof(kGreen);
    constexpr bool kTestDepth = false;

    // Draw a green triangle and do MSAA resolve.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView}, {resolveView}, dawn::LoadOp::Clear, dawn::LoadOp::Clear,
            kTestDepth);
        dawn::RenderPipeline pipeline = CreateRenderPipelineWithOneOutputForTest(kTestDepth);

        EncodeRenderPassForTest(commandEncoder, renderPass, pipeline, &kGreen.r, kSize);
    }

    dawn::CommandBuffer commandBuffer = commandEncoder.Finish();
    dawn::Queue queue = device.CreateQueue();
    queue.Submit(1, &commandBuffer);

    VerifyResolveTarget(kGreen, resolveTexture, kBaseMipLevel, 0);
}

// Test using a level or a layer of a 2D array texture as resolve target works correctly.
TEST_P(MultisampledRenderingTest, ResolveInto2DArrayTexture) {
    dawn::TextureView multisampledColorView2 =
        CreateTextureForOutputAttachment(kColorFormat, kSampleCount).CreateDefaultView();

    dawn::TextureViewDescriptor baseTextureViewDescriptor;
    baseTextureViewDescriptor.dimension = dawn::TextureViewDimension::e2D;
    baseTextureViewDescriptor.format = kColorFormat;
    baseTextureViewDescriptor.arrayLayerCount = 1;
    baseTextureViewDescriptor.mipLevelCount = 1;

    // Create resolveTexture1 with only 1 mipmap level.
    constexpr uint32_t kBaseArrayLayer1 = 2;
    constexpr uint32_t kBaseMipLevel1 = 0;
    dawn::Texture resolveTexture1 =
        CreateTextureForOutputAttachment(kColorFormat, 1, kBaseMipLevel1 + 1, kBaseArrayLayer1 + 1);
    dawn::TextureViewDescriptor resolveViewDescriptor1 = baseTextureViewDescriptor;
    resolveViewDescriptor1.baseArrayLayer = kBaseArrayLayer1;
    resolveViewDescriptor1.baseMipLevel = kBaseMipLevel1;
    dawn::TextureView resolveView1 = resolveTexture1.CreateView(&resolveViewDescriptor1);

    // Create resolveTexture2 with (kBaseMipLevel2 + 1) mipmap levels and resolve into its last
    // mipmap level.
    constexpr uint32_t kBaseArrayLayer2 = 5;
    constexpr uint32_t kBaseMipLevel2 = 3;
    dawn::Texture resolveTexture2 =
        CreateTextureForOutputAttachment(kColorFormat, 1, kBaseMipLevel2 + 1, kBaseArrayLayer2 + 1);
    dawn::TextureViewDescriptor resolveViewDescriptor2 = baseTextureViewDescriptor;
    resolveViewDescriptor2.baseArrayLayer = kBaseArrayLayer2;
    resolveViewDescriptor2.baseMipLevel = kBaseMipLevel2;
    dawn::TextureView resolveView2 = resolveTexture2.CreateView(&resolveViewDescriptor2);

    dawn::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    dawn::RenderPipeline pipeline = CreateRenderPipelineWithTwoOutputsForTest();

    constexpr dawn::Color kGreen = {0.0f, 0.8f, 0.0f, 0.8f};
    constexpr dawn::Color kRed = {0.8f, 0.0f, 0.0f, 0.8f};
    constexpr bool kTestDepth = false;

    // Draw a red triangle to the first color attachment, and a green triangle to the second color
    // attachment, and do MSAA resolve on two render targets in one render pass.
    {
        utils::ComboRenderPassDescriptor renderPass = CreateComboRenderPassDescriptorForTest(
            {mMultisampledColorView, multisampledColorView2}, {resolveView1, resolveView2},
            dawn::LoadOp::Clear, dawn::LoadOp::Clear, kTestDepth);

        std::array<float, 8> kUniformData = {kRed.r, kRed.g, kRed.b, kRed.a,          // color1
                                             kGreen.r, kGreen.g, kGreen.b, kGreen.a}; // color2
        constexpr uint32_t kSize = sizeof(kUniformData);
        EncodeRenderPassForTest(commandEncoder, renderPass, pipeline, kUniformData.data(), kSize);
    }

    dawn::CommandBuffer commandBuffer = commandEncoder.Finish();
    dawn::Queue queue = device.CreateQueue();
    queue.Submit(1, &commandBuffer);

    VerifyResolveTarget(kRed, resolveTexture1, kBaseMipLevel1, kBaseArrayLayer1);
    VerifyResolveTarget(kGreen, resolveTexture2, kBaseMipLevel2, kBaseArrayLayer2);
}

DAWN_INSTANTIATE_TEST(MultisampledRenderingTest,
                      D3D12Backend,
                      MetalBackend,
                      OpenGLBackend,
                      VulkanBackend,
                      ForceWorkarounds(MetalBackend, {"emulate_store_and_msaa_resolve"}),
                      ForceWorkarounds(MetalBackend, {"always_resolve_into_zero_level_and_layer"}),
                      ForceWorkarounds(MetalBackend,
                                       {"always_resolve_into_zero_level_and_layer",
                                        "emulate_store_and_msaa_resolve"}));
