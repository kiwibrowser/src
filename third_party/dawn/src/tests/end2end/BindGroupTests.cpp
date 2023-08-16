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
#include "common/Math.h"
#include "tests/DawnTest.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

constexpr static uint32_t kRTSize = 8;

class BindGroupTests : public DawnTest {
  protected:
    wgpu::CommandBuffer CreateSimpleComputeCommandBuffer(const wgpu::ComputePipeline& pipeline,
                                                         const wgpu::BindGroup& bindGroup) {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.Dispatch(1);
        pass.EndPass();
        return encoder.Finish();
    }

    wgpu::PipelineLayout MakeBasicPipelineLayout(
        std::vector<wgpu::BindGroupLayout> bindingInitializer) const {
        wgpu::PipelineLayoutDescriptor descriptor;

        descriptor.bindGroupLayoutCount = bindingInitializer.size();
        descriptor.bindGroupLayouts = bindingInitializer.data();

        return device.CreatePipelineLayout(&descriptor);
    }

    wgpu::ShaderModule MakeSimpleVSModule() const {
        return utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
            gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
        })");
    }

    wgpu::ShaderModule MakeFSModule(std::vector<wgpu::BindingType> bindingTypes) const {
        ASSERT(bindingTypes.size() <= kMaxBindGroups);

        std::ostringstream fs;
        fs << R"(
        #version 450
        layout(location = 0) out vec4 fragColor;
        )";

        for (size_t i = 0; i < bindingTypes.size(); ++i) {
            switch (bindingTypes[i]) {
                case wgpu::BindingType::UniformBuffer:
                    fs << "layout (std140, set = " << i << ", binding = 0) uniform UniformBuffer"
                       << i << R"( {
                        vec4 color;
                    } buffer)"
                       << i << ";\n";
                    break;
                case wgpu::BindingType::StorageBuffer:
                    fs << "layout (std430, set = " << i << ", binding = 0) buffer StorageBuffer"
                       << i << R"( {
                        vec4 color;
                    } buffer)"
                       << i << ";\n";
                    break;
                default:
                    UNREACHABLE();
            }
        }

        fs << R"(
        void main() {
            fragColor = vec4(0.0);
        )";
        for (size_t i = 0; i < bindingTypes.size(); ++i) {
            fs << "fragColor += buffer" << i << ".color;\n";
        }
        fs << "}\n";

        return utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment,
                                         fs.str().c_str());
    }

    wgpu::RenderPipeline MakeTestPipeline(const utils::BasicRenderPass& renderPass,
                                          std::vector<wgpu::BindingType> bindingTypes,
                                          std::vector<wgpu::BindGroupLayout> bindGroupLayouts) {
        wgpu::ShaderModule vsModule = MakeSimpleVSModule();
        wgpu::ShaderModule fsModule = MakeFSModule(bindingTypes);

        wgpu::PipelineLayout pipelineLayout = MakeBasicPipelineLayout(bindGroupLayouts);

        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
        pipelineDescriptor.layout = pipelineLayout;
        pipelineDescriptor.vertexStage.module = vsModule;
        pipelineDescriptor.cFragmentStage.module = fsModule;
        pipelineDescriptor.cColorStates[0].format = renderPass.colorFormat;
        pipelineDescriptor.cColorStates[0].colorBlend.operation = wgpu::BlendOperation::Add;
        pipelineDescriptor.cColorStates[0].colorBlend.srcFactor = wgpu::BlendFactor::One;
        pipelineDescriptor.cColorStates[0].colorBlend.dstFactor = wgpu::BlendFactor::One;
        pipelineDescriptor.cColorStates[0].alphaBlend.operation = wgpu::BlendOperation::Add;
        pipelineDescriptor.cColorStates[0].alphaBlend.srcFactor = wgpu::BlendFactor::One;
        pipelineDescriptor.cColorStates[0].alphaBlend.dstFactor = wgpu::BlendFactor::One;

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }
};

// Test a bindgroup reused in two command buffers in the same call to queue.Submit().
// This test passes by not asserting or crashing.
TEST_P(BindGroupTests, ReusedBindGroupSingleSubmit) {
    const char* shader = R"(
        #version 450
        layout(std140, set = 0, binding = 0) uniform Contents {
            float f;
        } contents;
        void main() {
        }
    )";

    wgpu::ShaderModule module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, shader);

    wgpu::ComputePipelineDescriptor cpDesc;
    cpDesc.computeStage.module = module;
    cpDesc.computeStage.entryPoint = "main";
    wgpu::ComputePipeline cp = device.CreateComputePipeline(&cpDesc);

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = sizeof(float);
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, cp.GetBindGroupLayout(0), {{0, buffer}});

    wgpu::CommandBuffer cb[2];
    cb[0] = CreateSimpleComputeCommandBuffer(cp, bindGroup);
    cb[1] = CreateSimpleComputeCommandBuffer(cp, bindGroup);
    queue.Submit(2, cb);
}

