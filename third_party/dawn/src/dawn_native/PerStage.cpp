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

#include "dawn_native/PerStage.h"

namespace dawn_native {

    BitSetIterator<kNumStages, SingleShaderStage> IterateStages(wgpu::ShaderStage stages) {
        std::bitset<kNumStages> bits(static_cast<uint32_t>(stages));
        return BitSetIterator<kNumStages, SingleShaderStage>(bits);
    }

    wgpu::ShaderStage StageBit(SingleShaderStage stage) {
        ASSERT(static_cast<uint32_t>(stage) < kNumStages);
        return static_cast<wgpu::ShaderStage>(1 << static_cast<uint32_t>(stage));
    }

}  // namespace dawn_native
