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

#include "common/Assert.h"
#include "tests/DawnTest.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

namespace {

    constexpr wgpu::CompareFunction kCompareFunctions[] = {
        wgpu::CompareFunction::Never,        wgpu::CompareFunction::Less,
        wgpu::CompareFunction::LessEqual,    wgpu::CompareFunction::Greater,
        wgpu::CompareFunction::GreaterEqual, wgpu::CompareFunction::Equal,
        wgpu::CompareFunction::NotEqual,     wgpu::CompareFunction::Always,
    };

    // Test a "normal" ref value between 0 and 1; as well as negative and > 1 refs.
    constexpr float kCompareRefs[] = {-0.1, 0.4, 1.2};

    // Test 0, below the ref, equal to, above the ref, and 1.
    const std::vector<float> kNormalizedTextureValues = {0.0, 0.3, 0.4, 0.5, 1.0};

}  // anonymous namespace

class DepthSamplingTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        wgpu::BufferDescriptor uniformBufferDesc;
        uniformBufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        uniformBufferDesc.size = sizeof(float);
        mUniformBuffer = device.CreateBuffer(&uniformBufferDesc);

        wgpu::BufferDescriptor textureUploadDesc;
        textureUploadDesc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        textureUploadDesc.size = sizeof(float);
        mTextureUploadBuffer = device.CreateBuffer(&textureUploadDesc);

        wgpu::TextureDescriptor inputTextureDesc;
        inputTextureDesc.usage = wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment;
        inputTextureDesc.size = {1, 1, 1};
        inputTextureDesc.format = wgpu::TextureFormat::Depth32Float;
        mInputTexture = device.CreateTexture(&inputTextureDesc);

        wgpu::TextureDescriptor outputTextureDesc;
        outputTextureDesc.usage =
            wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;
        outputTextureDesc.size = {1, 1, 1};
        outputTextureDesc.format = wgpu::TextureFormat::R32Float;
        mOutputTexture = device.CreateTexture(&outputTextureDesc);

        wgpu::BufferDescriptor outputBufferDesc;
        outputBufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
        outputBufferDesc.size = sizeof(float);
        mOutputBuffer = device.CreateBuffer(&outputBufferDesc);
    }

    wgpu::RenderPipeline CreateSamplingRenderPipeline() {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                    gl_Position = vec4(0.f, 0.f, 0.f, 1.f);
                    gl_PointSize = 1.0;
                }
            )");

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(set = 0, binding = 0) uniform sampler samp;
                layout(set = 0, binding = 1) uniform texture2D tex;

                layout(location = 0) out float samplerResult;

                void main() {
                    samplerResult = texture(sampler2D(tex, samp), vec2(0.5, 0.5)).r;
                }
            )");

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.vertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        pipelineDescriptor.primitiveTopology = wgpu::PrimitiveTopology::PointList;
        pipelineDescriptor.cColorStates[0].format = wgpu::TextureFormat::R32Float;

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    wgpu::ComputePipeline CreateSamplingComputePipeline() {
        wgpu::ShaderModule csModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
                #version 450
                layout(set = 0, binding = 0) uniform sampler samp;
                layout(set = 0, binding = 1) uniform texture2D tex;
                layout(set = 0, binding = 2) writeonly buffer SamplerResult {
                    float samplerResult;
                };

                void main() {
                    samplerResult = texture(sampler2D(tex, samp), vec2(0.5, 0.5)).r;
                }
            )");

        wgpu::ComputePipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.computeStage.module = csModule;
        pipelineDescriptor.computeStage.entryPoint = "main";

        return device.CreateComputePipeline(&pipelineDescriptor);
    }

    wgpu::RenderPipeline CreateComparisonRenderPipeline() {
        wgpu::ShaderModule vsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
                #version 450
                void main() {
                    gl_Position = vec4(0.f, 0.f, 0.f, 1.f);
                    gl_PointSize = 1.0;
                }
            )");

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
                #version 450
                layout(set = 0, binding = 0) uniform samplerShadow samp;
                layout(set = 0, binding = 1) uniform texture2D tex;
                layout(set = 0, binding = 2) uniform Uniforms {
                    float compareRef;
                };

                layout(location = 0) out float samplerResult;

                void main() {
                    samplerResult = texture(sampler2DShadow(tex, samp), vec3(0.5, 0.5, compareRef));
                }
            )");

        // TODO(dawn:367): Cannot use GetBindGroupLayout for comparison samplers without shader
        // reflection data.
        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::ComparisonSampler},
                     {1, wgpu::ShaderStage::Fragment, wgpu::BindingType::SampledTexture},
                     {2, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer}});

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.vertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl);
        pipelineDescriptor.primitiveTopology = wgpu::PrimitiveTopology::PointList;
        pipelineDescriptor.cColorStates[0].format = wgpu::TextureFormat::R32Float;

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    wgpu::ComputePipeline CreateComparisonComputePipeline() {
        wgpu::ShaderModule csModule =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
                #version 450
                layout(set = 0, binding = 0) uniform samplerShadow samp;
                layout(set = 0, binding = 1) uniform texture2D tex;
                layout(set = 0, binding = 2) uniform Uniforms {
                    float compareRef;
                };
                layout(set = 0, binding = 3) writeonly buffer SamplerResult {
                    float samplerResult;
                };

                void main() {
                    samplerResult = texture(sampler2DShadow(tex, samp), vec3(0.5, 0.5, compareRef));
                }
            )");

        // TODO(dawn:367): Cannot use GetBindGroupLayout without shader reflection data.
        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BindingType::ComparisonSampler},
                     {1, wgpu::ShaderStage::Compute, wgpu::BindingType::SampledTexture},
                     {2, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer},
                     {3, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer}});

        wgpu::ComputePipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl);
        pipelineDescriptor.computeStage.module = csModule;
        pipelineDescriptor.computeStage.entryPoint = "main";

        return device.CreateComputePipeline(&pipelineDescriptor);
    }

    void UpdateInputTexture(wgpu::CommandEncoder commandEncoder, float textureValue) {
        utils::ComboRenderPassDescriptor passDescriptor({}, mInputTexture.CreateView());
        passDescriptor.cDepthStencilAttachmentInfo.clearDepth = textureValue;

        wgpu::RenderPassEncoder pass = commandEncoder.BeginRenderPass(&passDescriptor);
        pass.EndPass();
    }

    void DoSamplingTest(wgpu::RenderPipeline pipeline, std::vector<float> textureValues) {
        wgpu::SamplerDescriptor samplerDesc;
        wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

        wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                         {
                                                             {0, sampler},
                                                             {1, mInputTexture.CreateView()},
                                                         });

        for (float textureValue : textureValues) {
            // Set the input depth texture to the provided texture value
            wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
            UpdateInputTexture(commandEncoder, textureValue);

            // Render into the output texture
            {
                utils::ComboRenderPassDescriptor passDescriptor({mOutputTexture.CreateView()});
                wgpu::RenderPassEncoder pass = commandEncoder.BeginRenderPass(&passDescriptor);
                pass.SetPipeline(pipeline);
                pass.SetBindGroup(0, bindGroup);
                pass.Draw(1);
                pass.EndPass();
            }

            wgpu::CommandBuffer commands = commandEncoder.Finish();
            queue.Submit(1, &commands);

            EXPECT_PIXEL_FLOAT_EQ(textureValue, mOutputTexture, 0, 0);
        }
    }

    void DoSamplingTest(wgpu::ComputePipeline pipeline, std::vector<float> textureValues) {
        wgpu::SamplerDescriptor samplerDesc;
        wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, pipeline.GetBindGroupLayout(0),
            {{0, sampler}, {1, mInputTexture.CreateView()}, {2, mOutputBuffer}});

        for (float textureValue : textureValues) {
            // Set the input depth texture to the provided texture value
            wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
            UpdateInputTexture(commandEncoder, textureValue);

            // Sample into the output buffer
            {
                wgpu::ComputePassEncoder pass = commandEncoder.BeginComputePass();
                pass.SetPipeline(pipeline);
                pass.SetBindGroup(0, bindGroup);
                pass.Dispatch(1);
                pass.EndPass();
            }

            wgpu::CommandBuffer commands = commandEncoder.Finish();
            queue.Submit(1, &commands);

            EXPECT_BUFFER_U32_EQ(*reinterpret_cast<uint32_t*>(&textureValue), mOutputBuffer, 0);
        }
    }

    static bool CompareFunctionPasses(float compareRef,
                                      wgpu::CompareFunction compare,
                                      float textureValue) {
        switch (compare) {
            case wgpu::CompareFunction::Never:
                return false;
            case wgpu::CompareFunction::Less:
                return compareRef < textureValue;
            case wgpu::CompareFunction::LessEqual:
                return compareRef <= textureValue;
            case wgpu::CompareFunction::Greater:
                return compareRef > textureValue;
            case wgpu::CompareFunction::GreaterEqual:
                return compareRef >= textureValue;
            case wgpu::CompareFunction::Equal:
                return compareRef == textureValue;
            case wgpu::CompareFunction::NotEqual:
                return compareRef != textureValue;
            case wgpu::CompareFunction::Always:
                return true;
            default:
                return false;
        }
    }

    void DoCompareRefTest(wgpu::RenderPipeline pipeline,
                          float compareRef,
                          wgpu::CompareFunction compare,
                          std::vector<float> textureValues) {
        queue.WriteBuffer(mUniformBuffer, 0, &compareRef, sizeof(float));

        wgpu::SamplerDescriptor samplerDesc;
        samplerDesc.compare = compare;
        wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

        wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                         {
                                                             {0, sampler},
                                                             {1, mInputTexture.CreateView()},
                                                             {2, mUniformBuffer},
                                                         });

        for (float textureValue : textureValues) {
            // Set the input depth texture to the provided texture value
            wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
            UpdateInputTexture(commandEncoder, textureValue);

            // Render into the output texture
            {
                utils::ComboRenderPassDescriptor passDescriptor({mOutputTexture.CreateView()});
                wgpu::RenderPassEncoder pass = commandEncoder.BeginRenderPass(&passDescriptor);
                pass.SetPipeline(pipeline);
                pass.SetBindGroup(0, bindGroup);
                pass.Draw(1);
                pass.EndPass();
            }

            wgpu::CommandBuffer commands = commandEncoder.Finish();
            queue.Submit(1, &commands);

            EXPECT_PIXEL_FLOAT_EQ(
                CompareFunctionPasses(compareRef, compare, textureValue) ? 1.f : 0.f,
                mOutputTexture, 0, 0);
        }
    }

    void DoCompareRefTest(wgpu::ComputePipeline pipeline,
                          float compareRef,
                          wgpu::CompareFunction compare,
                          std::vector<float> textureValues) {
        queue.WriteBuffer(mUniformBuffer, 0, &compareRef, sizeof(float));

        wgpu::SamplerDescriptor samplerDesc;
        samplerDesc.compare = compare;
        wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

        wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                         {{0, sampler},
                                                          {1, mInputTexture.CreateView()},
                                                          {2, mUniformBuffer},
                                                          {3, mOutputBuffer}});

        for (float textureValue : textureValues) {
            // Set the input depth texture to the provided texture value
            wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
            UpdateInputTexture(commandEncoder, textureValue);

            // Sample into the output buffer
            {
                wgpu::ComputePassEncoder pass = commandEncoder.BeginComputePass();
                pass.SetPipeline(pipeline);
                pass.SetBindGroup(0, bindGroup);
                pass.Dispatch(1);
                pass.EndPass();
            }

            wgpu::CommandBuffer commands = commandEncoder.Finish();
            queue.Submit(1, &commands);

            float float0 = 0.f;
            float float1 = 1.f;
            float* expected =
                CompareFunctionPasses(compareRef, compare, textureValue) ? &float1 : &float0;

            EXPECT_BUFFER_U32_EQ(*reinterpret_cast<uint32_t*>(expected), mOutputBuffer, 0);
        }
    }

  private:
    wgpu::Buffer mUniformBuffer;
    wgpu::Buffer mTextureUploadBuffer;
    wgpu::Texture mInputTexture;
    wgpu::Texture mOutputTexture;
    wgpu::Buffer mOutputBuffer;
};

