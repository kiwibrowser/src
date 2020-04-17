// Copyright 2018 The Dawn Authors
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
#include "common/Constants.h"
#include "tests/DawnTest.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/DawnHelpers.h"

constexpr static unsigned int kRTSize = 8;

class BindGroupTests : public DawnTest {
protected:
    dawn::CommandBuffer CreateSimpleComputeCommandBuffer(
            const dawn::ComputePipeline& pipeline, const dawn::BindGroup& bindGroup) {
        dawn::CommandEncoder encoder = device.CreateCommandEncoder();
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup, 0, nullptr);
        pass.Dispatch(1, 1, 1);
        pass.EndPass();
        return encoder.Finish();
    }

    dawn::PipelineLayout MakeBasicPipelineLayout(
        dawn::Device device,
        std::vector<dawn::BindGroupLayout> bindingInitializer) const {
        dawn::PipelineLayoutDescriptor descriptor;

        descriptor.bindGroupLayoutCount = bindingInitializer.size();
        descriptor.bindGroupLayouts = bindingInitializer.data();

        return device.CreatePipelineLayout(&descriptor);
    }
};

// Test a bindgroup reused in two command buffers in the same call to queue.Submit().
// This test passes by not asserting or crashing.
TEST_P(BindGroupTests, ReusedBindGroupSingleSubmit) {
    dawn::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device,
        {
            {0, dawn::ShaderStageBit::Compute, dawn::BindingType::UniformBuffer},
        }
    );
    dawn::PipelineLayout pl = utils::MakeBasicPipelineLayout(device, &bgl);

    const char* shader = R"(
        #version 450
        layout(std140, set = 0, binding = 0) uniform Contents {
            float f;
        } contents;
        void main() {
        }
    )";

    dawn::ShaderModule module =
        utils::CreateShaderModule(device, dawn::ShaderStage::Compute, shader);
    dawn::ComputePipelineDescriptor cpDesc;
    cpDesc.layout = pl;

    dawn::PipelineStageDescriptor computeStage;
    computeStage.module = module;
    computeStage.entryPoint = "main";
    cpDesc.computeStage = &computeStage;

    dawn::ComputePipeline cp = device.CreateComputePipeline(&cpDesc);

    dawn::BufferDescriptor bufferDesc;
    bufferDesc.size = sizeof(float);
    bufferDesc.usage = dawn::BufferUsageBit::TransferDst |
                       dawn::BufferUsageBit::Uniform;
    dawn::Buffer buffer = device.CreateBuffer(&bufferDesc);
    dawn::BindGroup bindGroup = utils::MakeBindGroup(device, bgl, {{0, buffer, 0, sizeof(float)}});

    dawn::CommandBuffer cb[2];
    cb[0] = CreateSimpleComputeCommandBuffer(cp, bindGroup);
    cb[1] = CreateSimpleComputeCommandBuffer(cp, bindGroup);
    queue.Submit(2, cb);
}

