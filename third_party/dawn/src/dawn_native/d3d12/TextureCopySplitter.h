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

#ifndef DAWNNATIVE_D3D12_TEXTURECOPYSPLITTER_H_
#define DAWNNATIVE_D3D12_TEXTURECOPYSPLITTER_H_

#include "dawn_native/dawn_platform.h"

#include <array>

namespace dawn_native {

    struct Format;

}  // namespace dawn_native

namespace dawn_native { namespace d3d12 {

    struct Texture2DCopySplit {
        static constexpr unsigned int kMaxTextureCopyRegions = 2;

        struct CopyInfo {
            Origin3D textureOffset;
            Origin3D bufferOffset;
            Extent3D bufferSize;

            Extent3D copySize;
        };

        uint64_t offset = 0;
        uint32_t count = 0;
        std::array<CopyInfo, kMaxTextureCopyRegions> copies;
    };

    struct TextureCopySplits {
        static constexpr uint32_t kMaxTextureCopySplits = 2;

        std::array<Texture2DCopySplit, kMaxTextureCopySplits> copies2D;
    };

    Texture2DCopySplit ComputeTextureCopySplit(Origin3D origin,
                                               Extent3D copySize,
                                               const Format& format,
                                               uint64_t offset,
                                               uint32_t bytesPerRow,
                                               uint32_t rowsPerImage);

    TextureCopySplits ComputeTextureCopySplits(Origin3D origin,
                                               Extent3D copySize,
                                               const Format& format,
                                               uint64_t offset,
                                               uint32_t bytesPerRow,
                                               uint32_t rowsPerImage);
}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_TEXTURECOPYSPLITTER_H_
