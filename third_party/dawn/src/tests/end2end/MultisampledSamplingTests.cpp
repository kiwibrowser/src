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

#include "common/Math.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

namespace {
    // https://github.com/gpuweb/gpuweb/issues/108
    // Vulkan, Metal, and D3D11 have the same standard multisample pattern. D3D12 is the same as
    // D3D11 but it was left out of the documentation.
    // {0.375, 0.125}, {0.875, 0.375}, {0.125 0.625}, {0.625, 0.875}
    // In this test, we store them in -1 to 1 space because it makes it
    // simpler to upload vertex data. Y is flipped because there is a flip between clip space and
    // rasterization space.
    static constexpr std::array<std::array<float, 2>, 4> kSamplePositions = {
        {{0.375 * 2 - 1, 1 - 0.125 * 2},
         {0.875 * 2 - 1, 1 - 0.375 * 2},
         {0.125 * 2 - 1, 1 - 0.625 * 2},
         {0.625 * 2 - 1, 1 - 0.875 * 2}}};
}  // anonymous namespace

class MultisampledSamplingTest : public DawnTest {
  protected:
    static constexpr wgpu::TextureFormat kColorFormat = wgpu::TextureFormat::R8Unorm;
    static constexpr wgpu::TextureFormat kDepthFormat = wgpu::TextureFormat::Depth32Float;

    static constexpr wgpu::TextureFormat kDepthOutFormat = wgpu::TextureFormat::R32Float;
    static constexpr uint32_t kSampleCount = 4;

    // Render pipeline for drawing to a multisampled color and depth attachment.
    wgpu::RenderPipeline drawPipeline;

    // A compute pipeline to texelFetch the sample locations and output the results to a buffer.
    wgpu::ComputePipeline checkSamplePipeline;

    void SetUp() override {
        DawnTest::SetUp();
        {
            utils::ComboRenderPipelineDescriptor desc(device);

            desc.vertexStage.module =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex,
                                          R"(#version 450
                layout(location=0) in vec2 pos;
                void main() {
                    gl_Position = vec4(pos, 0.0, 1.0);
                })");

            desc.cFragmentStage.module =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment,
                                          R"(#version 450
                layout(location = 0) out float fragColor;
                void main() {
                    fragColor = 1.0;
                    gl_FragDepth = 0.7;
                })");

            desc.cVertexState.vertexBufferCount = 1;
            desc.cVertexState.cVertexBuffers[0].attributeCount = 1;
            desc.cVertexState.cVertexBuffers[0].arrayStride = 2 * sizeof(float);
            desc.cVertexState.cAttributes[0].format = wgpu::VertexFormat::Float2;

            desc.cDepthStencilState.format = kDepthFormat;
            desc.cDepthStencilState.depthWriteEnabled = true;
            desc.depthStencilState = &desc.cDepthStencilState;

            desc.sampleCount = kSampleCount;
            desc.colorStateCount = 1;
            desc.cColorStates[0].format = kColorFormat;

            desc.primitiveTopology = wgpu::PrimitiveTopology::TriangleStrip;

            drawPipeline = device.CreateRenderPipeline(&desc);
        }
        {
            wgpu::ComputePipelineDescriptor desc = {};
            desc.computeStage.entryPoint = "main";
            desc.computeStage.module =
                utils::CreateShaderModule(device, utils::SingleShaderStage::Compute,
                                          R"(#version 450
                layout(set = 0, binding = 0) uniform sampler sampler0;
                layout(set = 0, binding = 1) uniform texture2DMS texture0;
                layout(set = 0, binding = 2) uniform texture2DMS texture1;

                layout(set = 0, binding = 3, std430) buffer Results {
                    float colorSamples[4];
                    float depthSamples[4];
                };

                void main() {
                    for (int i = 0; i < 4; ++i) {
                        colorSamples[i] =
                            texelFetch(sampler2DMS(texture0, sampler0), ivec2(0, 0), i).x;

                        depthSamples[i] =
                            texelFetch(sampler2DMS(texture1, sampler0), ivec2(0, 0), i).x;
                    }
                })");

            checkSamplePipeline = device.CreateComputePipeline(&desc);
        }
    }
};

