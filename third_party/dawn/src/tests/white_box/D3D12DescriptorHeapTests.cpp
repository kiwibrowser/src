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

#include "dawn_native/Toggles.h"
#include "dawn_native/d3d12/BindGroupLayoutD3D12.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/ShaderVisibleDescriptorAllocatorD3D12.h"
#include "dawn_native/d3d12/StagingDescriptorAllocatorD3D12.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

constexpr uint32_t kRTSize = 4;

// Pooling tests are required to advance the GPU completed serial to reuse heaps.
// This requires Tick() to be called at-least |kFrameDepth| times. This constant
// should be updated if the internals of Tick() change.
constexpr uint32_t kFrameDepth = 2;

using namespace dawn_native::d3d12;

class D3D12DescriptorHeapTests : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();
        DAWN_SKIP_TEST_IF(UsesWire());
        mD3DDevice = reinterpret_cast<Device*>(device.Get());

        mSimpleVSModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
            gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
        })");

        mSimpleFSModule = utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout (location = 0) out vec4 fragColor;
        layout (set = 0, binding = 0) uniform colorBuffer {
            vec4 color;
        };
        void main() {
            fragColor = color;
        })");
    }

    utils::BasicRenderPass MakeRenderPass(uint32_t width,
                                          uint32_t height,
                                          wgpu::TextureFormat format) {
        DAWN_ASSERT(width > 0 && height > 0);

        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = width;
        descriptor.size.height = height;
        descriptor.size.depth = 1;
        descriptor.arrayLayerCount = 1;
        descriptor.sampleCount = 1;
        descriptor.format = format;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::OutputAttachment | wgpu::TextureUsage::CopySrc;
        wgpu::Texture color = device.CreateTexture(&descriptor);

        return utils::BasicRenderPass(width, height, color);
    }

    std::array<float, 4> GetSolidColor(uint32_t n) const {
        ASSERT(n >> 24 == 0);
        float b = (n & 0xFF) / 255.0f;
        float g = ((n >> 8) & 0xFF) / 255.0f;
        float r = ((n >> 16) & 0xFF) / 255.0f;
        return {r, g, b, 1};
    }

    Device* mD3DDevice = nullptr;

    wgpu::ShaderModule mSimpleVSModule;
    wgpu::ShaderModule mSimpleFSModule;
};

class DummyStagingDescriptorAllocator {
  public:
    DummyStagingDescriptorAllocator(Device* device,
                                    uint32_t descriptorCount,
                                    uint32_t allocationsPerHeap)
        : mAllocator(device,
                     descriptorCount,
                     allocationsPerHeap * descriptorCount,
                     D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
    }

    CPUDescriptorHeapAllocation AllocateCPUDescriptors() {
        dawn_native::ResultOrError<CPUDescriptorHeapAllocation> result =
            mAllocator.AllocateCPUDescriptors();
        return (result.IsSuccess()) ? result.AcquireSuccess() : CPUDescriptorHeapAllocation{};
    }

    void Deallocate(CPUDescriptorHeapAllocation& allocation) {
        mAllocator.Deallocate(&allocation);
    }

  private:
    StagingDescriptorAllocator mAllocator;
};

// Verify the shader visible view heaps switch over within a single submit.
TEST_P(D3D12DescriptorHeapTests, SwitchOverViewHeap) {
    DAWN_SKIP_TEST_IF(!mD3DDevice->IsToggleEnabled(
        dawn_native::Toggle::UseD3D12SmallShaderVisibleHeapForTesting));

    utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);

    // Fill in a view heap with "view only" bindgroups (1x view per group) by creating a
    // view bindgroup each draw. After HEAP_SIZE + 1 draws, the heaps must switch over.
    renderPipelineDescriptor.vertexStage.module = mSimpleVSModule;
    renderPipelineDescriptor.cFragmentStage.module = mSimpleFSModule;

    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    Device* d3dDevice = reinterpret_cast<Device*>(device.Get());
    ShaderVisibleDescriptorAllocator* allocator =
        d3dDevice->GetViewShaderVisibleDescriptorAllocator();
    const uint64_t heapSize = allocator->GetShaderVisibleHeapSizeForTesting();

    const Serial heapSerial = allocator->GetShaderVisibleHeapSerialForTesting();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

        pass.SetPipeline(renderPipeline);

        std::array<float, 4> redColor = {1, 0, 0, 1};
        wgpu::Buffer uniformBuffer = utils::CreateBufferFromData(
            device, &redColor, sizeof(redColor), wgpu::BufferUsage::Uniform);

        for (uint32_t i = 0; i < heapSize + 1; ++i) {
            pass.SetBindGroup(0, utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                                                      {{0, uniformBuffer, 0, sizeof(redColor)}}));
            pass.Draw(3);
        }

        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_EQ(allocator->GetShaderVisibleHeapSerialForTesting(), heapSerial + 1);
}

