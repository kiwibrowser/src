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

#include "dawn_native/D3D12Backend.h"
#include "dawn_native/d3d12/BufferD3D12.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/ResidencyManagerD3D12.h"
#include "dawn_native/d3d12/ShaderVisibleDescriptorAllocatorD3D12.h"
#include "tests/DawnTest.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

#include <vector>

constexpr uint32_t kRestrictedBudgetSize = 100000000;         // 100MB
constexpr uint32_t kDirectlyAllocatedResourceSize = 5000000;  // 5MB
constexpr uint32_t kSuballocatedResourceSize = 1000000;       // 1MB
constexpr uint32_t kSourceBufferSize = 4;                     // 4B

constexpr wgpu::BufferUsage kMapReadBufferUsage =
    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
constexpr wgpu::BufferUsage kMapWriteBufferUsage =
    wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
constexpr wgpu::BufferUsage kNonMappableBufferUsage = wgpu::BufferUsage::CopyDst;

class D3D12ResidencyTestBase : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();
        DAWN_SKIP_TEST_IF(UsesWire());

        // Restrict Dawn's budget to create an artificial budget.
        dawn_native::d3d12::Device* d3dDevice =
            reinterpret_cast<dawn_native::d3d12::Device*>(device.Get());
        d3dDevice->GetResidencyManager()->RestrictBudgetForTesting(kRestrictedBudgetSize);

        // Initialize a source buffer on the GPU to serve as a source to quickly copy data to other
        // buffers.
        constexpr uint32_t one = 1;
        mSourceBuffer =
            utils::CreateBufferFromData(device, &one, sizeof(one), wgpu::BufferUsage::CopySrc);
    }

    std::vector<wgpu::Buffer> AllocateBuffers(uint32_t bufferSize,
                                              uint32_t numberOfBuffers,
                                              wgpu::BufferUsage usage) {
        std::vector<wgpu::Buffer> buffers;

        for (uint64_t i = 0; i < numberOfBuffers; i++) {
            buffers.push_back(CreateBuffer(bufferSize, usage));
        }

        return buffers;
    }

    wgpu::Buffer CreateBuffer(uint32_t bufferSize, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor descriptor;

        descriptor.size = bufferSize;
        descriptor.usage = usage;

        return device.CreateBuffer(&descriptor);
    }

    void TouchBuffers(uint32_t beginIndex,
                      uint32_t numBuffers,
                      const std::vector<wgpu::Buffer>& bufferSet) {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        // Perform a copy on the range of buffers to ensure the are moved to dedicated GPU memory.
        for (uint32_t i = beginIndex; i < beginIndex + numBuffers; i++) {
            encoder.CopyBufferToBuffer(mSourceBuffer, 0, bufferSet[i], 0, kSourceBufferSize);
        }
        wgpu::CommandBuffer copy = encoder.Finish();
        queue.Submit(1, &copy);
    }

    wgpu::Buffer mSourceBuffer;
    void* mMappedWriteData = nullptr;
    const void* mMappedReadData = nullptr;
};

class D3D12ResourceResidencyTests : public D3D12ResidencyTestBase {
  protected:
    bool CheckAllocationMethod(wgpu::Buffer buffer,
                               dawn_native::AllocationMethod allocationMethod) const {
        dawn_native::d3d12::Buffer* d3dBuffer =
            reinterpret_cast<dawn_native::d3d12::Buffer*>(buffer.Get());
        return d3dBuffer->CheckAllocationMethodForTesting(allocationMethod);
    }

    bool CheckIfBufferIsResident(wgpu::Buffer buffer) const {
        dawn_native::d3d12::Buffer* d3dBuffer =
            reinterpret_cast<dawn_native::d3d12::Buffer*>(buffer.Get());
        return d3dBuffer->CheckIsResidentForTesting();
    }

    bool IsUMA() const {
        return reinterpret_cast<dawn_native::d3d12::Device*>(device.Get())->GetDeviceInfo().isUMA;
    }

    static void MapReadCallback(WGPUBufferMapAsyncStatus status,
                                const void* data,
                                uint64_t,
                                void* userdata) {
        ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
        ASSERT_NE(nullptr, data);

        static_cast<D3D12ResourceResidencyTests*>(userdata)->mMappedReadData = data;
    }

    static void MapWriteCallback(WGPUBufferMapAsyncStatus status,
                                 void* data,
                                 uint64_t,
                                 void* userdata) {
        ASSERT_EQ(WGPUBufferMapAsyncStatus_Success, status);
        ASSERT_NE(nullptr, data);

        static_cast<D3D12ResourceResidencyTests*>(userdata)->mMappedWriteData = data;
    }
};

class D3D12DescriptorResidencyTests : public D3D12ResidencyTestBase {};

