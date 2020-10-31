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

#include <gtest/gtest.h>

#include "dawn_native/PerStage.h"

using namespace dawn_native;

// Tests for StageBit
TEST(PerStage, StageBit) {
    ASSERT_EQ(StageBit(SingleShaderStage::Vertex), wgpu::ShaderStage::Vertex);
    ASSERT_EQ(StageBit(SingleShaderStage::Fragment), wgpu::ShaderStage::Fragment);
    ASSERT_EQ(StageBit(SingleShaderStage::Compute), wgpu::ShaderStage::Compute);
}

// Basic test for the PerStage container
TEST(PerStage, PerStage) {
    PerStage<int> data;

    // Store data using wgpu::ShaderStage
    data[SingleShaderStage::Vertex] = 42;
    data[SingleShaderStage::Fragment] = 3;
    data[SingleShaderStage::Compute] = -1;

    // Load it using wgpu::ShaderStage
    ASSERT_EQ(data[wgpu::ShaderStage::Vertex], 42);
    ASSERT_EQ(data[wgpu::ShaderStage::Fragment], 3);
    ASSERT_EQ(data[wgpu::ShaderStage::Compute], -1);
}

// Test IterateStages with kAllStages
TEST(PerStage, IterateAllStages) {
    PerStage<int> counts;
    counts[SingleShaderStage::Vertex] = 0;
    counts[SingleShaderStage::Fragment] = 0;
    counts[SingleShaderStage::Compute] = 0;

    for (auto stage : IterateStages(kAllStages)) {
        counts[stage]++;
    }

    ASSERT_EQ(counts[wgpu::ShaderStage::Vertex], 1);
    ASSERT_EQ(counts[wgpu::ShaderStage::Fragment], 1);
    ASSERT_EQ(counts[wgpu::ShaderStage::Compute], 1);
}

// Test IterateStages with one stage
TEST(PerStage, IterateOneStage) {
    PerStage<int> counts;
    counts[SingleShaderStage::Vertex] = 0;
    counts[SingleShaderStage::Fragment] = 0;
    counts[SingleShaderStage::Compute] = 0;

    for (auto stage : IterateStages(wgpu::ShaderStage::Fragment)) {
        counts[stage]++;
    }

    ASSERT_EQ(counts[wgpu::ShaderStage::Vertex], 0);
    ASSERT_EQ(counts[wgpu::ShaderStage::Fragment], 1);
    ASSERT_EQ(counts[wgpu::ShaderStage::Compute], 0);
}

// Test IterateStages with no stage
TEST(PerStage, IterateNoStages) {
    PerStage<int> counts;
    counts[SingleShaderStage::Vertex] = 0;
    counts[SingleShaderStage::Fragment] = 0;
    counts[SingleShaderStage::Compute] = 0;

    for (auto stage : IterateStages(wgpu::ShaderStage::Fragment & wgpu::ShaderStage::Vertex)) {
        counts[stage]++;
    }

    ASSERT_EQ(counts[wgpu::ShaderStage::Vertex], 0);
    ASSERT_EQ(counts[wgpu::ShaderStage::Fragment], 0);
    ASSERT_EQ(counts[wgpu::ShaderStage::Compute], 0);
}
