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

#ifndef DAWNNATIVE_PERSTAGE_H_
#define DAWNNATIVE_PERSTAGE_H_

#include "common/Assert.h"
#include "common/BitSetIterator.h"
#include "common/Constants.h"

#include "dawn_native/dawn_platform.h"

#include <array>

namespace dawn_native {

    static_assert(static_cast<uint32_t>(dawn::ShaderStage::Vertex) < kNumStages, "");
    static_assert(static_cast<uint32_t>(dawn::ShaderStage::Fragment) < kNumStages, "");
    static_assert(static_cast<uint32_t>(dawn::ShaderStage::Compute) < kNumStages, "");

    static_assert(static_cast<uint32_t>(dawn::ShaderStageBit::Vertex) ==
                      (1 << static_cast<uint32_t>(dawn::ShaderStage::Vertex)),
                  "");
    static_assert(static_cast<uint32_t>(dawn::ShaderStageBit::Fragment) ==
                      (1 << static_cast<uint32_t>(dawn::ShaderStage::Fragment)),
                  "");
    static_assert(static_cast<uint32_t>(dawn::ShaderStageBit::Compute) ==
                      (1 << static_cast<uint32_t>(dawn::ShaderStage::Compute)),
                  "");

    BitSetIterator<kNumStages, dawn::ShaderStage> IterateStages(dawn::ShaderStageBit stages);
    dawn::ShaderStageBit StageBit(dawn::ShaderStage stage);

    static constexpr dawn::ShaderStageBit kAllStages =
        static_cast<dawn::ShaderStageBit>((1 << kNumStages) - 1);

    template <typename T>
    class PerStage {
      public:
        PerStage() = default;
        PerStage(const T& initialValue) {
            mData.fill(initialValue);
        }

        T& operator[](dawn::ShaderStage stage) {
            DAWN_ASSERT(static_cast<uint32_t>(stage) < kNumStages);
            return mData[static_cast<uint32_t>(stage)];
        }
        const T& operator[](dawn::ShaderStage stage) const {
            DAWN_ASSERT(static_cast<uint32_t>(stage) < kNumStages);
            return mData[static_cast<uint32_t>(stage)];
        }

        T& operator[](dawn::ShaderStageBit stageBit) {
            uint32_t bit = static_cast<uint32_t>(stageBit);
            DAWN_ASSERT(bit != 0 && IsPowerOfTwo(bit) && bit <= (1 << kNumStages));
            return mData[Log2(bit)];
        }
        const T& operator[](dawn::ShaderStageBit stageBit) const {
            uint32_t bit = static_cast<uint32_t>(stageBit);
            DAWN_ASSERT(bit != 0 && IsPowerOfTwo(bit) && bit <= (1 << kNumStages));
            return mData[Log2(bit)];
        }

      private:
        std::array<T, kNumStages> mData;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_PERSTAGE_H_