// Verify the shader visible sampler heaps does not switch over within a single submit.
TEST_P(D3D12DescriptorHeapTests, NoSwitchOverSamplerHeap) {
    utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);

    // Fill in a sampler heap with "sampler only" bindgroups (1x sampler per group) by creating a
    // sampler bindgroup each draw. After HEAP_SIZE + 1 draws, the heaps WILL NOT switch over
    // because the sampler heap allocations are de-duplicated.
    renderPipelineDescriptor.vertexStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
            #version 450
            void main() {
                gl_Position = vec4(0.f, 0.f, 0.f, 1.f);
            })");

    renderPipelineDescriptor.cFragmentStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(#version 450
            layout(set = 0, binding = 0) uniform sampler sampler0;
            layout(location = 0) out vec4 fragColor;
            void main() {
               fragColor = vec4(0.0, 0.0, 0.0, 0.0);
            })");

    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

    Device* d3dDevice = reinterpret_cast<Device*>(device.Get());
    ShaderVisibleDescriptorAllocator* allocator =
        d3dDevice->GetSamplerShaderVisibleDescriptorAllocator();
    const uint64_t samplerHeapSize = allocator->GetShaderVisibleHeapSizeForTesting();

    const Serial heapSerial = allocator->GetShaderVisibleHeapSerialForTesting();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

        pass.SetPipeline(renderPipeline);

        for (uint32_t i = 0; i < samplerHeapSize + 1; ++i) {
            pass.SetBindGroup(0, utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                                                      {{0, sampler}}));
            pass.Draw(3);
        }

        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_EQ(allocator->GetShaderVisibleHeapSerialForTesting(), heapSerial);
}

// Verify shader-visible heaps can be recycled for multiple submits.
TEST_P(D3D12DescriptorHeapTests, PoolHeapsInMultipleSubmits) {
    ShaderVisibleDescriptorAllocator* allocator =
        mD3DDevice->GetSamplerShaderVisibleDescriptorAllocator();

    std::list<ComPtr<ID3D12DescriptorHeap>> heaps = {allocator->GetShaderVisibleHeap()};

    EXPECT_EQ(allocator->GetShaderVisiblePoolSizeForTesting(), 0u);

    // Allocate + Tick() up to |kFrameDepth| and ensure heaps are always unique.
    for (uint32_t i = 0; i < kFrameDepth; i++) {
        EXPECT_TRUE(allocator->AllocateAndSwitchShaderVisibleHeap().IsSuccess());
        ComPtr<ID3D12DescriptorHeap> heap = allocator->GetShaderVisibleHeap();
        EXPECT_TRUE(std::find(heaps.begin(), heaps.end(), heap) == heaps.end());
        heaps.push_back(heap);
        mD3DDevice->Tick();
    }

    // Repeat up to |kFrameDepth| again but ensure heaps are the same in the expected order
    // (oldest heaps are recycled first). The "+ 1" is so we also include the very first heap in the
    // check.
    for (uint32_t i = 0; i < kFrameDepth + 1; i++) {
        EXPECT_TRUE(allocator->AllocateAndSwitchShaderVisibleHeap().IsSuccess());
        ComPtr<ID3D12DescriptorHeap> heap = allocator->GetShaderVisibleHeap();
        EXPECT_TRUE(heaps.front() == heap);
        heaps.pop_front();
        mD3DDevice->Tick();
    }

    EXPECT_TRUE(heaps.empty());
    EXPECT_EQ(allocator->GetShaderVisiblePoolSizeForTesting(), kFrameDepth);
}

// Verify shader-visible heaps do not recycle in a pending submit.
TEST_P(D3D12DescriptorHeapTests, PoolHeapsInPendingSubmit) {
    constexpr uint32_t kNumOfSwitches = 5;

    ShaderVisibleDescriptorAllocator* allocator =
        mD3DDevice->GetSamplerShaderVisibleDescriptorAllocator();

    const Serial heapSerial = allocator->GetShaderVisibleHeapSerialForTesting();

    std::set<ComPtr<ID3D12DescriptorHeap>> heaps = {allocator->GetShaderVisibleHeap()};

    EXPECT_EQ(allocator->GetShaderVisiblePoolSizeForTesting(), 0u);

    // Switch-over |kNumOfSwitches| and ensure heaps are always unique.
    for (uint32_t i = 0; i < kNumOfSwitches; i++) {
        EXPECT_TRUE(allocator->AllocateAndSwitchShaderVisibleHeap().IsSuccess());
        ComPtr<ID3D12DescriptorHeap> heap = allocator->GetShaderVisibleHeap();
        EXPECT_TRUE(std::find(heaps.begin(), heaps.end(), heap) == heaps.end());
        heaps.insert(heap);
    }

    // After |kNumOfSwitches|, no heaps are recycled.
    EXPECT_EQ(allocator->GetShaderVisibleHeapSerialForTesting(), heapSerial + kNumOfSwitches);
    EXPECT_EQ(allocator->GetShaderVisiblePoolSizeForTesting(), kNumOfSwitches);
}

