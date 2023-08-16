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

#include "SampleUtils.h"

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/SystemUtils.h"
#include "utils/WGPUHelpers.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

wgpu::Device device;

wgpu::Buffer indexBuffer;
wgpu::Buffer vertexBuffer;
wgpu::Buffer planeBuffer;
wgpu::Buffer cameraBuffer;
wgpu::Buffer transformBuffer[2];

wgpu::BindGroup cameraBindGroup;
wgpu::BindGroup bindGroup[2];
wgpu::BindGroup cubeTransformBindGroup[2];

wgpu::Queue queue;
wgpu::SwapChain swapchain;
wgpu::TextureView depthStencilView;
wgpu::RenderPipeline pipeline;
wgpu::RenderPipeline planePipeline;
wgpu::RenderPipeline reflectionPipeline;

void initBuffers() {
    static const uint32_t indexData[6 * 6] = {0,  1,  2,  0,  2,  3,

                                              4,  5,  6,  4,  6,  7,

                                              8,  9,  10, 8,  10, 11,

                                              12, 13, 14, 12, 14, 15,

                                              16, 17, 18, 16, 18, 19,

                                              20, 21, 22, 20, 22, 23};
    indexBuffer =
        utils::CreateBufferFromData(device, indexData, sizeof(indexData), wgpu::BufferUsage::Index);

    static const float vertexData[6 * 4 * 6] = {
        -1.0, -1.0, 1.0,  1.0, 0.0, 0.0, 1.0,  -1.0, 1.0,  1.0, 0.0, 0.0,
        1.0,  1.0,  1.0,  1.0, 0.0, 0.0, -1.0, 1.0,  1.0,  1.0, 0.0, 0.0,

        -1.0, -1.0, -1.0, 1.0, 1.0, 0.0, -1.0, 1.0,  -1.0, 1.0, 1.0, 0.0,
        1.0,  1.0,  -1.0, 1.0, 1.0, 0.0, 1.0,  -1.0, -1.0, 1.0, 1.0, 0.0,

        -1.0, 1.0,  -1.0, 1.0, 0.0, 1.0, -1.0, 1.0,  1.0,  1.0, 0.0, 1.0,
        1.0,  1.0,  1.0,  1.0, 0.0, 1.0, 1.0,  1.0,  -1.0, 1.0, 0.0, 1.0,

        -1.0, -1.0, -1.0, 0.0, 1.0, 0.0, 1.0,  -1.0, -1.0, 0.0, 1.0, 0.0,
        1.0,  -1.0, 1.0,  0.0, 1.0, 0.0, -1.0, -1.0, 1.0,  0.0, 1.0, 0.0,

        1.0,  -1.0, -1.0, 0.0, 1.0, 1.0, 1.0,  1.0,  -1.0, 0.0, 1.0, 1.0,
        1.0,  1.0,  1.0,  0.0, 1.0, 1.0, 1.0,  -1.0, 1.0,  0.0, 1.0, 1.0,

        -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0,  1.0, 1.0, 1.0,
        -1.0, 1.0,  1.0,  1.0, 1.0, 1.0, -1.0, 1.0,  -1.0, 1.0, 1.0, 1.0};
    vertexBuffer = utils::CreateBufferFromData(device, vertexData, sizeof(vertexData),
                                               wgpu::BufferUsage::Vertex);

    static const float planeData[6 * 4] = {
        -2.0, -1.0, -2.0, 0.5, 0.5, 0.5, 2.0,  -1.0, -2.0, 0.5, 0.5, 0.5,
        2.0,  -1.0, 2.0,  0.5, 0.5, 0.5, -2.0, -1.0, 2.0,  0.5, 0.5, 0.5,
    };
    planeBuffer = utils::CreateBufferFromData(device, planeData, sizeof(planeData),
                                              wgpu::BufferUsage::Vertex);
}

struct CameraData {
    glm::mat4 view;
    glm::mat4 proj;
} cameraData;