// Check that resources existing on suballocated heaps are made resident and evicted correctly.
TEST_P(D3D12ResourceResidencyTests, OvercommitSmallResources) {
    // TODO(http://crbug.com/dawn/416): Tests fails on Intel HD 630 bot.
    DAWN_SKIP_TEST_IF(IsIntel() && IsBackendValidationEnabled());

    // Create suballocated buffers to fill half the budget.
    std::vector<wgpu::Buffer> bufferSet1 = AllocateBuffers(
        kSuballocatedResourceSize, ((kRestrictedBudgetSize / 2) / kSuballocatedResourceSize),
        kNonMappableBufferUsage);

    // Check that all the buffers allocated are resident. Also make sure they were suballocated
    // internally.
    for (uint32_t i = 0; i < bufferSet1.size(); i++) {
        EXPECT_TRUE(CheckIfBufferIsResident(bufferSet1[i]));
        EXPECT_TRUE(
            CheckAllocationMethod(bufferSet1[i], dawn_native::AllocationMethod::kSubAllocated));
    }

    // Create enough directly-allocated buffers to use the entire budget.
    std::vector<wgpu::Buffer> bufferSet2 = AllocateBuffers(
        kDirectlyAllocatedResourceSize, kRestrictedBudgetSize / kDirectlyAllocatedResourceSize,
        kNonMappableBufferUsage);

    // Check that everything in bufferSet1 is now evicted.
    for (uint32_t i = 0; i < bufferSet1.size(); i++) {
        EXPECT_FALSE(CheckIfBufferIsResident(bufferSet1[i]));
    }

    // Touch one of the non-resident buffers. This should cause the buffer to become resident.
    constexpr uint32_t indexOfBufferInSet1 = 5;
    TouchBuffers(indexOfBufferInSet1, 1, bufferSet1);
    // Check that this buffer is now resident.
    EXPECT_TRUE(CheckIfBufferIsResident(bufferSet1[indexOfBufferInSet1]));

    // Touch everything in bufferSet2 again to evict the buffer made resident in the previous
    // operation.
    TouchBuffers(0, bufferSet2.size(), bufferSet2);
    // Check that indexOfBufferInSet1 was evicted.
    EXPECT_FALSE(CheckIfBufferIsResident(bufferSet1[indexOfBufferInSet1]));
}

// Check that resources existing on directly allocated heaps are made resident and evicted
// correctly.
TEST_P(D3D12ResourceResidencyTests, OvercommitLargeResources) {
    // Create directly-allocated buffers to fill half the budget.
    std::vector<wgpu::Buffer> bufferSet1 = AllocateBuffers(
        kDirectlyAllocatedResourceSize,
        ((kRestrictedBudgetSize / 2) / kDirectlyAllocatedResourceSize), kNonMappableBufferUsage);

    // Check that all the allocated buffers are resident. Also make sure they were directly
    // allocated internally.
    for (uint32_t i = 0; i < bufferSet1.size(); i++) {
        EXPECT_TRUE(CheckIfBufferIsResident(bufferSet1[i]));
        EXPECT_TRUE(CheckAllocationMethod(bufferSet1[i], dawn_native::AllocationMethod::kDirect));
    }

    // Create enough directly-allocated buffers to use the entire budget.
    std::vector<wgpu::Buffer> bufferSet2 = AllocateBuffers(
        kDirectlyAllocatedResourceSize, kRestrictedBudgetSize / kDirectlyAllocatedResourceSize,
        kNonMappableBufferUsage);

    // Check that everything in bufferSet1 is now evicted.
    for (uint32_t i = 0; i < bufferSet1.size(); i++) {
        EXPECT_FALSE(CheckIfBufferIsResident(bufferSet1[i]));
    }

    // Touch one of the non-resident buffers. This should cause the buffer to become resident.
    constexpr uint32_t indexOfBufferInSet1 = 1;
    TouchBuffers(indexOfBufferInSet1, 1, bufferSet1);
    EXPECT_TRUE(CheckIfBufferIsResident(bufferSet1[indexOfBufferInSet1]));

    // Touch everything in bufferSet2 again to evict the buffer made resident in the previous
    // operation.
    TouchBuffers(0, bufferSet2.size(), bufferSet2);
    // Check that indexOfBufferInSet1 was evicted.
    EXPECT_FALSE(CheckIfBufferIsResident(bufferSet1[indexOfBufferInSet1]));
}