// Verify switching shader-visible heaps do not recycle in a pending submit but do so
// once no longer pending.
TEST_P(D3D12DescriptorHeapTests, PoolHeapsInPendingAndMultipleSubmits) {
    constexpr uint32_t kNumOfSwitches = 5;

    ShaderVisibleDescriptorAllocator* allocator =
        mD3DDevice->GetSamplerShaderVisibleDescriptorAllocator();
    const Serial heapSerial = allocator->GetShaderVisibleHeapSerialForTesting();

    std::set<ComPtr<ID3D12DescriptorHeap>> heaps = {allocator->GetShaderVisibleHeap()};

    EXPECT_EQ(allocator->GetShaderVisiblePoolSizeForTesting(), 0u);

    // Switch-over |kNumOfSwitches| to create a pool of unique heaps.
    for (uint32_t i = 0; i < kNumOfSwitches; i++) {
        EXPECT_TRUE(allocator->AllocateAndSwitchShaderVisibleHeap().IsSuccess());
        ComPtr<ID3D12DescriptorHeap> heap = allocator->GetShaderVisibleHeap();
        EXPECT_TRUE(std::find(heaps.begin(), heaps.end(), heap) == heaps.end());
        heaps.insert(heap);
    }

    EXPECT_EQ(allocator->GetShaderVisibleHeapSerialForTesting(), heapSerial + kNumOfSwitches);
    EXPECT_EQ(allocator->GetShaderVisiblePoolSizeForTesting(), kNumOfSwitches);

    // Ensure switched-over heaps can be recycled by advancing the GPU by at-least |kFrameDepth|.
    for (uint32_t i = 0; i < kFrameDepth; i++) {
        mD3DDevice->Tick();
    }

    // Switch-over |kNumOfSwitches| again reusing the same heaps.
    for (uint32_t i = 0; i < kNumOfSwitches; i++) {
        EXPECT_TRUE(allocator->AllocateAndSwitchShaderVisibleHeap().IsSuccess());
        ComPtr<ID3D12DescriptorHeap> heap = allocator->GetShaderVisibleHeap();
        EXPECT_TRUE(std::find(heaps.begin(), heaps.end(), heap) != heaps.end());
        heaps.erase(heap);
    }

    // After switching-over |kNumOfSwitches| x 2, ensure no additional heaps exist.
    EXPECT_EQ(allocator->GetShaderVisibleHeapSerialForTesting(), heapSerial + kNumOfSwitches * 2);
    EXPECT_EQ(allocator->GetShaderVisiblePoolSizeForTesting(), kNumOfSwitches);
}