// Test a bindgroup containing a UBO which is used in both the vertex and fragment shader.
// It contains a transformation matrix for the VS and the fragment color for the FS.
// These must result in different register offsets in the native APIs.
TEST_P(BindGroupTests, ReusedUBO) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout (set = 0, binding = 0) uniform vertexUniformBuffer {
            mat2 transform;
        };
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
            gl_Position = vec4(transform * pos[gl_VertexIndex], 0.f, 1.f);
        })");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout (set = 0, binding = 1) uniform fragmentUniformBuffer {
            vec4 color;
        };
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = color;
        })");

    utils::ComboRenderPipelineDescriptor textureDescriptor(device);
    textureDescriptor.vertexStage.module = vsModule;
    textureDescriptor.cFragmentStage.module = fsModule;
    textureDescriptor.cColorStates[0].format = renderPass.colorFormat;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&textureDescriptor);

    struct Data {
        float transform[8];
        char padding[256 - 8 * sizeof(float)];
        float color[4];
    };
    ASSERT(offsetof(Data, color) == 256);
    constexpr float dummy = 0.0f;
    Data data{
        {1.f, 0.f, dummy, dummy, 0.f, 1.0f, dummy, dummy},
        {0},
        {0.f, 1.f, 0.f, 1.f},
    };
    wgpu::Buffer buffer =
        utils::CreateBufferFromData(device, &data, sizeof(data), wgpu::BufferUsage::Uniform);
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(
        device, pipeline.GetBindGroupLayout(0),
        {{0, buffer, 0, sizeof(Data::transform)}, {1, buffer, 256, sizeof(Data::color)}});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(3);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(0, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    uint32_t min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// Test a bindgroup containing a UBO in the vertex shader and a sampler and texture in the fragment
// shader. In D3D12 for example, these different types of bindings end up in different namespaces,
// but the register offsets used must match between the shader module and descriptor range.
TEST_P(BindGroupTests, UBOSamplerAndTexture) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout (set = 0, binding = 0) uniform vertexUniformBuffer {
            mat2 transform;
        };
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
            gl_Position = vec4(transform * pos[gl_VertexIndex], 0.f, 1.f);
        })");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout (set = 0, binding = 1) uniform sampler samp;
        layout (set = 0, binding = 2) uniform texture2D tex;
        layout (location = 0) out vec4 fragColor;
        void main() {
            fragColor = texture(sampler2D(tex, samp), gl_FragCoord.xy);
        })");

    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
    pipelineDescriptor.vertexStage.module = vsModule;
    pipelineDescriptor.cFragmentStage.module = fsModule;
    pipelineDescriptor.cColorStates[0].format = renderPass.colorFormat;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    constexpr float dummy = 0.0f;
    constexpr float transform[] = {1.f, 0.f, dummy, dummy, 0.f, 1.f, dummy, dummy};
    wgpu::Buffer buffer = utils::CreateBufferFromData(device, &transform, sizeof(transform),
                                                      wgpu::BufferUsage::Uniform);

    wgpu::SamplerDescriptor samplerDescriptor = {};
    samplerDescriptor.minFilter = wgpu::FilterMode::Nearest;
    samplerDescriptor.magFilter = wgpu::FilterMode::Nearest;
    samplerDescriptor.mipmapFilter = wgpu::FilterMode::Nearest;
    samplerDescriptor.addressModeU = wgpu::AddressMode::ClampToEdge;
    samplerDescriptor.addressModeV = wgpu::AddressMode::ClampToEdge;
    samplerDescriptor.addressModeW = wgpu::AddressMode::ClampToEdge;

    wgpu::Sampler sampler = device.CreateSampler(&samplerDescriptor);

    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kRTSize;
    descriptor.size.height = kRTSize;
    descriptor.size.depth = 1;
    descriptor.sampleCount = 1;
    descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled;
    wgpu::Texture texture = device.CreateTexture(&descriptor);
    wgpu::TextureView textureView = texture.CreateView();

    uint32_t width = kRTSize, height = kRTSize;
    uint32_t widthInBytes = width * sizeof(RGBA8);
    widthInBytes = (widthInBytes + 255) & ~255;
    uint32_t sizeInBytes = widthInBytes * height;
    uint32_t size = sizeInBytes / sizeof(RGBA8);
    std::vector<RGBA8> data = std::vector<RGBA8>(size);
    for (uint32_t i = 0; i < size; i++) {
        data[i] = RGBA8(0, 255, 0, 255);
    }
    wgpu::Buffer stagingBuffer =
        utils::CreateBufferFromData(device, data.data(), sizeInBytes, wgpu::BufferUsage::CopySrc);

    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                             {{0, buffer, 0, sizeof(transform)}, {1, sampler}, {2, textureView}});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::BufferCopyView bufferCopyView =
        utils::CreateBufferCopyView(stagingBuffer, 0, widthInBytes, 0);
    wgpu::TextureCopyView textureCopyView = utils::CreateTextureCopyView(texture, 0, {0, 0, 0});
    wgpu::Extent3D copySize = {width, height, 1};
    encoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(3);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(0, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    uint32_t min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

TEST_P(BindGroupTests, MultipleBindLayouts) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout (set = 0, binding = 0) uniform vertexUniformBuffer1 {
            mat2 transform1;
        };
        layout (set = 1, binding = 0) uniform vertexUniformBuffer2 {
            mat2 transform2;
        };
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
            gl_Position = vec4((transform1 + transform2) * pos[gl_VertexIndex], 0.f, 1.f);
        })");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
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

    utils::ComboRenderPipelineDescriptor textureDescriptor(device);
    textureDescriptor.vertexStage.module = vsModule;
    textureDescriptor.cFragmentStage.module = fsModule;
    textureDescriptor.cColorStates[0].format = renderPass.colorFormat;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&textureDescriptor);

    struct Data {
        float transform[8];
        char padding[256 - 8 * sizeof(float)];
        float color[4];
    };
    ASSERT(offsetof(Data, color) == 256);

    std::vector<Data> data;
    std::vector<wgpu::Buffer> buffers;
    std::vector<wgpu::BindGroup> bindGroups;

    data.push_back(
        {{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0}, {0.0f, 1.0f, 0.0f, 1.0f}});

    data.push_back(
        {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}, {0}, {1.0f, 0.0f, 0.0f, 1.0f}});

    for (int i = 0; i < 2; i++) {
        wgpu::Buffer buffer =
            utils::CreateBufferFromData(device, &data[i], sizeof(Data), wgpu::BufferUsage::Uniform);
        buffers.push_back(buffer);
        bindGroups.push_back(utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                  {{0, buffers[i], 0, sizeof(Data::transform)},
                                                   {1, buffers[i], 256, sizeof(Data::color)}}));
    }

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroups[0]);
    pass.SetBindGroup(1, bindGroups[1]);
    pass.Draw(3);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(255, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    uint32_t min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// This test reproduces an out-of-bound bug on D3D12 backends when calling draw command twice with
// one pipeline that has 4 bind group sets in one render pass.
TEST_P(BindGroupTests, DrawTwiceInSamePipelineWithFourBindGroupSets) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer}});

    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(renderPass,
                         {wgpu::BindingType::UniformBuffer, wgpu::BindingType::UniformBuffer,
                          wgpu::BindingType::UniformBuffer, wgpu::BindingType::UniformBuffer},
                         {layout, layout, layout, layout});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    pass.SetPipeline(pipeline);

    // The color will be added 8 times, so the value should be 0.125. But we choose 0.126
    // because of precision issues on some devices (for example NVIDIA bots).
    std::array<float, 4> color = {0.126, 0, 0, 0.126};
    wgpu::Buffer uniformBuffer =
        utils::CreateBufferFromData(device, &color, sizeof(color), wgpu::BufferUsage::Uniform);
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, layout, {{0, uniformBuffer, 0, sizeof(color)}});

    pass.SetBindGroup(0, bindGroup);
    pass.SetBindGroup(1, bindGroup);
    pass.SetBindGroup(2, bindGroup);
    pass.SetBindGroup(3, bindGroup);
    pass.Draw(3);

    pass.SetPipeline(pipeline);
    pass.Draw(3);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(255, 0, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    uint32_t min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// Test that bind groups can be set before the pipeline.
TEST_P(BindGroupTests, SetBindGroupBeforePipeline) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    // Create a bind group layout which uses a single uniform buffer.
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer}});

    // Create a pipeline that uses the uniform bind group layout.
    wgpu::RenderPipeline pipeline =
        MakeTestPipeline(renderPass, {wgpu::BindingType::UniformBuffer}, {layout});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    // Create a bind group with a uniform buffer and fill it with RGBAunorm(1, 0, 0, 1).
    std::array<float, 4> color = {1, 0, 0, 1};
    wgpu::Buffer uniformBuffer =
        utils::CreateBufferFromData(device, &color, sizeof(color), wgpu::BufferUsage::Uniform);
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, layout, {{0, uniformBuffer, 0, sizeof(color)}});

    // Set the bind group, then the pipeline, and draw.
    pass.SetBindGroup(0, bindGroup);
    pass.SetPipeline(pipeline);
    pass.Draw(3);

    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // The result should be red.
    RGBA8 filled(255, 0, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    uint32_t min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// Test that dynamic bind groups can be set before the pipeline.
TEST_P(BindGroupTests, SetDynamicBindGroupBeforePipeline) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    // Create a bind group layout which uses a single dynamic uniform buffer.
    wgpu::BindGroupLayout layout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer, true}});

    // Create a pipeline that uses the dynamic uniform bind group layout for two bind groups.
    wgpu::RenderPipeline pipeline = MakeTestPipeline(
        renderPass, {wgpu::BindingType::UniformBuffer, wgpu::BindingType::UniformBuffer},
        {layout, layout});

    // Prepare data RGBAunorm(1, 0, 0, 0.5) and RGBAunorm(0, 1, 0, 0.5). They will be added in the
    // shader.
    std::array<float, 4> color0 = {1, 0, 0, 0.501};
    std::array<float, 4> color1 = {0, 1, 0, 0.501};

    size_t color1Offset = Align(sizeof(color0), kMinDynamicBufferOffsetAlignment);

    std::vector<uint8_t> data(color1Offset + sizeof(color1));
    memcpy(data.data(), color0.data(), sizeof(color0));
    memcpy(data.data() + color1Offset, color1.data(), sizeof(color1));

    // Create a bind group and uniform buffer with the color data. It will be bound at the offset
    // to each color.
    wgpu::Buffer uniformBuffer =
        utils::CreateBufferFromData(device, data.data(), data.size(), wgpu::BufferUsage::Uniform);
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, layout, {{0, uniformBuffer, 0, 4 * sizeof(float)}});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    // Set the first dynamic bind group.
    uint32_t dynamicOffset = 0;
    pass.SetBindGroup(0, bindGroup, 1, &dynamicOffset);

    // Set the second dynamic bind group.
    dynamicOffset = color1Offset;
    pass.SetBindGroup(1, bindGroup, 1, &dynamicOffset);

    // Set the pipeline and draw.
    pass.SetPipeline(pipeline);
    pass.Draw(3);

    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // The result should be RGBAunorm(1, 0, 0, 0.5) + RGBAunorm(0, 1, 0, 0.5)
    RGBA8 filled(255, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    uint32_t min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// Test that bind groups set for one pipeline are still set when the pipeline changes.
TEST_P(BindGroupTests, BindGroupsPersistAfterPipelineChange) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    // Create a bind group layout which uses a single dynamic uniform buffer.
    wgpu::BindGroupLayout uniformLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer, true}});

    // Create a bind group layout which uses a single dynamic storage buffer.
    wgpu::BindGroupLayout storageLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer, true}});

    // Create a pipeline which uses the uniform buffer and storage buffer bind groups.
    wgpu::RenderPipeline pipeline0 = MakeTestPipeline(
        renderPass, {wgpu::BindingType::UniformBuffer, wgpu::BindingType::StorageBuffer},
        {uniformLayout, storageLayout});

    // Create a pipeline which uses the uniform buffer bind group twice.
    wgpu::RenderPipeline pipeline1 = MakeTestPipeline(
        renderPass, {wgpu::BindingType::UniformBuffer, wgpu::BindingType::UniformBuffer},
        {uniformLayout, uniformLayout});

    // Prepare data RGBAunorm(1, 0, 0, 0.5) and RGBAunorm(0, 1, 0, 0.5). They will be added in the
    // shader.
    std::array<float, 4> color0 = {1, 0, 0, 0.5};
    std::array<float, 4> color1 = {0, 1, 0, 0.5};

    size_t color1Offset = Align(sizeof(color0), kMinDynamicBufferOffsetAlignment);

    std::vector<uint8_t> data(color1Offset + sizeof(color1));
    memcpy(data.data(), color0.data(), sizeof(color0));
    memcpy(data.data() + color1Offset, color1.data(), sizeof(color1));

    // Create a bind group and uniform buffer with the color data. It will be bound at the offset
    // to each color.
    wgpu::Buffer uniformBuffer =
        utils::CreateBufferFromData(device, data.data(), data.size(), wgpu::BufferUsage::Uniform);
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, uniformLayout, {{0, uniformBuffer, 0, 4 * sizeof(float)}});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    // Set the first pipeline (uniform, storage).
    pass.SetPipeline(pipeline0);

    // Set the first bind group at a dynamic offset.
    // This bind group matches the slot in the pipeline layout.
    uint32_t dynamicOffset = 0;
    pass.SetBindGroup(0, bindGroup, 1, &dynamicOffset);

    // Set the second bind group at a dynamic offset.
    // This bind group does not match the slot in the pipeline layout.
    dynamicOffset = color1Offset;
    pass.SetBindGroup(1, bindGroup, 1, &dynamicOffset);

    // Set the second pipeline (uniform, uniform).
    // Both bind groups match the pipeline.
    // They should persist and not need to be bound again.
    pass.SetPipeline(pipeline1);
    pass.Draw(3);

    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // The result should be RGBAunorm(1, 0, 0, 0.5) + RGBAunorm(0, 1, 0, 0.5)
    RGBA8 filled(255, 255, 0, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    uint32_t min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// Do a successful draw. Then, change the pipeline and one bind group.
// Draw to check that the all bind groups are set.
TEST_P(BindGroupTests, DrawThenChangePipelineAndBindGroup) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    // Create a bind group layout which uses a single dynamic uniform buffer.
    wgpu::BindGroupLayout uniformLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::UniformBuffer, true}});

    // Create a bind group layout which uses a single dynamic storage buffer.
    wgpu::BindGroupLayout storageLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer, true}});

    // Create a pipeline with pipeline layout (uniform, uniform, storage).
    wgpu::RenderPipeline pipeline0 =
        MakeTestPipeline(renderPass,
                         {wgpu::BindingType::UniformBuffer, wgpu::BindingType::UniformBuffer,
                          wgpu::BindingType::StorageBuffer},
                         {uniformLayout, uniformLayout, storageLayout});

    // Create a pipeline with pipeline layout (uniform, storage, storage).
    wgpu::RenderPipeline pipeline1 =
        MakeTestPipeline(renderPass,
                         {wgpu::BindingType::UniformBuffer, wgpu::BindingType::StorageBuffer,
                          wgpu::BindingType::StorageBuffer},
                         {uniformLayout, storageLayout, storageLayout});

    // Prepare color data.
    // The first draw will use { color0, color1, color2 }.
    // The second draw will use { color0, color3, color2 }.
    // The pipeline uses additive color and alpha blending so the result of two draws should be
    // { 2 * color0 + color1 + 2 * color2 + color3} = RGBAunorm(1, 1, 1, 1)
    std::array<float, 4> color0 = {0.501, 0, 0, 0};
    std::array<float, 4> color1 = {0, 1, 0, 0};
    std::array<float, 4> color2 = {0, 0, 0, 0.501};
    std::array<float, 4> color3 = {0, 0, 1, 0};

    size_t color1Offset = Align(sizeof(color0), kMinDynamicBufferOffsetAlignment);
    size_t color2Offset = Align(color1Offset + sizeof(color1), kMinDynamicBufferOffsetAlignment);
    size_t color3Offset = Align(color2Offset + sizeof(color2), kMinDynamicBufferOffsetAlignment);

    std::vector<uint8_t> data(color3Offset + sizeof(color3), 0);
    memcpy(data.data(), color0.data(), sizeof(color0));
    memcpy(data.data() + color1Offset, color1.data(), sizeof(color1));
    memcpy(data.data() + color2Offset, color2.data(), sizeof(color2));
    memcpy(data.data() + color3Offset, color3.data(), sizeof(color3));

    // Create a uniform and storage buffer bind groups to bind the color data.
    wgpu::Buffer uniformBuffer =
        utils::CreateBufferFromData(device, data.data(), data.size(), wgpu::BufferUsage::Uniform);

    wgpu::Buffer storageBuffer =
        utils::CreateBufferFromData(device, data.data(), data.size(), wgpu::BufferUsage::Storage);

    wgpu::BindGroup uniformBindGroup =
        utils::MakeBindGroup(device, uniformLayout, {{0, uniformBuffer, 0, 4 * sizeof(float)}});
    wgpu::BindGroup storageBindGroup =
        utils::MakeBindGroup(device, storageLayout, {{0, storageBuffer, 0, 4 * sizeof(float)}});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    // Set the pipeline to (uniform, uniform, storage)
    pass.SetPipeline(pipeline0);

    // Set the first bind group to color0 in the dynamic uniform buffer.
    uint32_t dynamicOffset = 0;
    pass.SetBindGroup(0, uniformBindGroup, 1, &dynamicOffset);

    // Set the first bind group to color1 in the dynamic uniform buffer.
    dynamicOffset = color1Offset;
    pass.SetBindGroup(1, uniformBindGroup, 1, &dynamicOffset);

    // Set the first bind group to color2 in the dynamic storage buffer.
    dynamicOffset = color2Offset;
    pass.SetBindGroup(2, storageBindGroup, 1, &dynamicOffset);

    pass.Draw(3);

    // Set the pipeline to (uniform, storage, storage)
    //  - The first bind group should persist (inherited on some backends)
    //  - The second bind group needs to be set again to pass validation.
    //    It changed from uniform to storage.
    //  - The third bind group should persist. It should be set again by the backend internally.
    pass.SetPipeline(pipeline1);

    // Set the second bind group to color3 in the dynamic storage buffer.
    dynamicOffset = color3Offset;
    pass.SetBindGroup(1, storageBindGroup, 1, &dynamicOffset);

    pass.Draw(3);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    RGBA8 filled(255, 255, 255, 255);
    RGBA8 notFilled(0, 0, 0, 0);
    uint32_t min = 1, max = kRTSize - 3;
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, max, min);
    EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, min, max);
    EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, max, max);
}