// Test a bindgroup containing a UBO which is used in both the vertex and fragment shader.
// It contains a transformation matrix for the VS and the fragment color for the FS.
// These must result in different register offsets in the native APIs.
TEST_P(BindGroupTests, ReusedUBO) {
    // TODO(jiawei.shao@intel.com): find out why this test fails on Metal
    DAWN_SKIP_TEST_IF(IsMetal());

    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    dawn::ShaderModule vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
        #version 450
        layout (set = 0, binding = 0) uniform vertexUniformBuffer {
            mat2 transform;
        };
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, -1.f), vec2(1.f, -1.f), vec2(-1.f, 1.f));
            gl_Position = vec4(transform * pos[gl_VertexIndex], 0.f, 1.f);
        })"
    );

    dawn::ShaderModule fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
        #version 450
        layout (set = 0, binding = 1) uniform fragmentUniformBuffer {
            vec4 color;
        };
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = color;
        })"
    );

    dawn::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device,
        {
            {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
            {1, dawn::ShaderStageBit::Fragment, dawn::BindingType::UniformBuffer},
        }
    );
    dawn::PipelineLayout pipelineLayout = utils::MakeBasicPipelineLayout(device, &bgl);

    utils::ComboRenderPipelineDescriptor textureDescriptor(device);
    textureDescriptor.layout = pipelineLayout;
    textureDescriptor.cVertexStage.module = vsModule;
    textureDescriptor.cFragmentStage.module = fsModule;
    textureDescriptor.cColorStates[0]->format = renderPass.colorFormat;

    dawn::RenderPipeline pipeline = device.CreateRenderPipeline(&textureDescriptor);

    struct Data {
        float transform[8];
        char padding[256 - 8 * sizeof(float)];
        float color[4];
    };
    ASSERT(offsetof(Data, color) == 256);
    constexpr float dummy = 0.0f;
    Data data {
        { 1.f, 0.f, dummy, dummy, 0.f, 1.0f, dummy, dummy },
        { 0 },
        { 0.f, 1.f, 0.f, 1.f },
    };
    dawn::Buffer buffer = utils::CreateBufferFromData(device, &data, sizeof(data), dawn::BufferUsageBit::Uniform);
    dawn::BindGroup bindGroup = utils::MakeBindGroup(device, bgl, {
        {0, buffer, 0, sizeof(Data::transform)},
        {1, buffer, 256, sizeof(Data::color)}
    });

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup, 0, nullptr);
    pass.Draw(3, 1, 0, 0);
    pass.EndPass();

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(0, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    int min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color,    min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color,    max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color,    min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// Test a bindgroup containing a UBO in the vertex shader and a sampler and texture in the fragment shader.
// In D3D12 for example, these different types of bindings end up in different namespaces, but the register
// offsets used must match between the shader module and descriptor range.
TEST_P(BindGroupTests, UBOSamplerAndTexture) {
    // TODO(jiawei.shao@intel.com): find out why this test fails on Metal
    DAWN_SKIP_TEST_IF(IsMetal());

    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    dawn::ShaderModule vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
        #version 450
        layout (set = 0, binding = 0) uniform vertexUniformBuffer {
            mat2 transform;
        };
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, -1.f), vec2(1.f, -1.f), vec2(-1.f, 1.f));
            gl_Position = vec4(transform * pos[gl_VertexIndex], 0.f, 1.f);
        })"
    );

    dawn::ShaderModule fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
        #version 450
        layout (set = 0, binding = 1) uniform sampler samp;
        layout (set = 0, binding = 2) uniform texture2D tex;
        layout (location = 0) out vec4 fragColor;
        void main() {
            fragColor = texture(sampler2D(tex, samp), gl_FragCoord.xy);
        })"
    );

    dawn::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device,
        {
            {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
            {1, dawn::ShaderStageBit::Fragment, dawn::BindingType::Sampler},
            {2, dawn::ShaderStageBit::Fragment, dawn::BindingType::SampledTexture},
        }
    );
    dawn::PipelineLayout pipelineLayout = utils::MakeBasicPipelineLayout(device, &bgl);

    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
    pipelineDescriptor.layout = pipelineLayout;
    pipelineDescriptor.cVertexStage.module = vsModule;
    pipelineDescriptor.cFragmentStage.module = fsModule;
    pipelineDescriptor.cColorStates[0]->format = renderPass.colorFormat;

    dawn::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    constexpr float dummy = 0.0f;
    constexpr float transform[] = { 1.f, 0.f, dummy, dummy, 0.f, 1.f, dummy, dummy };
    dawn::Buffer buffer = utils::CreateBufferFromData(device, &transform, sizeof(transform), dawn::BufferUsageBit::Uniform);

    dawn::SamplerDescriptor samplerDescriptor;
    samplerDescriptor.minFilter = dawn::FilterMode::Nearest;
    samplerDescriptor.magFilter = dawn::FilterMode::Nearest;
    samplerDescriptor.mipmapFilter = dawn::FilterMode::Nearest;
    samplerDescriptor.addressModeU = dawn::AddressMode::ClampToEdge;
    samplerDescriptor.addressModeV = dawn::AddressMode::ClampToEdge;
    samplerDescriptor.addressModeW = dawn::AddressMode::ClampToEdge;
    samplerDescriptor.lodMinClamp = kLodMin;
    samplerDescriptor.lodMaxClamp = kLodMax;
    samplerDescriptor.compareFunction = dawn::CompareFunction::Never;

    dawn::Sampler sampler = device.CreateSampler(&samplerDescriptor);

    dawn::TextureDescriptor descriptor;
    descriptor.dimension = dawn::TextureDimension::e2D;
    descriptor.size.width = kRTSize;
    descriptor.size.height = kRTSize;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = dawn::TextureFormat::R8G8B8A8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = dawn::TextureUsageBit::TransferDst | dawn::TextureUsageBit::Sampled;
    dawn::Texture texture = device.CreateTexture(&descriptor);
    dawn::TextureView textureView = texture.CreateDefaultView();

    int width = kRTSize, height = kRTSize;
    int widthInBytes = width * sizeof(RGBA8);
    widthInBytes = (widthInBytes + 255) & ~255;
    int sizeInBytes = widthInBytes * height;
    int size = sizeInBytes / sizeof(RGBA8);
    std::vector<RGBA8> data = std::vector<RGBA8>(size);
    for (int i = 0; i < size; i++) {
        data[i] = RGBA8(0, 255, 0, 255);
    }
    dawn::Buffer stagingBuffer = utils::CreateBufferFromData(device, data.data(), sizeInBytes, dawn::BufferUsageBit::TransferSrc);

    dawn::BindGroup bindGroup = utils::MakeBindGroup(device, bgl, {
        {0, buffer, 0, sizeof(transform)},
        {1, sampler},
        {2, textureView}
    });

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    dawn::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(stagingBuffer, 0, widthInBytes, 0);
    dawn::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, 0, {0, 0, 0});
    dawn::Extent3D copySize = {width, height, 1};
    encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup, 0, nullptr);
    pass.Draw(3, 1, 0, 0);
    pass.EndPass();

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(0, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    int min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color,    min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color,    max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color,    min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

TEST_P(BindGroupTests, MultipleBindLayouts) {
    // Test fails on Metal.
    // https://bugs.chromium.org/p/dawn/issues/detail?id=33
    DAWN_SKIP_TEST_IF(IsMetal());

    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    dawn::ShaderModule vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
        #version 450
        layout (set = 0, binding = 0) uniform vertexUniformBuffer1 {
            mat2 transform1;
        };
        layout (set = 1, binding = 0) uniform vertexUniformBuffer2 {
            mat2 transform2;
        };
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, -1.f), vec2(1.f, -1.f), vec2(-1.f, 1.f));
            gl_Position = vec4((transform1 + transform2) * pos[gl_VertexIndex], 0.f, 1.f);
        })");

    dawn::ShaderModule fsModule =
        utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
        #version 450
        layout (set = 0, binding = 1) uniform fragmentUniformBuffer1 {
            vec4 color1;
        };
        layout (set = 1, binding = 1) uniform fragmentUniformBuffer2 {
            vec4 color2;
        };
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = color1 + color2;
        })");

    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {
                    {0, dawn::ShaderStageBit::Vertex, dawn::BindingType::UniformBuffer},
                    {1, dawn::ShaderStageBit::Fragment, dawn::BindingType::UniformBuffer},
                });

    dawn::PipelineLayout pipelineLayout = MakeBasicPipelineLayout(device, {layout, layout});

    utils::ComboRenderPipelineDescriptor textureDescriptor(device);
    textureDescriptor.layout = pipelineLayout;
    textureDescriptor.cVertexStage.module = vsModule;
    textureDescriptor.cFragmentStage.module = fsModule;
    textureDescriptor.cColorStates[0]->format = renderPass.colorFormat;

    dawn::RenderPipeline pipeline = device.CreateRenderPipeline(&textureDescriptor);

    struct Data {
        float transform[8];
        char padding[256 - 8 * sizeof(float)];
        float color[4];
    };
    ASSERT(offsetof(Data, color) == 256);

    std::vector<Data> data;
    std::vector<dawn::Buffer> buffers;
    std::vector<dawn::BindGroup> bindGroups;

    data.push_back(
        {{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0}, {0.0f, 1.0f, 0.0f, 1.0f}});

    data.push_back(
        {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}, {0}, {1.0f, 0.0f, 0.0f, 1.0f}});

    for (int i = 0; i < 2; i++) {
        dawn::Buffer buffer = utils::CreateBufferFromData(device, &data[i], sizeof(Data),
                                                          dawn::BufferUsageBit::Uniform);
        buffers.push_back(buffer);
        bindGroups.push_back(utils::MakeBindGroup(device, layout,
                                                  {{0, buffers[i], 0, sizeof(Data::transform)},
                                                   {1, buffers[i], 256, sizeof(Data::color)}}));
    }

    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroups[0], 0, nullptr);
    pass.SetBindGroup(1, bindGroups[1], 0, nullptr);
    pass.Draw(3, 1, 0, 0);
    pass.EndPass();

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(255, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    int min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// This test reproduces an out-of-bound bug on D3D12 backends when calling draw command twice with
// one pipeline that has 4 bind group sets in one render pass.
TEST_P(BindGroupTests, DrawTwiceInSamePipelineWithFourBindGroupSets)
{
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    dawn::ShaderModule vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
        #version 450
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, -1.f), vec2(1.f, -1.f), vec2(-1.f, 1.f));
            gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
        })");

    dawn::ShaderModule fsModule =
        utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
        #version 450
        layout (std140, set = 0, binding = 0) uniform fragmentUniformBuffer1 {
            vec4 color1;
        };
        layout (std140, set = 1, binding = 0) uniform fragmentUniformBuffer2 {
            vec4 color2;
        };
        layout (std140, set = 2, binding = 0) uniform fragmentUniformBuffer3 {
            vec4 color3;
        };
        layout (std140, set = 3, binding = 0) uniform fragmentUniformBuffer4 {
            vec4 color4;
        };
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = color1 + color2 + color3 + color4;
        })");

    dawn::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {
            { 0, dawn::ShaderStageBit::Fragment, dawn::BindingType::UniformBuffer }
        });
    dawn::PipelineLayout pipelineLayout = MakeBasicPipelineLayout(
        device, { layout, layout, layout, layout });

    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
    pipelineDescriptor.layout = pipelineLayout;
    pipelineDescriptor.cVertexStage.module = vsModule;
    pipelineDescriptor.cFragmentStage.module = fsModule;
    pipelineDescriptor.cColorStates[0]->format = renderPass.colorFormat;

    dawn::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDescriptor);
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();
    dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    pass.SetPipeline(pipeline);

    std::array<float, 4> color = { 0.25, 0, 0, 0.25 };
    dawn::Buffer uniformBuffer = utils::CreateBufferFromData(
        device, &color, sizeof(color), dawn::BufferUsageBit::Uniform);
    dawn::BindGroup bindGroup = utils::MakeBindGroup(
        device, layout, { { 0, uniformBuffer, 0, sizeof(color) } });

    pass.SetBindGroup(0, bindGroup, 0, nullptr);
    pass.SetBindGroup(1, bindGroup, 0, nullptr);
    pass.SetBindGroup(2, bindGroup, 0, nullptr);
    pass.SetBindGroup(3, bindGroup, 0, nullptr);
    pass.Draw(3, 1, 0, 0);

    pass.SetPipeline(pipeline);
    pass.Draw(3, 1, 0, 0);
    pass.EndPass();

    dawn::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(255, 0, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    int min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

DAWN_INSTANTIATE_TEST(BindGroupTests, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend);
