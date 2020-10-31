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

#include "common/Constants.h"
#include "tests/DawnTest.h"

#include <vector>

class PipelineLayoutTests : public DawnTest {};

// Test creating a PipelineLayout with multiple BGLs where the first BGL uses the max number of
// dynamic buffers. This is a regression test for crbug.com/dawn/449 which would overflow when
// dynamic offset bindings were at max. Test is successful if the pipeline layout is created
// without error.
TEST_P(PipelineLayoutTests, DynamicBuffersOverflow) {
    // Create the first bind group layout which uses max number of dynamic buffers bindings.
    wgpu::BindGroupLayout bglA;
    {
        std::vector<wgpu::BindGroupLayoutEntry> entries;
        for (uint32_t i = 0; i < kMaxDynamicStorageBuffersPerPipelineLayout; i++) {
            entries.push_back(
                {i, wgpu::ShaderStage::Compute, wgpu::BindingType::StorageBuffer, true});
        }

        wgpu::BindGroupLayoutDescriptor descriptor;
        descriptor.entryCount = static_cast<uint32_t>(entries.size());
        descriptor.entries = entries.data();
        bglA = device.CreateBindGroupLayout(&descriptor);
    }

    // Create the second bind group layout that has one non-dynamic buffer binding.
    // It is in the fragment stage to avoid the max per-stage storage buffer limit.
    wgpu::BindGroupLayout bglB;
    {
        wgpu::BindGroupLayoutDescriptor descriptor;
        wgpu::BindGroupLayoutEntry entry = {0, wgpu::ShaderStage::Fragment,
                                            wgpu::BindingType::StorageBuffer, false};
        descriptor.entryCount = 1;
        descriptor.entries = &entry;
        bglB = device.CreateBindGroupLayout(&descriptor);
    }

    // Create a pipeline layout using both bind group layouts.
    wgpu::PipelineLayoutDescriptor descriptor;
    std::vector<wgpu::BindGroupLayout> bindgroupLayouts = {bglA, bglB};
    descriptor.bindGroupLayoutCount = bindgroupLayouts.size();
    descriptor.bindGroupLayouts = bindgroupLayouts.data();
    device.CreatePipelineLayout(&descriptor);
}

DAWN_INSTANTIATE_TEST(PipelineLayoutTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      VulkanBackend());