// Regression test for crbug.com/dawn/408 where dynamic offsets were applied in the wrong order.
// Dynamic offsets should be applied in increasing order of binding number.
TEST_P(BindGroupTests, DynamicOffsetOrder) {
    // We will put the following values and the respective offsets into a buffer.
    // The test will ensure that the correct dynamic offset is applied to each buffer by reading the
    // value from an offset binding.
    std::array<uint32_t, 3> offsets = {3 * kMinDynamicBufferOffsetAlignment,
                                       1 * kMinDynamicBufferOffsetAlignment,
                                       2 * kMinDynamicBufferOffsetAlignment};
    std::array<uint32_t, 3> values = {21, 67, 32};

    // Create three buffers large enough to by offset by the largest offset.
    wgpu::BufferDescriptor bufferDescriptor;
    bufferDescriptor.size = 3 * kMinDynamicBufferOffsetAlignment + sizeof(uint32_t);
    bufferDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;

    wgpu::Buffer buffer0 = device.CreateBuffer(&bufferDescriptor);
    wgpu::Buffer buffer3 = device.CreateBuffer(&bufferDescriptor);

    // This test uses both storage and uniform buffers to ensure buffer bindings are sorted first by
    // binding number before type.
    bufferDescriptor.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer2 = device.CreateBuffer(&bufferDescriptor);

    // Populate the values
    queue.WriteBuffer(buffer0, offsets[0], &values[0], sizeof(uint32_t));
    queue.WriteBuffer(buffer2, offsets[1], &values[1], sizeof(uint32_t));
    queue.WriteBuffer(buffer3, offsets[2], &values[2], sizeof(uint32_t));

    wgpu::Buffer outputBuffer = utils::CreateBufferFromData(
        device, wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage, {0, 0, 0});

    // Create the bind group and bind group layout.
    // Note: The order of the binding numbers are intentionally different and not in increasing
    // order.
    wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device, {
                    {3, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer, true},
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::ReadonlyStorageBuffer, true},
                    {2, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer, true},
                    {4, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer},
                });
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, bgl,
                                                     {
                                                         {0, buffer0, 0, sizeof(uint32_t)},
                                                         {3, buffer3, 0, sizeof(uint32_t)},
                                                         {2, buffer2, 0, sizeof(uint32_t)},
                                                         {4, outputBuffer, 0, 3 * sizeof(uint32_t)},
                                                     });

    wgpu::ComputePipelineDescriptor pipelineDescriptor;
    pipelineDescriptor.computeStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
        #version 450
        layout(std140, set = 0, binding = 2) uniform Buffer2 {
            uint value2;
        };
        layout(std430, set = 0, binding = 3) readonly buffer Buffer3 {
            uint value3;
        };
        layout(std430, set = 0, binding = 0) readonly buffer Buffer0 {
            uint value0;
        };
        layout(std430, set = 0, binding = 4) buffer OutputBuffer {
            uvec3 outputBuffer;
        };
        void main() {
            outputBuffer = uvec3(value0, value2, value3);
        }
    )");
    pipelineDescriptor.computeStage.entryPoint = "main";
    pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl);
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&pipelineDescriptor);

    wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
    computePassEncoder.SetPipeline(pipeline);
    computePassEncoder.SetBindGroup(0, bindGroup, offsets.size(), offsets.data());
    computePassEncoder.Dispatch(1);
    computePassEncoder.EndPass();

    wgpu::CommandBuffer commands = commandEncoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_BUFFER_U32_RANGE_EQ(values.data(), outputBuffer, 0, values.size());
}