// Verify encoding multiple heaps worth of bindgroups.
// Shader-visible heaps will switch out |kNumOfHeaps| times.
TEST_P(D3D12DescriptorHeapTests, EncodeManyUBO) {
    // This test draws a solid color triangle |heapSize| times. Each draw uses a new bindgroup that
    // has its own UBO with a "color value" in the range [1... heapSize]. After |heapSize| draws,
    // the result is the arithmetic sum of the sequence after the framebuffer is blended by
    // accumulation. By checking for this sum, we ensure each bindgroup was encoded correctly.
    DAWN_SKIP_TEST_IF(!mD3DDevice->IsToggleEnabled(
        dawn_native::Toggle::UseD3D12SmallShaderVisibleHeapForTesting));

    utils::BasicRenderPass renderPass =
        MakeRenderPass(kRTSize, kRTSize, wgpu::TextureFormat::R32Float);

    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
    pipelineDescriptor.vertexStage.module = mSimpleVSModule;

    pipelineDescriptor.cFragmentStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout (location = 0) out float fragColor;
        layout (set = 0, binding = 0) uniform buffer0 {
            float heapSize;
        };
        void main() {
            fragColor = heapSize;
        })");

    pipelineDescriptor.cColorStates[0].format = wgpu::TextureFormat::R32Float;
    pipelineDescriptor.cColorStates[0].colorBlend.operation = wgpu::BlendOperation::Add;
    pipelineDescriptor.cColorStates[0].colorBlend.srcFactor = wgpu::BlendFactor::One;
    pipelineDescriptor.cColorStates[0].colorBlend.dstFactor = wgpu::BlendFactor::One;
    pipelineDescriptor.cColorStates[0].alphaBlend.operation = wgpu::BlendOperation::Add;
    pipelineDescriptor.cColorStates[0].alphaBlend.srcFactor = wgpu::BlendFactor::One;
    pipelineDescriptor.cColorStates[0].alphaBlend.dstFactor = wgpu::BlendFactor::One;

    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    const uint32_t heapSize =
        mD3DDevice->GetViewShaderVisibleDescriptorAllocator()->GetShaderVisibleHeapSizeForTesting();

    constexpr uint32_t kNumOfHeaps = 2;

    const uint32_t numOfEncodedBindGroups = kNumOfHeaps * heapSize;

    std::vector<wgpu::BindGroup> bindGroups;
    for (uint32_t i = 0; i < numOfEncodedBindGroups; i++) {
        const float color = i + 1;
        wgpu::Buffer uniformBuffer =
            utils::CreateBufferFromData(device, &color, sizeof(color), wgpu::BufferUsage::Uniform);
        bindGroups.push_back(utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                                                  {{0, uniformBuffer}}));
    }

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

        pass.SetPipeline(renderPipeline);

        for (uint32_t i = 0; i < numOfEncodedBindGroups; ++i) {
            pass.SetBindGroup(0, bindGroups[i]);
            pass.Draw(3);
        }

        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    float colorSum = numOfEncodedBindGroups * (numOfEncodedBindGroups + 1) / 2;
    EXPECT_PIXEL_FLOAT_EQ(colorSum, renderPass.color, 0, 0);
}

// Verify encoding one bindgroup then a heaps worth in different submits.
// Shader-visible heaps should switch out once upon encoding 1 + |heapSize| descriptors.
// The first descriptor's memory will be reused when the second submit encodes |heapSize|
// descriptors.
TEST_P(D3D12DescriptorHeapTests, EncodeUBOOverflowMultipleSubmit) {
    DAWN_SKIP_TEST_IF(!mD3DDevice->IsToggleEnabled(
        dawn_native::Toggle::UseD3D12SmallShaderVisibleHeapForTesting));

    utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);

    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
    pipelineDescriptor.vertexStage.module = mSimpleVSModule;
    pipelineDescriptor.cFragmentStage.module = mSimpleFSModule;
    pipelineDescriptor.cColorStates[0].format = renderPass.colorFormat;

    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    // Encode the first descriptor and submit.
    {
        std::array<float, 4> greenColor = {0, 1, 0, 1};
        wgpu::Buffer uniformBuffer = utils::CreateBufferFromData(
            device, &greenColor, sizeof(greenColor), wgpu::BufferUsage::Uniform);

        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, renderPipeline.GetBindGroupLayout(0), {{0, uniformBuffer}});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

            pass.SetPipeline(renderPipeline);
            pass.SetBindGroup(0, bindGroup);
            pass.Draw(3);
            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 0);

    // Encode a heap worth of descriptors.
    {
        const uint32_t heapSize = mD3DDevice->GetSamplerShaderVisibleDescriptorAllocator()
                                      ->GetShaderVisibleHeapSizeForTesting();

        std::vector<wgpu::BindGroup> bindGroups;
        for (uint32_t i = 0; i < heapSize - 1; i++) {
            std::array<float, 4> fillColor = GetSolidColor(i + 1);  // Avoid black
            wgpu::Buffer uniformBuffer = utils::CreateBufferFromData(
                device, &fillColor, sizeof(fillColor), wgpu::BufferUsage::Uniform);

            bindGroups.push_back(utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                                                      {{0, uniformBuffer}}));
        }

        std::array<float, 4> redColor = {1, 0, 0, 1};
        wgpu::Buffer lastUniformBuffer = utils::CreateBufferFromData(
            device, &redColor, sizeof(redColor), wgpu::BufferUsage::Uniform);

        bindGroups.push_back(utils::MakeBindGroup(device, renderPipeline.GetBindGroupLayout(0),
                                                  {{0, lastUniformBuffer, 0, sizeof(redColor)}}));

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

            pass.SetPipeline(renderPipeline);

            for (uint32_t i = 0; i < heapSize; ++i) {
                pass.SetBindGroup(0, bindGroups[i]);
                pass.Draw(3);
            }

            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kRed, renderPass.color, 0, 0);
}