// Test that sampling a depth texture with a render pipeline works
TEST_P(DepthSamplingTest, SampleRender) {
    // Test 0, between [0, 1], and 1.
    DoSamplingTest(CreateSamplingRenderPipeline(), kNormalizedTextureValues);
}

// Test that sampling a depth texture with a compute pipeline works
TEST_P(DepthSamplingTest, SampleCompute) {
    // Test 0, between [0, 1], and 1.
    DoSamplingTest(CreateSamplingComputePipeline(), kNormalizedTextureValues);
}

// Test that sampling in a render pipeline with all of the compare functions works.
TEST_P(DepthSamplingTest, CompareFunctionsRender) {
    // Initialization via renderPass loadOp doesn't work on Mac Intel.
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    wgpu::RenderPipeline pipeline = CreateComparisonRenderPipeline();

    // Test a "normal" ref value between 0 and 1; as well as negative and > 1 refs.
    for (float compareRef : kCompareRefs) {
        // Test 0, below the ref, equal to, above the ref, and 1.
        for (wgpu::CompareFunction f : kCompareFunctions) {
            DoCompareRefTest(pipeline, compareRef, f, kNormalizedTextureValues);
        }
    }
}

// Test that sampling in a render pipeline with all of the compare functions works.
TEST_P(DepthSamplingTest, CompareFunctionsCompute) {
    // Initialization via renderPass loadOp doesn't work on Mac Intel.
    DAWN_SKIP_TEST_IF(IsMetal() && IsIntel());

    wgpu::ComputePipeline pipeline = CreateComparisonComputePipeline();

    // Test a "normal" ref value between 0 and 1; as well as negative and > 1 refs.
    for (float compareRef : kCompareRefs) {
        // Test 0, below the ref, equal to, above the ref, and 1.
        for (wgpu::CompareFunction f : kCompareFunctions) {
            DoCompareRefTest(pipeline, compareRef, f, kNormalizedTextureValues);
        }
    }
}

DAWN_INSTANTIATE_TEST(DepthSamplingTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