// Test that visibility of bindings in BindGroupLayout can be none
// This test passes by not asserting or crashing.
TEST_P(BindGroupTests, BindGroupLayoutVisibilityCanBeNone) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    wgpu::BindGroupLayoutEntry entry = {0, wgpu::ShaderStage::None,
                                        wgpu::BindingType::UniformBuffer};
    wgpu::BindGroupLayoutDescriptor descriptor;
    descriptor.entryCount = 1;
    descriptor.entries = &entry;
    wgpu::BindGroupLayout layout = device.CreateBindGroupLayout(&descriptor);

    wgpu::RenderPipeline pipeline = MakeTestPipeline(renderPass, {}, {layout});

    std::array<float, 4> color = {1, 0, 0, 1};
    wgpu::Buffer uniformBuffer =
        utils::CreateBufferFromData(device, &color, sizeof(color), wgpu::BufferUsage::Uniform);
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, layout, {{0, uniformBuffer, 0, sizeof(color)}});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(3);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
}

// Regression test for crbug.com/dawn/448 that dynamic buffer bindings can have None visibility.
TEST_P(BindGroupTests, DynamicBindingNoneVisibility) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    wgpu::BindGroupLayoutEntry entry = {0, wgpu::ShaderStage::None,
                                        wgpu::BindingType::UniformBuffer, true};
    wgpu::BindGroupLayoutDescriptor descriptor;
    descriptor.entryCount = 1;
    descriptor.entries = &entry;
    wgpu::BindGroupLayout layout = device.CreateBindGroupLayout(&descriptor);

    wgpu::RenderPipeline pipeline = MakeTestPipeline(renderPass, {}, {layout});

    std::array<float, 4> color = {1, 0, 0, 1};
    wgpu::Buffer uniformBuffer =
        utils::CreateBufferFromData(device, &color, sizeof(color), wgpu::BufferUsage::Uniform);
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, layout, {{0, uniformBuffer, 0, sizeof(color)}});

    uint32_t dynamicOffset = 0;

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup, 1, &dynamicOffset);
    pass.Draw(3);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
}