// Verify encoding a heaps worth of bindgroups plus one more then reuse the first
// bindgroup in the same submit.
// Shader-visible heaps should switch out once then re-encode the first descriptor at a new offset
// in the heap.
TEST_P(D3D12DescriptorHeapTests, EncodeReuseUBOOverflow) {
    DAWN_SKIP_TEST_IF(!mD3DDevice->IsToggleEnabled(
        dawn_native::Toggle::UseD3D12SmallShaderVisibleHeapForTesting));

    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
    pipelineDescriptor.vertexStage.module = mSimpleVSModule;
    pipelineDescriptor.cFragmentStage.module = mSimpleFSModule;
    pipelineDescriptor.cColorStates[0].format = renderPass.colorFormat;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    std::array<float, 4> redColor = {1, 0, 0, 1};
    wgpu::Buffer firstUniformBuffer = utils::CreateBufferFromData(
        device, &redColor, sizeof(redColor), wgpu::BufferUsage::Uniform);

    std::vector<wgpu::BindGroup> bindGroups = {utils::MakeBindGroup(
        device, pipeline.GetBindGroupLayout(0), {{0, firstUniformBuffer, 0, sizeof(redColor)}})};

    const uint32_t heapSize =
        mD3DDevice->GetViewShaderVisibleDescriptorAllocator()->GetShaderVisibleHeapSizeForTesting();

    for (uint32_t i = 0; i < heapSize; i++) {
        const std::array<float, 4>& fillColor = GetSolidColor(i + 1);  // Avoid black
        wgpu::Buffer uniformBuffer = utils::CreateBufferFromData(
            device, &fillColor, sizeof(fillColor), wgpu::BufferUsage::Uniform);
        bindGroups.push_back(utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                  {{0, uniformBuffer, 0, sizeof(fillColor)}}));
    }

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

        pass.SetPipeline(pipeline);

        // Encode a heap worth of descriptors plus one more.
        for (uint32_t i = 0; i < heapSize + 1; ++i) {
            pass.SetBindGroup(0, bindGroups[i]);
            pass.Draw(3);
        }

        // Re-encode the first bindgroup again.
        pass.SetBindGroup(0, bindGroups[0]);
        pass.Draw(3);

        pass.EndPass();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Make sure the first bindgroup was encoded correctly.
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kRed, renderPass.color, 0, 0);
}

// Verify encoding a heaps worth of bindgroups plus one more in the first submit then reuse the
// first bindgroup again in the second submit.
// Shader-visible heaps should switch out once then re-encode the
// first descriptor at the same offset in the heap.
TEST_P(D3D12DescriptorHeapTests, EncodeReuseUBOMultipleSubmits) {
    DAWN_SKIP_TEST_IF(!mD3DDevice->IsToggleEnabled(
        dawn_native::Toggle::UseD3D12SmallShaderVisibleHeapForTesting));

    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);
    pipelineDescriptor.vertexStage.module = mSimpleVSModule;
    pipelineDescriptor.cFragmentStage.module = mSimpleFSModule;
    pipelineDescriptor.cColorStates[0].format = renderPass.colorFormat;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    // Encode heap worth of descriptors plus one more.
    std::array<float, 4> redColor = {1, 0, 0, 1};

    wgpu::Buffer firstUniformBuffer = utils::CreateBufferFromData(
        device, &redColor, sizeof(redColor), wgpu::BufferUsage::Uniform);

    std::vector<wgpu::BindGroup> bindGroups = {utils::MakeBindGroup(
        device, pipeline.GetBindGroupLayout(0), {{0, firstUniformBuffer, 0, sizeof(redColor)}})};

    const uint32_t heapSize =
        mD3DDevice->GetViewShaderVisibleDescriptorAllocator()->GetShaderVisibleHeapSizeForTesting();

    for (uint32_t i = 0; i < heapSize; i++) {
        std::array<float, 4> fillColor = GetSolidColor(i + 1);  // Avoid black
        wgpu::Buffer uniformBuffer = utils::CreateBufferFromData(
            device, &fillColor, sizeof(fillColor), wgpu::BufferUsage::Uniform);

        bindGroups.push_back(utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                  {{0, uniformBuffer, 0, sizeof(fillColor)}}));
    }

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

            pass.SetPipeline(pipeline);

            for (uint32_t i = 0; i < heapSize + 1; ++i) {
                pass.SetBindGroup(0, bindGroups[i]);
                pass.Draw(3);
            }

            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

    // Re-encode the first bindgroup again.
    {
        std::array<float, 4> greenColor = {0, 1, 0, 1};
        queue.WriteBuffer(firstUniformBuffer, 0, &greenColor, sizeof(greenColor));

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

            pass.SetPipeline(pipeline);

            pass.SetBindGroup(0, bindGroups[0]);
            pass.Draw(3);

            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

    // Make sure the first bindgroup was re-encoded correctly.
    EXPECT_PIXEL_RGBA8_EQ(RGBA8::kGreen, renderPass.color, 0, 0);
}

