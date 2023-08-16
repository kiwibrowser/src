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

#include <array>
#include <cstring>
#include <random>

#include <glm/glm.hpp>

wgpu::Device device;
wgpu::Queue queue;
wgpu::SwapChain swapchain;
wgpu::TextureView depthStencilView;

wgpu::Buffer modelBuffer;
std::array<wgpu::Buffer, 2> particleBuffers;

wgpu::RenderPipeline renderPipeline;

wgpu::Buffer updateParams;
wgpu::ComputePipeline updatePipeline;
std::array<wgpu::BindGroup, 2> updateBGs;

size_t pingpong = 0;

static const uint32_t kNumParticles = 1000;

struct Particle {
    glm::vec2 pos;
    glm::vec2 vel;
};

struct SimParams {
    float deltaT;
    float rule1Distance;
    float rule2Distance;
    float rule3Distance;
    float rule1Scale;
    float rule2Scale;
    float rule3Scale;
    int particleCount;
};

void initBuffers() {
    glm::vec2 model[3] = {
        {-0.01, -0.02},
        {0.01, -0.02},
        {0.00, 0.02},
    };
    modelBuffer =
        utils::CreateBufferFromData(device, model, sizeof(model), wgpu::BufferUsage::Vertex);

    SimParams params = {0.04f, 0.1f, 0.025f, 0.025f, 0.02f, 0.05f, 0.005f, kNumParticles};
    updateParams =
        utils::CreateBufferFromData(device, &params, sizeof(params), wgpu::BufferUsage::Uniform);

    std::vector<Particle> initialParticles(kNumParticles);
    {
        std::mt19937 generator;
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (auto& p : initialParticles) {
            p.pos = glm::vec2(dist(generator), dist(generator));
            p.vel = glm::vec2(dist(generator), dist(generator)) * 0.1f;
        }
    }

    for (size_t i = 0; i < 2; i++) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = sizeof(Particle) * kNumParticles;
        descriptor.usage =
            wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Storage;
        particleBuffers[i] = device.CreateBuffer(&descriptor);

        queue.WriteBuffer(particleBuffers[i], 0,
                          reinterpret_cast<uint8_t*>(initialParticles.data()),
                          sizeof(Particle) * kNumParticles);
    }
}

void initRender() {
    wgpu::ShaderModule vsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout(location = 0) in vec2 a_particlePos;
        layout(location = 1) in vec2 a_particleVel;
        layout(location = 2) in vec2 a_pos;
        void main() {
            float angle = -atan(a_particleVel.x, a_particleVel.y);
            vec2 pos = vec2(a_pos.x * cos(angle) - a_pos.y * sin(angle),
                            a_pos.x * sin(angle) + a_pos.y * cos(angle));
            gl_Position = vec4(pos + a_particlePos, 0, 1);
        }
    )");

    wgpu::ShaderModule fsModule =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = vec4(1.0);
        }
    )");

    depthStencilView = CreateDefaultDepthStencilView(device);

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.vertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    descriptor.cVertexState.vertexBufferCount = 2;
    descriptor.cVertexState.cVertexBuffers[0].arrayStride = sizeof(Particle);
    descriptor.cVertexState.cVertexBuffers[0].stepMode = wgpu::InputStepMode::Instance;
    descriptor.cVertexState.cVertexBuffers[0].attributeCount = 2;
    descriptor.cVertexState.cAttributes[0].offset = offsetof(Particle, pos);
    descriptor.cVertexState.cAttributes[0].format = wgpu::VertexFormat::Float2;
    descriptor.cVertexState.cAttributes[1].shaderLocation = 1;
    descriptor.cVertexState.cAttributes[1].offset = offsetof(Particle, vel);
    descriptor.cVertexState.cAttributes[1].format = wgpu::VertexFormat::Float2;
    descriptor.cVertexState.cVertexBuffers[1].arrayStride = sizeof(glm::vec2);
    descriptor.cVertexState.cVertexBuffers[1].attributeCount = 1;
    descriptor.cVertexState.cVertexBuffers[1].attributes = &descriptor.cVertexState.cAttributes[2];
    descriptor.cVertexState.cAttributes[2].shaderLocation = 2;
    descriptor.cVertexState.cAttributes[2].format = wgpu::VertexFormat::Float2;
    descriptor.depthStencilState = &descriptor.cDepthStencilState;
    descriptor.cDepthStencilState.format = wgpu::TextureFormat::Depth24PlusStencil8;
    descriptor.cColorStates[0].format = GetPreferredSwapChainTextureFormat();

    renderPipeline = device.CreateRenderPipeline(&descriptor);
}