// Test that bind group bindings may have unbounded and arbitrary binding numbers
TEST_P(BindGroupTests, ArbitraryBindingNumbers) {
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
            gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
        })");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout (set = 0, binding = 953) uniform ubo1 {
            vec4 color1;
        };
        layout (set = 0, binding = 47) uniform ubo2 {
            vec4 color2;
        };
        layout (set = 0, binding = 111) uniform ubo3 {
            vec4 color3;
        };
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = color1 + 2 * color2 + 4 * color3;
        })");

    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
    pipelineDescriptor.vertexStage.module = vsModule;
    pipelineDescriptor.cFragmentStage.module = fsModule;
    pipelineDescriptor.cColorStates[0].format = renderPass.colorFormat;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    wgpu::Buffer black =
        utils::CreateBufferFromData(device, wgpu::BufferUsage::Uniform, {0.f, 0.f, 0.f, 0.f});
    wgpu::Buffer red =
        utils::CreateBufferFromData(device, wgpu::BufferUsage::Uniform, {0.251f, 0.0f, 0.0f, 0.0f});
    wgpu::Buffer green =
        utils::CreateBufferFromData(device, wgpu::BufferUsage::Uniform, {0.0f, 0.251f, 0.0f, 0.0f});
    wgpu::Buffer blue =
        utils::CreateBufferFromData(device, wgpu::BufferUsage::Uniform, {0.0f, 0.0f, 0.251f, 0.0f});

    auto DoTest = [&](wgpu::Buffer color1, wgpu::Buffer color2, wgpu::Buffer color3, RGBA8 filled) {
        auto DoTestInner = [&](wgpu::BindGroup bindGroup) {
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
            pass.SetPipeline(pipeline);
            pass.SetBindGroup(0, bindGroup);
            pass.Draw(3);
            pass.EndPass();

            wgpu::CommandBuffer commands = encoder.Finish();
            queue.Submit(1, &commands);

            EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, 1, 1);
        };

        utils::BindingInitializationHelper bindings[] = {
            {953, color1, 0, 4 * sizeof(float)},  //
            {47, color2, 0, 4 * sizeof(float)},   //
            {111, color3, 0, 4 * sizeof(float)},  //
        };

        // Should work regardless of what order the bindings are specified in.
        DoTestInner(utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                         {bindings[0], bindings[1], bindings[2]}));
        DoTestInner(utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                         {bindings[1], bindings[0], bindings[2]}));
        DoTestInner(utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                         {bindings[2], bindings[0], bindings[1]}));
    };

    // first color is normal, second is 2x, third is 3x.
    DoTest(black, black, black, RGBA8(0, 0, 0, 0));

    // Check the first binding maps to the first slot. We know this because the colors are
    // multiplied 1x.
    DoTest(red, black, black, RGBA8(64, 0, 0, 0));
    DoTest(green, black, black, RGBA8(0, 64, 0, 0));
    DoTest(blue, black, black, RGBA8(0, 0, 64, 0));

    // Use multiple bindings and check the second color maps to the second slot.
    // We know this because the second slot is multiplied 2x.
    DoTest(green, blue, black, RGBA8(0, 64, 128, 0));
    DoTest(blue, green, black, RGBA8(0, 128, 64, 0));
    DoTest(red, green, black, RGBA8(64, 128, 0, 0));

    // Use multiple bindings and check the third color maps to the third slot.
    // We know this because the third slot is multiplied 4x.
    DoTest(black, blue, red, RGBA8(255, 0, 128, 0));
    DoTest(blue, black, green, RGBA8(0, 255, 64, 0));
    DoTest(red, black, blue, RGBA8(64, 0, 255, 0));
}

