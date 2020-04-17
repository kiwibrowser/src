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
#include "utils/DawnHelpers.h"
#include "utils/SystemUtils.h"

#include <array>
#include <cstring>
#include <random>

#include <glm/glm.hpp>

dawn::Device device;
dawn::Queue queue;
dawn::SwapChain swapchain;
dawn::TextureView depthStencilView;

dawn::Buffer modelBuffer;
std::array<dawn::Buffer, 2> particleBuffers;

dawn::RenderPipeline renderPipeline;

dawn::Buffer updateParams;
dawn::ComputePipeline updatePipeline;
std::array<dawn::BindGroup, 2> updateBGs;

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
    modelBuffer = utils::CreateBufferFromData(device, model, sizeof(model), dawn::BufferUsageBit::Vertex);

    SimParams params = { 0.04f, 0.1f, 0.025f, 0.025f, 0.02f, 0.05f, 0.005f, kNumParticles };
    updateParams = utils::CreateBufferFromData(device, &params, sizeof(params), dawn::BufferUsageBit::Uniform);

    std::vector<Particle> initialParticles(kNumParticles);
    {
        std::mt19937 generator;
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (auto& p : initialParticles)
        {
            p.pos = glm::vec2(dist(generator), dist(generator));
            p.vel = glm::vec2(dist(generator), dist(generator)) * 0.1f;
        }
    }

    for (size_t i = 0; i < 2; i++) {
        dawn::BufferDescriptor descriptor;
        descriptor.size = sizeof(Particle) * kNumParticles;
        descriptor.usage = dawn::BufferUsageBit::TransferDst | dawn::BufferUsageBit::Vertex | dawn::BufferUsageBit::Storage;
        particleBuffers[i] = device.CreateBuffer(&descriptor);

        particleBuffers[i].SetSubData(0,
            sizeof(Particle) * kNumParticles,
            reinterpret_cast<uint8_t*>(initialParticles.data()));
    }
}

void initRender() {
    dawn::ShaderModule vsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Vertex, R"(
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

    dawn::ShaderModule fsModule = utils::CreateShaderModule(device, dawn::ShaderStage::Fragment, R"(
        #version 450
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = vec4(1.0);
        }
    )");

    depthStencilView = CreateDefaultDepthStencilView(device);

    utils::ComboRenderPipelineDescriptor descriptor(device);
    descriptor.cVertexStage.module = vsModule;
    descriptor.cFragmentStage.module = fsModule;

    descriptor.cVertexInput.bufferCount = 2;
    descriptor.cVertexInput.cBuffers[0].stride = sizeof(Particle);
    descriptor.cVertexInput.cBuffers[0].stepMode = dawn::InputStepMode::Instance;
    descriptor.cVertexInput.cBuffers[0].attributeCount = 2;
    descriptor.cVertexInput.cAttributes[0].offset = offsetof(Particle, pos);
    descriptor.cVertexInput.cAttributes[0].format = dawn::VertexFormat::Float2;
    descriptor.cVertexInput.cAttributes[1].shaderLocation = 1;
    descriptor.cVertexInput.cAttributes[1].offset = offsetof(Particle, vel);
    descriptor.cVertexInput.cAttributes[1].format = dawn::VertexFormat::Float2;
    descriptor.cVertexInput.cBuffers[1].stride = sizeof(glm::vec2);
    descriptor.cVertexInput.cBuffers[1].attributeCount = 1;
    descriptor.cVertexInput.cBuffers[1].attributes = &descriptor.cVertexInput.cAttributes[2];
    descriptor.cVertexInput.cAttributes[2].shaderLocation = 2;
    descriptor.cVertexInput.cAttributes[2].format = dawn::VertexFormat::Float2;
    descriptor.depthStencilState = &descriptor.cDepthStencilState;
    descriptor.cDepthStencilState.format = dawn::TextureFormat::D32FloatS8Uint;
    descriptor.cColorStates[0]->format = GetPreferredSwapChainTextureFormat();

    renderPipeline = device.CreateRenderPipeline(&descriptor);
}

void initSim() {
    dawn::ShaderModule module = utils::CreateShaderModule(device, dawn::ShaderStage::Compute, R"(
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
                    {0, dawn::ShaderStageBit::Compute, dawn::BindingType::UniformBuffer},
                    {1, dawn::ShaderStageBit::Compute, dawn::BindingType::StorageBuffer},
                    {2, dawn::ShaderStageBit::Compute, dawn::BindingType::StorageBuffer},
                });

    dawn::PipelineLayout pl = utils::MakeBasicPipelineLayout(device, &bgl);

    dawn::ComputePipelineDescriptor csDesc;
    csDesc.layout = pl;

    dawn::PipelineStageDescriptor computeStage;
    computeStage.module = module;
    computeStage.entryPoint = "main";
    csDesc.computeStage = &computeStage;

    updatePipeline = device.CreateComputePipeline(&csDesc);

    for (uint32_t i = 0; i < 2; ++i) {
        updateBGs[i] = utils::MakeBindGroup(device, bgl, {
            {0, updateParams, 0, sizeof(SimParams)},
            {1, particleBuffers[i], 0, kNumParticles * sizeof(Particle)},
            {2, particleBuffers[(i + 1) % 2], 0, kNumParticles * sizeof(Particle)},
        });
    }
}

dawn::CommandBuffer createCommandBuffer(const dawn::Texture backbuffer, size_t i) {
    static const uint64_t zeroOffsets[1] = {0};
    auto& bufferDst = particleBuffers[(i + 1) % 2];
    dawn::CommandEncoder encoder = device.CreateCommandEncoder();

    {
        dawn::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(updatePipeline);
        pass.SetBindGroup(0, updateBGs[i], 0, nullptr);
        pass.Dispatch(kNumParticles, 1, 1);
        pass.EndPass();
    }

    {
        utils::ComboRenderPassDescriptor renderPass({backbuffer.CreateDefaultView()},
                                                    depthStencilView);
        dawn::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(renderPipeline);
        pass.SetVertexBuffers(0, 1, &bufferDst, zeroOffsets);
        pass.SetVertexBuffers(1, 1, &modelBuffer, zeroOffsets);
        pass.Draw(3, kNumParticles, 0, 0);
        pass.EndPass();
    }

    return encoder.Finish();
}

void init() {
    device = CreateCppDawnDevice();

    queue = device.CreateQueue();
    swapchain = GetSwapChain(device);
    swapchain.Configure(GetPreferredSwapChainTextureFormat(),
                        dawn::TextureUsageBit::OutputAttachment, 640, 480);

    initBuffers();
    initRender();
    initSim();
}

void frame() {
    dawn::Texture backbuffer = swapchain.GetNextTexture();

    dawn::CommandBuffer commandBuffer = createCommandBuffer(backbuffer, pingpong);
    queue.Submit(1, &commandBuffer);
    swapchain.Present(backbuffer);
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
