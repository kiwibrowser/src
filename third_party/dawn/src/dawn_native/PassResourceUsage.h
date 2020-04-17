// Copyright 2018 The Dawn Authors
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

#ifndef DAWNNATIVE_PASSRESOURCEUSAGE_H
#define DAWNNATIVE_PASSRESOURCEUSAGE_H

#include "dawn_native/dawn_platform.h"

#include <set>
#include <vector>

namespace dawn_native {

    class BufferBase;
    class TextureBase;

    // Which resources are used by pass and how they are used. The command buffer validation
    // pre-computes this information so that backends with explicit barriers don't have to
    // re-compute it.
    struct PassResourceUsage {
        std::vector<BufferBase*> buffers;
        std::vector<dawn::BufferUsageBit> bufferUsages;

        std::vector<TextureBase*> textures;
        std::vector<dawn::TextureUsageBit> textureUsages;
    };

    struct CommandBufferResourceUsage {
        std::vector<PassResourceUsage> perPass;
        std::set<BufferBase*> topLevelBuffers;
        std::set<TextureBase*> topLevelTextures;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_PASSRESOURCEUSAGE_H