// This is a regression test for crbug.com/dawn/355 which tests that destruction of a bind group
// that holds the last reference to its bind group layout does not result in a use-after-free. In
// the bug, the destructor of BindGroupBase, when destroying member mLayout,
// Ref<BindGroupLayoutBase> assigns to Ref::mPointee, AFTER calling Release(). After the BGL is
// destroyed, the storage for |mPointee| has been freed.
TEST_P(BindGroupTests, LastReferenceToBindGroupLayout) {
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = sizeof(float);
    bufferDesc.usage = wgpu::BufferUsage::Uniform;
    wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);

    wgpu::BindGroup bg;
    {
        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer}});
        bg = utils::MakeBindGroup(device, bgl, {{0, buffer, 0, sizeof(float)}});
    }
}

// Test that bind groups with an empty bind group layout may be created and used.
TEST_P(BindGroupTests, EmptyLayout) {
    wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(device, {});
    wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {});

    wgpu::ComputePipelineDescriptor pipelineDesc;
    pipelineDesc.layout = utils::MakeBasicPipelineLayout(device, &bgl);
    pipelineDesc.computeStage.entryPoint = "main";
    pipelineDesc.computeStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
        #version 450
        void main() {
        })");

    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&pipelineDesc);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bg);
    pass.Dispatch(1);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
}