// Check that calling MapReadAsync makes the buffer resident and keeps it locked resident.
TEST_P(D3D12ResourceResidencyTests, AsyncMappedBufferRead) {
    // Create a mappable buffer.
    wgpu::Buffer buffer = CreateBuffer(4, kMapReadBufferUsage);

    uint32_t data = 12345;
    queue.WriteBuffer(buffer, 0, &data, sizeof(uint32_t));

    // The mappable buffer should be resident.
    EXPECT_TRUE(CheckIfBufferIsResident(buffer));

    // Create and touch enough buffers to use the entire budget.
    std::vector<wgpu::Buffer> bufferSet = AllocateBuffers(
        kDirectlyAllocatedResourceSize, kRestrictedBudgetSize / kDirectlyAllocatedResourceSize,
        kMapReadBufferUsage);
    TouchBuffers(0, bufferSet.size(), bufferSet);

    // The mappable buffer should have been evicted.
    EXPECT_FALSE(CheckIfBufferIsResident(buffer));

    // Calling MapReadAsync should make the buffer resident.
    buffer.MapReadAsync(MapReadCallback, this);
    EXPECT_TRUE(CheckIfBufferIsResident(buffer));

    while (mMappedReadData == nullptr) {
        WaitABit();
    }

    // Touch enough resources such that the entire budget is used. The mappable buffer should remain
    // locked resident.
    TouchBuffers(0, bufferSet.size(), bufferSet);
    EXPECT_TRUE(CheckIfBufferIsResident(buffer));

    // Unmap the buffer, allocate and touch enough resources such that the entire budget is used.
    // This should evict the mappable buffer.
    buffer.Unmap();
    std::vector<wgpu::Buffer> bufferSet2 = AllocateBuffers(
        kDirectlyAllocatedResourceSize, kRestrictedBudgetSize / kDirectlyAllocatedResourceSize,
        kMapReadBufferUsage);
    TouchBuffers(0, bufferSet2.size(), bufferSet2);
    EXPECT_FALSE(CheckIfBufferIsResident(buffer));
}

// Check that calling MapWriteAsync makes the buffer resident and keeps it locked resident.
TEST_P(D3D12ResourceResidencyTests, AsyncMappedBufferWrite) {
    // Create a mappable buffer.
    wgpu::Buffer buffer = CreateBuffer(4, kMapWriteBufferUsage);
    // The mappable buffer should be resident.
    EXPECT_TRUE(CheckIfBufferIsResident(buffer));

    // Create and touch enough buffers to use the entire budget.
    std::vector<wgpu::Buffer> bufferSet1 = AllocateBuffers(
        kDirectlyAllocatedResourceSize, kRestrictedBudgetSize / kDirectlyAllocatedResourceSize,
        kMapReadBufferUsage);
    TouchBuffers(0, bufferSet1.size(), bufferSet1);

    // The mappable buffer should have been evicted.
    EXPECT_FALSE(CheckIfBufferIsResident(buffer));

    // Calling MapWriteAsync should make the buffer resident.
    buffer.MapWriteAsync(MapWriteCallback, this);
    EXPECT_TRUE(CheckIfBufferIsResident(buffer));

    while (mMappedWriteData == nullptr) {
        WaitABit();
    }

    // Touch enough resources such that the entire budget is used. The mappable buffer should remain
    // locked resident.
    TouchBuffers(0, bufferSet1.size(), bufferSet1);
    EXPECT_TRUE(CheckIfBufferIsResident(buffer));

    // Unmap the buffer, allocate and touch enough resources such that the entire budget is used.
    // This should evict the mappable buffer.
    buffer.Unmap();
    std::vector<wgpu::Buffer> bufferSet2 = AllocateBuffers(
        kDirectlyAllocatedResourceSize, kRestrictedBudgetSize / kDirectlyAllocatedResourceSize,
        kMapReadBufferUsage);
    TouchBuffers(0, bufferSet2.size(), bufferSet2);
    EXPECT_FALSE(CheckIfBufferIsResident(buffer));
}

// Check that overcommitting in a single submit works, then make sure the budget is enforced after.
TEST_P(D3D12ResourceResidencyTests, OvercommitInASingleSubmit) {
    // Create enough buffers to exceed the budget
    constexpr uint32_t numberOfBuffersToOvercommit = 5;
    std::vector<wgpu::Buffer> bufferSet1 = AllocateBuffers(
        kDirectlyAllocatedResourceSize,
        (kRestrictedBudgetSize / kDirectlyAllocatedResourceSize) + numberOfBuffersToOvercommit,
        kNonMappableBufferUsage);
    // Touch the buffers, which creates an overcommitted command list.
    TouchBuffers(0, bufferSet1.size(), bufferSet1);
    // Ensure that all of these buffers are resident, even though we're exceeding the budget.
    for (uint32_t i = 0; i < bufferSet1.size(); i++) {
        EXPECT_TRUE(CheckIfBufferIsResident(bufferSet1[i]));
    }

    // Allocate another set of buffers that exceeds the budget.
    std::vector<wgpu::Buffer> bufferSet2 = AllocateBuffers(
        kDirectlyAllocatedResourceSize,
        (kRestrictedBudgetSize / kDirectlyAllocatedResourceSize) + numberOfBuffersToOvercommit,
        kNonMappableBufferUsage);
    // Ensure the first <numberOfBuffersToOvercommit> buffers in the second buffer set were evicted,
    // since they shouldn't fit in the budget.
    for (uint32_t i = 0; i < numberOfBuffersToOvercommit; i++) {
        EXPECT_FALSE(CheckIfBufferIsResident(bufferSet2[i]));
    }
}