void init() {
    device = CreateCppDawnDevice();

    queue = device.GetDefaultQueue();
    swapchain = GetSwapChain(device);
    swapchain.Configure(GetPreferredSwapChainTextureFormat(), wgpu::TextureUsage::OutputAttachment,
                        640, 480);

    initBuffers();

    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(set = 0, binding = 0) uniform cameraData {
            mat4 view;
            mat4 proj;
        } camera;
        layout(set = 0, binding = 1) uniform modelData {
            mat4 modelMatrix;
        };
        layout(location = 0) in vec3 pos;
        layout(location = 1) in vec3 col;
        layout(location = 2) out vec3 f_col;
        void main() {
            f_col = col;
            gl_Position = camera.proj * camera.view * modelMatrix * vec4(pos, 1.0);
        })");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(location = 2) in vec3 f_col;
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = vec4(f_col, 1.0);
        })");

    wgpu::ShaderModule fsReflectionModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(location = 2) in vec3 f_col;
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = vec4(mix(f_col, vec3(0.5, 0.5, 0.5), 0.5), 1.0);
        })");

    utils::ComboVertexStateDescriptor vertexState;
    vertexState.cVertexBuffers[0].attributeCount = 2;
    vertexState.cAttributes[0].format = wgpu::VertexFormat::Float3;
    vertexState.cAttributes[1].shaderLocation = 1;
    vertexState.cAttributes[1].offset = 3 * sizeof(float);
    vertexState.cAttributes[1].format = wgpu::VertexFormat::Float3;

    vertexState.vertexBufferCount = 1;
    vertexState.cVertexBuffers[0].arrayStride = 6 * sizeof(float);

    auto bgl = utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                    {1, wgpu::ShaderStage::Vertex, wgpu::BindingType::UniformBuffer},
                });

    wgpu::PipelineLayout pl = utils::MakeBasicPipelineLayout(device, &bgl);

    wgpu::BufferDescriptor cameraBufDesc;
    cameraBufDesc.size = sizeof(CameraData);
    cameraBufDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    cameraBuffer = device.CreateBuffer(&cameraBufDesc);

    glm::mat4 transform(1.0);
    transformBuffer[0] = utils::CreateBufferFromData(device, &transform, sizeof(glm::mat4),
                                                     wgpu::BufferUsage::Uniform);

    transform = glm::translate(transform, glm::vec3(0.f, -2.f, 0.f));
    transformBuffer[1] = utils::CreateBufferFromData(device, &transform, sizeof(glm::mat4),
                                                     wgpu::BufferUsage::Uniform);

    bindGroup[0] = utils::MakeBindGroup(
        device, bgl,
        {{0, cameraBuffer, 0, sizeof(CameraData)}, {1, transformBuffer[0], 0, sizeof(glm::mat4)}});

    bindGroup[1] = utils::MakeBindGroup(
        device, bgl,
        {{0, cameraBuffer, 0, sizeof(CameraData)}, {1, transformBuffer[1], 0, sizeof(glm::mat4)}});

    depthStencilView = CreateDefaultDepthStencilView(device);

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.layout = pl;
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;
    descriptor.vertexState = &vertexState;
    descriptor.depthStencilState = &descriptor.cDepthStencilState;
    descriptor.cDepthStencilState.format = wgpu::TextureFormat::Depth24PlusStencil8;
    descriptor.cColorStates[0].format = GetPreferredSwapChainTextureFormat();
    descriptor.cDepthStencilState.depthWriteEnabled = true;
    descriptor.cDepthStencilState.depthCompare = wgpu::CompareFunction::Less;

    pipeline = device.CreateRenderPipeline(&descriptor);

    utils::ComboRenderPipelineDescriptor pDescriptor(device);
    pDescriptor.layout = pl;
    pDescriptor.vertexStage.module = vsModule;
    pDescriptor.cFragmentStage.module = fsModule;
    pDescriptor.vertexState = &vertexState;
    pDescriptor.depthStencilState = &pDescriptor.cDepthStencilState;
    pDescriptor.cDepthStencilState.format = wgpu::TextureFormat::Depth24PlusStencil8;
    pDescriptor.cColorStates[0].format = GetPreferredSwapChainTextureFormat();
    pDescriptor.cDepthStencilState.stencilFront.passOp = wgpu::StencilOperation::Replace;
    pDescriptor.cDepthStencilState.stencilBack.passOp = wgpu::StencilOperation::Replace;
    pDescriptor.cDepthStencilState.depthCompare = wgpu::CompareFunction::Less;

    planePipeline = device.CreateRenderPipeline(&pDescriptor);

    utils::ComboRenderPipelineDescriptor rfDescriptor(device);
    rfDescriptor.layout = pl;
    rfDescriptor.vertexStage.module = vsModule;
    rfDescriptor.cFragmentStage.module = fsReflectionModule;
    rfDescriptor.vertexState = &vertexState;
    rfDescriptor.depthStencilState = &rfDescriptor.cDepthStencilState;
    rfDescriptor.cDepthStencilState.format = wgpu::TextureFormat::Depth24PlusStencil8;
    rfDescriptor.cColorStates[0].format = GetPreferredSwapChainTextureFormat();
    rfDescriptor.cDepthStencilState.stencilFront.compare = wgpu::CompareFunction::Equal;
    rfDescriptor.cDepthStencilState.stencilBack.compare = wgpu::CompareFunction::Equal;
    rfDescriptor.cDepthStencilState.stencilFront.passOp = wgpu::StencilOperation::Replace;
    rfDescriptor.cDepthStencilState.stencilBack.passOp = wgpu::StencilOperation::Replace;
    rfDescriptor.cDepthStencilState.depthWriteEnabled = true;
    rfDescriptor.cDepthStencilState.depthCompare = wgpu::CompareFunction::Less;

    reflectionPipeline = device.CreateRenderPipeline(&rfDescriptor);

    cameraData.proj = glm::perspective(glm::radians(45.0f), 1.f, 1.0f, 100.0f);
}

struct {
    uint32_t a;
    float b;
} s;
void frame() {
    s.a = (s.a + 1) % 256;
    s.b += 0.01f;
    if (s.b >= 1.0f) {
        s.b = 0.0f;
    }

    cameraData.view = glm::lookAt(glm::vec3(8.f * std::sin(glm::radians(s.b * 360.f)), 2.f,
                                            8.f * std::cos(glm::radians(s.b * 360.f))),
                                  glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    queue.WriteBuffer(cameraBuffer, 0, &cameraData, sizeof(CameraData));

    wgpu::TextureView backbufferView = swapchain.GetCurrentTextureView();
    utils::ComboRenderPassDescriptor renderPass({backbufferView}, depthStencilView);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup[0]);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetIndexBuffer(indexBuffer);
        pass.DrawIndexed(36);

        pass.SetStencilReference(0x1);
        pass.SetPipeline(planePipeline);
        pass.SetBindGroup(0, bindGroup[0]);
        pass.SetVertexBuffer(0, planeBuffer);
        pass.DrawIndexed(6);

        pass.SetPipeline(reflectionPipeline);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetBindGroup(0, bindGroup[1]);
        pass.DrawIndexed(36);

        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
    swapchain.Present();
    DoFlush();
}

int main(int argc, const char* argv[]) {
    if (!InitSample(argc, argv)) {
        return 1;
    }
    init();

    while (!ShouldQuit()) {
        frame();
        utils::USleep(16000);
    }

    // TODO release stuff
}