// Test creating a BGL with a storage buffer binding but declared readonly in the shader works.
// This is a regression test for crbug.com/dawn/410 which tests that it can successfully compile and
// execute the shader.
TEST_P(BindGroupTests, ReadonlyStorage) {
    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);

    pipelineDescriptor.vertexStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            void main() {
                const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
                gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
            })");

    pipelineDescriptor.cFragmentStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
            #version 450
            layout(set = 0, binding = 0) readonly buffer buffer0 {
                vec4 color;
            };
            layout(location = 0) out vec4 fragColor;
            void main() {
                fragColor = color;
            })");

    constexpr uint32_t kRTSize = 4;
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);
    pipelineDescriptor.cColorStates[0].format = renderPass.colorFormat;

    wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BindingType::StorageBuffer}});

    pipelineDescriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl);

    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

    std::array<float, 4> greenColor = {0, 1, 0, 1};
    wgpu::Buffer storageBuffer = utils::CreateBufferFromData(
        device, &greenColor, sizeof(greenColor), wgpu::BufferUsage::Storage);

    pass.SetPipeline(renderPipeline);
    pass.SetBindGroup(0, utils::MakeBindGroup(device, bgl, {{0, storageBuffer}}));
    pass.Draw(3);
    pass.EndPass();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 0);
}