// Test that the multisampling sample positions are correct. This test works by drawing a
// thin quad multiple times from left to right and from top to bottom on a 1x1 canvas.
// Each time, the quad should cover a single sample position.
// After drawing, a compute shader fetches all of the samples (both color and depth),
// and we check that only the one covered has data.
// We "scan" the vertical and horizontal dimensions separately to check that the triangle
// must cover both the X and Y coordinates of the sample position (no false positives if
// it covers the X position but not the Y, or vice versa).
TEST_P(MultisampledSamplingTest, SamplePositions) {
    static constexpr wgpu::Extent3D kTextureSize = {1, 1, 1};

    wgpu::Texture colorTexture;
    {
        wgpu::TextureDescriptor desc = {};
        desc.usage = wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment;
        desc.size = kTextureSize;
        desc.format = kColorFormat;
        desc.sampleCount = kSampleCount;
        colorTexture = device.CreateTexture(&desc);
    }

    wgpu::Texture depthTexture;
    {
        wgpu::TextureDescriptor desc = {};
        desc.usage = wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment;
        desc.size = kTextureSize;
        desc.format = kDepthFormat;
        desc.sampleCount = kSampleCount;
        depthTexture = device.CreateTexture(&desc);
    }

    static constexpr float kQuadWidth = 0.075;
    std::vector<float> vBufferData;

    // Add vertices for vertical quads
    for (uint32_t s = 0; s < kSampleCount; ++s) {
        // clang-format off
        vBufferData.insert(vBufferData.end(), {
            kSamplePositions[s][0] - kQuadWidth, -1.0,
            kSamplePositions[s][0] - kQuadWidth,  1.0,
            kSamplePositions[s][0] + kQuadWidth, -1.0,
            kSamplePositions[s][0] + kQuadWidth,  1.0,
        });
        // clang-format on
    }

    // Add vertices for horizontal quads
    for (uint32_t s = 0; s < kSampleCount; ++s) {
        // clang-format off
        vBufferData.insert(vBufferData.end(), {
            -1.0, kSamplePositions[s][1] - kQuadWidth,
            -1.0, kSamplePositions[s][1] + kQuadWidth,
             1.0, kSamplePositions[s][1] - kQuadWidth,
             1.0, kSamplePositions[s][1] + kQuadWidth,
        });
        // clang-format on
    }

    wgpu::Buffer vBuffer = utils::CreateBufferFromData(
        device, vBufferData.data(), static_cast<uint32_t>(vBufferData.size() * sizeof(float)),
        wgpu::BufferUsage::Vertex);

    static constexpr uint32_t kQuadNumBytes = 8 * sizeof(float);

    wgpu::SamplerDescriptor samplerDesc = {};
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);
    wgpu::TextureView colorView = colorTexture.CreateView();
    wgpu::TextureView depthView = depthTexture.CreateView();

    static constexpr uint64_t kResultSize = 4 * sizeof(float) + 4 * sizeof(float);
    uint64_t alignedResultSize = Align(kResultSize, 256);

    wgpu::BufferDescriptor outputBufferDesc = {};
    outputBufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
    outputBufferDesc.size = alignedResultSize * 8;
    wgpu::Buffer outputBuffer = device.CreateBuffer(&outputBufferDesc);

    wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    for (uint32_t iter = 0; iter < 2; ++iter) {
        for (uint32_t sample = 0; sample < kSampleCount; ++sample) {
            uint32_t sampleOffset = (iter * kSampleCount + sample);

            utils::ComboRenderPassDescriptor renderPass({colorView}, depthView);
            renderPass.cDepthStencilAttachmentInfo.clearDepth = 0.f;

            wgpu::RenderPassEncoder renderPassEncoder = commandEncoder.BeginRenderPass(&renderPass);
            renderPassEncoder.SetPipeline(drawPipeline);
            renderPassEncoder.SetVertexBuffer(0, vBuffer, kQuadNumBytes * sampleOffset,
                                              kQuadNumBytes);
            renderPassEncoder.Draw(4);
            renderPassEncoder.EndPass();

            wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
            computePassEncoder.SetPipeline(checkSamplePipeline);
            computePassEncoder.SetBindGroup(
                0, utils::MakeBindGroup(
                       device, checkSamplePipeline.GetBindGroupLayout(0),
                       {{0, sampler},
                        {1, colorView},
                        {2, depthView},
                        {3, outputBuffer, alignedResultSize * sampleOffset, kResultSize}}));
            computePassEncoder.Dispatch(1);
            computePassEncoder.EndPass();
        }
    }

    wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
    queue.Submit(1, &commandBuffer);

    std::array<float, 8> expectedData;

    expectedData = {1, 0, 0, 0, 0.7, 0, 0, 0};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedData.data(), outputBuffer, 0 * alignedResultSize, 8)
        << "vertical sample 0";

    expectedData = {0, 1, 0, 0, 0, 0.7, 0, 0};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedData.data(), outputBuffer, 1 * alignedResultSize, 8)
        << "vertical sample 1";

    expectedData = {0, 0, 1, 0, 0, 0, 0.7, 0};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedData.data(), outputBuffer, 2 * alignedResultSize, 8)
        << "vertical sample 2";

    expectedData = {0, 0, 0, 1, 0, 0, 0, 0.7};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedData.data(), outputBuffer, 3 * alignedResultSize, 8)
        << "vertical sample 3";

    expectedData = {1, 0, 0, 0, 0.7, 0, 0, 0};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedData.data(), outputBuffer, 4 * alignedResultSize, 8)
        << "horizontal sample 0";

    expectedData = {0, 1, 0, 0, 0, 0.7, 0, 0};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedData.data(), outputBuffer, 5 * alignedResultSize, 8)
        << "horizontal sample 1";

    expectedData = {0, 0, 1, 0, 0, 0, 0.7, 0};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedData.data(), outputBuffer, 6 * alignedResultSize, 8)
        << "horizontal sample 2";

    expectedData = {0, 0, 0, 1, 0, 0, 0, 0.7};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expectedData.data(), outputBuffer, 7 * alignedResultSize, 8)
        << "horizontal sample 3";
}

DAWN_INSTANTIATE_TEST(MultisampledSamplingTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