// Verify encoding many sampler and ubo worth of bindgroups.
// Shader-visible heaps should switch out |kNumOfViewHeaps| times.
TEST_P(D3D12DescriptorHeapTests, EncodeManyUBOAndSamplers) {
    DAWN_SKIP_TEST_IF(!mD3DDevice->IsToggleEnabled(
        dawn_native::Toggle::UseD3D12SmallShaderVisibleHeapForTesting));

    // Create a solid filled texture.
    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kRTSize;
    descriptor.size.height = kRTSize;
    descriptor.size.depth = 1;
    descriptor.arrayLayerCount = 1;
    descriptor.sampleCount = 1;
    descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.usage = wgpu::TextureUsage::Sampled | wgpu::TextureUsage::OutputAttachment |
                       wgpu::TextureUsage::CopySrc;
    wgpu::Texture texture = device.CreateTexture(&descriptor);
    wgpu::TextureView textureView = texture.CreateView();

    {
        utils::BasicRenderPass renderPass = utils::BasicRenderPass(kRTSize, kRTSize, texture);

        utils::ComboRenderPassDescriptor renderPassDesc({textureView});
        renderPassDesc.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPassDesc.cColorAttachments[0].clearColor = {0.0f, 1.0f, 0.0f, 1.0f};
        renderPass.renderPassInfo.cColorAttachments[0].attachment = textureView;

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        auto pass = encoder.BeginRenderPass(&renderPassDesc);
        pass.EndPass();

        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        queue.Submit(1, &commandBuffer);

        RGBA8 filled(0, 255, 0, 255);
        EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, 0, 0);
    }

    {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor(device);

        pipelineDescriptor.vertexStage.module =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        layout (set = 0, binding = 0) uniform vertexUniformBuffer {
            mat2 transform;
        };
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
            gl_Position = vec4(transform * pos[gl_VertexIndex], 0.f, 1.f);
        })");

        pipelineDescriptor.cFragmentStage.module =
            utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout (set = 0, binding = 1) uniform sampler sampler0;
        layout (set = 0, binding = 2) uniform texture2D texture0;
        layout (set = 0, binding = 3) uniform buffer0 {
            vec4 color;
        };
        layout (location = 0) out vec4 fragColor;
        void main() {
            fragColor = texture(sampler2D(texture0, sampler0), gl_FragCoord.xy);
            fragColor += color;
        })");

        utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);
        pipelineDescriptor.cColorStates[0].format = renderPass.colorFormat;

        wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

        // Encode a heap worth of descriptors |kNumOfHeaps| times.
        constexpr float dummy = 0.0f;
        constexpr float transform[] = {1.f, 0.f, dummy, dummy, 0.f, 1.f, dummy, dummy};
        wgpu::Buffer transformBuffer = utils::CreateBufferFromData(
            device, &transform, sizeof(transform), wgpu::BufferUsage::Uniform);

        wgpu::SamplerDescriptor samplerDescriptor;
        wgpu::Sampler sampler = device.CreateSampler(&samplerDescriptor);

        ShaderVisibleDescriptorAllocator* viewAllocator =
            mD3DDevice->GetViewShaderVisibleDescriptorAllocator();

        ShaderVisibleDescriptorAllocator* samplerAllocator =
            mD3DDevice->GetSamplerShaderVisibleDescriptorAllocator();

        const Serial viewHeapSerial = viewAllocator->GetShaderVisibleHeapSerialForTesting();
        const Serial samplerHeapSerial = samplerAllocator->GetShaderVisibleHeapSerialForTesting();

        const uint32_t viewHeapSize = viewAllocator->GetShaderVisibleHeapSizeForTesting();

        // "Small" view heap is always 2 x sampler heap size and encodes 3x the descriptors per
        // group. This means the count of heaps switches is determined by the total number of views
        // to encode. Compute the number of bindgroups to encode by counting the required views for
        // |kNumOfViewHeaps| heaps worth.
        constexpr uint32_t kViewsPerBindGroup = 3;
        constexpr uint32_t kNumOfViewHeaps = 5;

        const uint32_t numOfEncodedBindGroups =
            (viewHeapSize * kNumOfViewHeaps) / kViewsPerBindGroup;

        std::vector<wgpu::BindGroup> bindGroups;
        for (uint32_t i = 0; i < numOfEncodedBindGroups - 1; i++) {
            std::array<float, 4> fillColor = GetSolidColor(i + 1);  // Avoid black
            wgpu::Buffer uniformBuffer = utils::CreateBufferFromData(
                device, &fillColor, sizeof(fillColor), wgpu::BufferUsage::Uniform);

            bindGroups.push_back(utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                      {{0, transformBuffer, 0, sizeof(transform)},
                                                       {1, sampler},
                                                       {2, textureView},
                                                       {3, uniformBuffer, 0, sizeof(fillColor)}}));
        }

        std::array<float, 4> redColor = {1, 0, 0, 1};
        wgpu::Buffer lastUniformBuffer = utils::CreateBufferFromData(
            device, &redColor, sizeof(redColor), wgpu::BufferUsage::Uniform);

        bindGroups.push_back(utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                  {{0, transformBuffer, 0, sizeof(transform)},
                                                   {1, sampler},
                                                   {2, textureView},
                                                   {3, lastUniformBuffer, 0, sizeof(redColor)}}));

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);

        pass.SetPipeline(pipeline);

        for (uint32_t i = 0; i < numOfEncodedBindGroups; ++i) {
            pass.SetBindGroup(0, bindGroups[i]);
            pass.Draw(3);
        }

        pass.EndPass();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        // Final accumulated color is result of sampled + UBO color.
        RGBA8 filled(255, 255, 0, 255);
        RGBA8 notFilled(0, 0, 0, 0);
        EXPECT_PIXEL_RGBA8_EQ(filled, renderPass.color, 0, 0);
        EXPECT_PIXEL_RGBA8_EQ(notFilled, renderPass.color, kRTSize - 1, 0);

        EXPECT_EQ(viewAllocator->GetShaderVisiblePoolSizeForTesting(), kNumOfViewHeaps);
        EXPECT_EQ(viewAllocator->GetShaderVisibleHeapSerialForTesting(),
                  viewHeapSerial + kNumOfViewHeaps);

        EXPECT_EQ(samplerAllocator->GetShaderVisiblePoolSizeForTesting(), 0u);
        EXPECT_EQ(samplerAllocator->GetShaderVisibleHeapSerialForTesting(), samplerHeapSerial);
    }
}

