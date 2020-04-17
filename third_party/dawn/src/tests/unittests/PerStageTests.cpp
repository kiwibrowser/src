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
    ASSERT_EQ(StageBit(dawn::ShaderStage::Vertex), dawn::ShaderStageBit::Vertex);
    ASSERT_EQ(StageBit(dawn::ShaderStage::Fragment), dawn::ShaderStageBit::Fragment);
    ASSERT_EQ(StageBit(dawn::ShaderStage::Compute), dawn::ShaderStageBit::Compute);
}

// Basic test for the PerStage container
TEST(PerStage, PerStage) {
    PerStage<int> data;

    // Store data using dawn::ShaderStage
    data[dawn::ShaderStage::Vertex] = 42;
    data[dawn::ShaderStage::Fragment] = 3;
    data[dawn::ShaderStage::Compute] = -1;

    // Load it using dawn::ShaderStageBit
    ASSERT_EQ(data[dawn::ShaderStageBit::Vertex], 42);
    ASSERT_EQ(data[dawn::ShaderStageBit::Fragment], 3);
    ASSERT_EQ(data[dawn::ShaderStageBit::Compute], -1);
}

// Test IterateStages with kAllStages
TEST(PerStage, IterateAllStages) {
    PerStage<int> counts;
    counts[dawn::ShaderStage::Vertex] = 0;
    counts[dawn::ShaderStage::Fragment] = 0;
    counts[dawn::ShaderStage::Compute] = 0;

    for (auto stage : IterateStages(kAllStages)) {
        counts[stage] ++;
    }

    ASSERT_EQ(counts[dawn::ShaderStageBit::Vertex], 1);
    ASSERT_EQ(counts[dawn::ShaderStageBit::Fragment], 1);
    ASSERT_EQ(counts[dawn::ShaderStageBit::Compute], 1);
}

// Test IterateStages with one stage
TEST(PerStage, IterateOneStage) {
    PerStage<int> counts;
    counts[dawn::ShaderStage::Vertex] = 0;
    counts[dawn::ShaderStage::Fragment] = 0;
    counts[dawn::ShaderStage::Compute] = 0;

    for (auto stage : IterateStages(dawn::ShaderStageBit::Fragment)) {
        counts[stage] ++;
    }

    ASSERT_EQ(counts[dawn::ShaderStageBit::Vertex], 0);
    ASSERT_EQ(counts[dawn::ShaderStageBit::Fragment], 1);
    ASSERT_EQ(counts[dawn::ShaderStageBit::Compute], 0);
}

// Test IterateStages with no stage
TEST(PerStage, IterateNoStages) {
    PerStage<int> counts;
    counts[dawn::ShaderStage::Vertex] = 0;
    counts[dawn::ShaderStage::Fragment] = 0;
    counts[dawn::ShaderStage::Compute] = 0;

    for (auto stage : IterateStages(dawn::ShaderStageBit::Fragment & dawn::ShaderStageBit::Vertex)) {
        counts[stage] ++;
    }

    ASSERT_EQ(counts[dawn::ShaderStageBit::Vertex], 0);
    ASSERT_EQ(counts[dawn::ShaderStageBit::Fragment], 0);
    ASSERT_EQ(counts[dawn::ShaderStageBit::Compute], 0);
}