// Test that creating a large bind group, with each binding type at the max count, works and can be
// used correctly. The test loads a different value from each binding, and writes 1 to a storage
// buffer if all values are correct.
TEST_P(BindGroupTests, ReallyLargeBindGroup) {
    // When we run dawn_end2end_tests with "--use-spvc-parser", extracting the binding type of a
    // read-only image will always return shaderc_spvc_binding_type_writeonly_storage_texture.
    // TODO(jiawei.shao@intel.com): enable this test when we specify "--use-spvc-parser" after the
    // bug in spvc parser is fixed.
    DAWN_SKIP_TEST_IF(IsD3D12() && IsSpvcParserBeingUsed());

    std::string interface = "#version 450\n";
    std::string body;
    uint32_t binding = 0;
    uint32_t expectedValue = 42;

    wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();

    auto CreateTextureWithRedData = [&](uint32_t value, wgpu::TextureUsage usage) {
        wgpu::TextureDescriptor textureDesc = {};
        textureDesc.usage = wgpu::TextureUsage::CopyDst | usage;
        textureDesc.size = {1, 1, 1};
        textureDesc.format = wgpu::TextureFormat::R32Uint;
        wgpu::Texture texture = device.CreateTexture(&textureDesc);

        wgpu::Buffer textureData =
            utils::CreateBufferFromData(device, wgpu::BufferUsage::CopySrc, {expectedValue});
        wgpu::BufferCopyView bufferCopyView = {};
        bufferCopyView.buffer = textureData;
        bufferCopyView.bytesPerRow = 256;

        wgpu::TextureCopyView textureCopyView = {};
        textureCopyView.texture = texture;

        wgpu::Extent3D copySize = {1, 1, 1};

        commandEncoder.CopyBufferToTexture(&bufferCopyView, &textureCopyView, &copySize);
        return texture;
    };

    std::vector<wgpu::BindGroupEntry> bgEntries;
    static_assert(kMaxSampledTexturesPerShaderStage == kMaxSamplersPerShaderStage,
                  "Please update this test");
    body += "result = 0;\n";
    for (uint32_t i = 0; i < kMaxSampledTexturesPerShaderStage; ++i) {
        wgpu::Texture texture =
            CreateTextureWithRedData(expectedValue, wgpu::TextureUsage::Sampled);
        bgEntries.push_back({binding, nullptr, 0, 0, nullptr, texture.CreateView()});

        interface += "layout(set = 0, binding = " + std::to_string(binding++) +
                     ") uniform utexture2D tex" + std::to_string(i) + ";\n";

        wgpu::SamplerDescriptor samplerDesc = {};
        bgEntries.push_back({binding, nullptr, 0, 0, device.CreateSampler(&samplerDesc), nullptr});

        interface += "layout(set = 0, binding = " + std::to_string(binding++) +
                     ") uniform sampler samp" + std::to_string(i) + ";\n";

        body += "if (texelFetch(usampler2D(tex" + std::to_string(i) + ", samp" + std::to_string(i) +
                "), ivec2(0, 0), 0).r != " + std::to_string(expectedValue++) + ") {\n";
        body += "    return;\n";
        body += "}\n";
    }
    for (uint32_t i = 0; i < kMaxStorageTexturesPerShaderStage; ++i) {
        wgpu::Texture texture =
            CreateTextureWithRedData(expectedValue, wgpu::TextureUsage::Storage);
        bgEntries.push_back({binding, nullptr, 0, 0, nullptr, texture.CreateView()});

        interface += "layout(set = 0, binding = " + std::to_string(binding++) +
                     ", r32ui) uniform readonly uimage2D image" + std::to_string(i) + ";\n";

        body += "if (imageLoad(image" + std::to_string(i) +
                ", ivec2(0, 0)).r != " + std::to_string(expectedValue++) + ") {\n";
        body += "    return;\n";
        body += "}\n";
    }
    for (uint32_t i = 0; i < kMaxUniformBuffersPerShaderStage; ++i) {
        wgpu::Buffer buffer = utils::CreateBufferFromData<uint32_t>(
            device, wgpu::BufferUsage::Uniform, {expectedValue, 0, 0, 0});
        bgEntries.push_back({binding, buffer, 0, 4 * sizeof(uint32_t), nullptr, nullptr});

        interface += "layout(std140, set = 0, binding = " + std::to_string(binding++) +
                     ") uniform UBuf" + std::to_string(i) + " {\n";
        interface += "    uint ubuf" + std::to_string(i) + ";\n";
        interface += "};\n";

        body += "if (ubuf" + std::to_string(i) + " != " + std::to_string(expectedValue++) + ") {\n";
        body += "    return;\n";
        body += "}\n";
    }
    // Save one storage buffer for writing the result
    for (uint32_t i = 0; i < kMaxStorageBuffersPerShaderStage - 1; ++i) {
        wgpu::Buffer buffer = utils::CreateBufferFromData<uint32_t>(
            device, wgpu::BufferUsage::Storage, {expectedValue});
        bgEntries.push_back({binding, buffer, 0, sizeof(uint32_t), nullptr, nullptr});

        interface += "layout(std430, set = 0, binding = " + std::to_string(binding++) +
                     ") readonly buffer SBuf" + std::to_string(i) + " {\n";
        interface += "    uint sbuf" + std::to_string(i) + ";\n";
        interface += "};\n";

        body += "if (sbuf" + std::to_string(i) + " != " + std::to_string(expectedValue++) + ") {\n";
        body += "    return;\n";
        body += "}\n";
    }

    wgpu::Buffer result = utils::CreateBufferFromData<uint32_t>(
        device, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc, {0});
    bgEntries.push_back({binding, result, 0, sizeof(uint32_t), nullptr, nullptr});

    interface += "layout(std430, set = 0, binding = " + std::to_string(binding++) +
                 ") writeonly buffer Result {\n";
    interface += "    uint result;\n";
    interface += "};\n";

    body += "result = 1;\n";

    std::string shader = interface + "void main() {\n" + body + "}\n";

    wgpu::ComputePipelineDescriptor cpDesc;
    cpDesc.computeStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, shader.c_str());
    cpDesc.computeStage.entryPoint = "main";
    wgpu::ComputePipeline cp = device.CreateComputePipeline(&cpDesc);

    wgpu::BindGroupDescriptor bgDesc = {};
    bgDesc.layout = cp.GetBindGroupLayout(0);
    bgDesc.entryCount = static_cast<uint32_t>(bgEntries.size());
    bgDesc.entries = bgEntries.data();

    wgpu::BindGroup bg = device.CreateBindGroup(&bgDesc);

    wgpu::ComputePassEncoder pass = commandEncoder.BeginComputePass();
    pass.SetPipeline(cp);
    pass.SetBindGroup(0, bg);
    pass.Dispatch(1, 1, 1);
    pass.EndPass();

    wgpu::CommandBuffer commands = commandEncoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_BUFFER_U32_EQ(1, result, 0);
}

DAWN_INSTANTIATE_TEST(BindGroupTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