// Verify a single allocate/deallocate.
// One non-shader visible heap will be created.
TEST_P(D3D12DescriptorHeapTests, Single) {
    constexpr uint32_t kDescriptorCount = 4;
    constexpr uint32_t kAllocationsPerHeap = 3;
    DummyStagingDescriptorAllocator allocator(mD3DDevice, kDescriptorCount, kAllocationsPerHeap);

    CPUDescriptorHeapAllocation allocation = allocator.AllocateCPUDescriptors();
    EXPECT_EQ(allocation.GetHeapIndex(), 0u);
    EXPECT_NE(allocation.OffsetFrom(0, 0).ptr, 0u);

    allocator.Deallocate(allocation);
    EXPECT_FALSE(allocation.IsValid());
}

// Verify allocating many times causes the pool to increase in size.
// Creates |kNumOfHeaps| non-shader visible heaps.
TEST_P(D3D12DescriptorHeapTests, Sequential) {
    constexpr uint32_t kDescriptorCount = 4;
    constexpr uint32_t kAllocationsPerHeap = 3;
    DummyStagingDescriptorAllocator allocator(mD3DDevice, kDescriptorCount, kAllocationsPerHeap);

    // Allocate |kNumOfHeaps| worth.
    constexpr uint32_t kNumOfHeaps = 2;

    std::set<uint32_t> allocatedHeaps;

    std::vector<CPUDescriptorHeapAllocation> allocations;
    for (uint32_t i = 0; i < kAllocationsPerHeap * kNumOfHeaps; i++) {
        CPUDescriptorHeapAllocation allocation = allocator.AllocateCPUDescriptors();
        EXPECT_EQ(allocation.GetHeapIndex(), i / kAllocationsPerHeap);
        EXPECT_NE(allocation.OffsetFrom(0, 0).ptr, 0u);
        allocations.push_back(allocation);
        allocatedHeaps.insert(allocation.GetHeapIndex());
    }

    EXPECT_EQ(allocatedHeaps.size(), kNumOfHeaps);

    // Deallocate all.
    for (CPUDescriptorHeapAllocation& allocation : allocations) {
        allocator.Deallocate(allocation);
        EXPECT_FALSE(allocation.IsValid());
    }
}