void initSim() {
    wgpu::ShaderModule module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Compute, R"(
        #version 450

        struct Particle {
            vec2 pos;
            vec2 vel;
        };

        layout(std140, set = 0, binding = 0) uniform SimParams {
            float deltaT;
            float rule1Distance;
            float rule2Distance;
            float rule3Distance;
            float rule1Scale;
            float rule2Scale;
            float rule3Scale;
            int particleCount;
        } params;

        layout(std140, set = 0, binding = 1) buffer ParticlesA {
            Particle particles[1000];
        } particlesA;

        layout(std140, set = 0, binding = 2) buffer ParticlesB {
            Particle particles[1000];
        } particlesB;

        void main() {
            // https://github.com/austinEng/Project6-Vulkan-Flocking/blob/master/data/shaders/computeparticles/particle.comp

            uint index = gl_GlobalInvocationID.x;
            if (index >= params.particleCount) { return; }

            vec2 vPos = particlesA.particles[index].pos;
            vec2 vVel = particlesA.particles[index].vel;

            vec2 cMass = vec2(0.0, 0.0);
            vec2 cVel = vec2(0.0, 0.0);
            vec2 colVel = vec2(0.0, 0.0);
            int cMassCount = 0;
            int cVelCount = 0;

            vec2 pos;
            vec2 vel;
            for (int i = 0; i < params.particleCount; ++i) {
                if (i == index) { continue; }
                pos = particlesA.particles[i].pos.xy;
                vel = particlesA.particles[i].vel.xy;

                if (distance(pos, vPos) < params.rule1Distance) {
                    cMass += pos;
                    cMassCount++;
                }
                if (distance(pos, vPos) < params.rule2Distance) {
                    colVel -= (pos - vPos);
                }
                if (distance(pos, vPos) < params.rule3Distance) {
                    cVel += vel;
                    cVelCount++;
                }
            }
            if (cMassCount > 0) {
                cMass = cMass / cMassCount - vPos;
            }
            if (cVelCount > 0) {
                cVel = cVel / cVelCount;
            }

            vVel += cMass * params.rule1Scale + colVel * params.rule2Scale + cVel * params.rule3Scale;

            // clamp velocity for a more pleasing simulation.
            vVel = normalize(vVel) * clamp(length(vVel), 0.0, 0.1);

            // kinematic update
            vPos += vVel * params.deltaT;

            // Wrap around boundary
            if (vPos.x < -1.0) vPos.x = 1.0;
            if (vPos.x > 1.0) vPos.x = -1.0;
            if (vPos.y < -1.0) vPos.y = 1.0;
            if (vPos.y > 1.0) vPos.y = -1.0;

            particlesB.particles[index].pos = vPos;

            // Write back
            particlesB.particles[index].vel = vVel;
        }
    )");

    auto bgl = utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BindingType::UniformBuffer},
                    {1, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer},
                    {2, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer},
                });

    wgpu::PipelineLayout pl = utils::MakeBasicPipelineLayout(device, &bgl);

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.layout = pl;
    csDesc.computeStage.module = module;
    csDesc.computeStage.entryPoint = "main";
    updatePipeline = device.CreateComputePipeline(&csDesc);

    for (uint32_t i = 0; i < 2; ++i) {
        updateBGs[i] = utils::MakeBindGroup(
            device, bgl,
            {
                {0, updateParams, 0, sizeof(SimParams)},
                {1, particleBuffers[i], 0, kNumParticles * sizeof(Particle)},
                {2, particleBuffers[(i + 1) % 2], 0, kNumParticles * sizeof(Particle)},
            });
    }
}

wgpu::CommandBuffer createCommandBuffer(const wgpu::TextureView backbufferView, size_t i) {
    auto& bufferDst = particleBuffers[(i + 1) % 2];
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    {
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(updatePipeline);
        pass.SetBindGroup(0, updateBGs[i]);
        pass.Dispatch(kNumParticles);
        pass.EndPass();
    }

    {
        utils::ComboRenderPassDescriptor renderPass({backbufferView}, depthStencilView);
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(renderPipeline);
        pass.SetVertexBuffer(0, bufferDst);
        pass.SetVertexBuffer(1, modelBuffer);
        pass.Draw(3, kNumParticles);
        pass.EndPass();
    }

    return encoder.Finish();
}

void init() {
    device = CreateCppDawnDevice();

    queue = device.GetDefaultQueue();
    swapchain = GetSwapChain(device);
    swapchain.Configure(GetPreferredSwapChainTextureFormat(), wgpu::TextureUsage::OutputAttachment,
                        640, 480);

    initBuffers();
    initRender();
    initSim();
}

void frame() {
    wgpu::TextureView backbufferView = swapchain.GetCurrentTextureView();

    wgpu::CommandBuffer commandBuffer = createCommandBuffer(backbufferView, pingpong);
    queue.Submit(1, &commandBuffer);
    swapchain.Present();
    DoFlush();

    pingpong = (pingpong + 1) % 2;
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