TEST_P(D3D12ResourceResidencyTests, SetExternalReservation) {
    // Set an external reservation of 20% the budget. We should succesfully reserve the amount we
    // request.
    uint64_t amountReserved = dawn_native::d3d12::SetExternalMemoryReservation(
        device.Get(), kRestrictedBudgetSize * .2, dawn_native::d3d12::MemorySegment::Local);
    EXPECT_EQ(amountReserved, kRestrictedBudgetSize * .2);

    // If we're on a non-UMA device, we should also check the NON_LOCAL memory segment.
    if (!IsUMA()) {
        amountReserved = dawn_native::d3d12::SetExternalMemoryReservation(
            device.Get(), kRestrictedBudgetSize * .2, dawn_native::d3d12::MemorySegment::NonLocal);
        EXPECT_EQ(amountReserved, kRestrictedBudgetSize * .2);
    }
}

// Checks that when a descriptor heap is bound, it is locked resident. Also checks that when a
// previous descriptor heap becomes unbound, it is unlocked, placed in the LRU and can be evicted.
TEST_P(D3D12DescriptorResidencyTests, SwitchedViewHeapResidency) {
    utils::ComboRenderPipelineDescriptor renderPipelineDescriptor(device);

    // Fill in a view heap with "view only" bindgroups (1x view per group) by creating a
    // view bindgroup each draw. After HEAP_SIZE + 1 draws, the heaps must switch over.
    renderPipelineDescriptor.vertexStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Vertex, R"(
        #version 450
        void main() {
            const vec2 pos[3] = vec2[3](vec2(-1.f, 1.f), vec2(1.f, 1.f), vec2(-1.f, -1.f));
            gl_Position = vec4(pos[gl_VertexIndex], 0.f, 1.f);
        })");

    renderPipelineDescriptor.cFragmentStage.module =
        utils::CreateShaderModule(device, utils::SingleShaderStage::Fragment, R"(
        #version 450
        layout (location = 0) out vec4 fragColor;
        layout (set = 0, binding = 0) uniform colorBuffer {
            vec4 color;
        };
        void main() {
            fragColor = color;
        })");

    wgpu::RenderPipeline renderPipeline = device.CreateRenderPipeline(&renderPipelineDescriptor);
    constexpr uint32_t kSize = 512;
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kSize, kSize);

    wgpu::SamplerDescriptor samplerDesc = utils::GetDefaultSamplerDescriptor();
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

    dawn_native::d3d12::Device* d3dDevice =
        reinterpret_cast<dawn_native::d3d12::Device*>(device.Get());

    dawn_native::d3d12::ShaderVisibleDescriptorAllocator* allocator =
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

    // Check the heap serial to ensure the heap has switched.
    EXPECT_EQ(allocator->GetShaderVisibleHeapSerialForTesting(), heapSerial + 1);

    // Check that currrently bound ShaderVisibleHeap is locked resident.
    EXPECT_TRUE(allocator->IsShaderVisibleHeapLockedResidentForTesting());
    // Check that the previously bound ShaderVisibleHeap was unlocked and was placed in the LRU
    // cache.
    EXPECT_TRUE(allocator->IsLastShaderVisibleHeapInLRUForTesting());
    // Allocate enough buffers to exceed the budget, which will purge everything from the Residency
    // LRU.
    AllocateBuffers(kDirectlyAllocatedResourceSize,
                    kRestrictedBudgetSize / kDirectlyAllocatedResourceSize,
                    kNonMappableBufferUsage);
    // Check that currrently bound ShaderVisibleHeap remained locked resident.
    EXPECT_TRUE(allocator->IsShaderVisibleHeapLockedResidentForTesting());
    // Check that the previously bound ShaderVisibleHeap has been evicted from the LRU cache.
    EXPECT_FALSE(allocator->IsLastShaderVisibleHeapInLRUForTesting());
}

DAWN_INSTANTIATE_TEST(D3D12ResourceResidencyTests, D3D12Backend());
DAWN_INSTANTIATE_TEST(D3D12DescriptorResidencyTests,
                      D3D12Backend({"use_d3d12_small_shader_visible_heap"}));