// Verify that re-allocating a number of allocations < pool size, all heaps are reused.
// Creates and reuses |kNumofHeaps| non-shader visible heaps.
TEST_P(D3D12DescriptorHeapTests, ReuseFreedHeaps) {
    constexpr uint32_t kDescriptorCount = 4;
    constexpr uint32_t kAllocationsPerHeap = 25;
    DummyStagingDescriptorAllocator allocator(mD3DDevice, kDescriptorCount, kAllocationsPerHeap);

    constexpr uint32_t kNumofHeaps = 10;

    std::list<CPUDescriptorHeapAllocation> allocations;
    std::set<size_t> allocationPtrs;

    // Allocate |kNumofHeaps| heaps worth.
    for (uint32_t i = 0; i < kAllocationsPerHeap * kNumofHeaps; i++) {
        CPUDescriptorHeapAllocation allocation = allocator.AllocateCPUDescriptors();
        allocations.push_back(allocation);
        EXPECT_TRUE(allocationPtrs.insert(allocation.OffsetFrom(0, 0).ptr).second);
    }

    // Deallocate all.
    for (CPUDescriptorHeapAllocation& allocation : allocations) {
        allocator.Deallocate(allocation);
        EXPECT_FALSE(allocation.IsValid());
    }

    allocations.clear();

    // Re-allocate all again.
    std::set<size_t> reallocatedPtrs;
    for (uint32_t i = 0; i < kAllocationsPerHeap * kNumofHeaps; i++) {
        CPUDescriptorHeapAllocation allocation = allocator.AllocateCPUDescriptors();
        allocations.push_back(allocation);
        EXPECT_TRUE(reallocatedPtrs.insert(allocation.OffsetFrom(0, 0).ptr).second);
        EXPECT_TRUE(std::find(allocationPtrs.begin(), allocationPtrs.end(),
                              allocation.OffsetFrom(0, 0).ptr) != allocationPtrs.end());
    }

    // Deallocate all again.
    for (CPUDescriptorHeapAllocation& allocation : allocations) {
        allocator.Deallocate(allocation);
        EXPECT_FALSE(allocation.IsValid());
    }
}

// Verify allocating then deallocating many times.
TEST_P(D3D12DescriptorHeapTests, AllocateDeallocateMany) {
    constexpr uint32_t kDescriptorCount = 4;
    constexpr uint32_t kAllocationsPerHeap = 25;
    DummyStagingDescriptorAllocator allocator(mD3DDevice, kDescriptorCount, kAllocationsPerHeap);

    std::list<CPUDescriptorHeapAllocation> list3;
    std::list<CPUDescriptorHeapAllocation> list5;
    std::list<CPUDescriptorHeapAllocation> allocations;

    constexpr uint32_t kNumofHeaps = 2;

    // Allocate |kNumofHeaps| heaps worth.
    for (uint32_t i = 0; i < kAllocationsPerHeap * kNumofHeaps; i++) {
        CPUDescriptorHeapAllocation allocation = allocator.AllocateCPUDescriptors();
        EXPECT_NE(allocation.OffsetFrom(0, 0).ptr, 0u);
        if (i % 3 == 0) {
            list3.push_back(allocation);
        } else {
            allocations.push_back(allocation);
        }
    }

    // Deallocate every 3rd allocation.
    for (auto it = list3.begin(); it != list3.end(); it = list3.erase(it)) {
        allocator.Deallocate(*it);
    }

    // Allocate again.
    for (uint32_t i = 0; i < kAllocationsPerHeap * kNumofHeaps; i++) {
        CPUDescriptorHeapAllocation allocation = allocator.AllocateCPUDescriptors();
        EXPECT_NE(allocation.OffsetFrom(0, 0).ptr, 0u);
        if (i % 5 == 0) {
            list5.push_back(allocation);
        } else {
            allocations.push_back(allocation);
        }
    }

    // Deallocate every 5th allocation.
    for (auto it = list5.begin(); it != list5.end(); it = list5.erase(it)) {
        allocator.Deallocate(*it);
    }

    // Allocate again.
    for (uint32_t i = 0; i < kAllocationsPerHeap * kNumofHeaps; i++) {
        CPUDescriptorHeapAllocation allocation = allocator.AllocateCPUDescriptors();
        EXPECT_NE(allocation.OffsetFrom(0, 0).ptr, 0u);
        allocations.push_back(allocation);
    }

    // Deallocate remaining.
    for (CPUDescriptorHeapAllocation& allocation : allocations) {
        allocator.Deallocate(allocation);
        EXPECT_FALSE(allocation.IsValid());
    }
}

DAWN_INSTANTIATE_TEST(D3D12DescriptorHeapTests,
                      D3D12Backend(),
                      D3D12Backend({"use_d3d12_small_shader_visible_heap"}));
