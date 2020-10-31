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
    class QuerySetBase;
    class TextureBase;

    enum class PassType { Render, Compute };

    // Describe the usage of the whole texture and its subresources.
    // - subresourceUsages vector is used to track every subresource's usage within a texture.
    //
    // - usage variable is used the track the whole texture even though it can be deduced from
    // subresources' usages. This is designed deliberately to track texture usage in a fast path
    // at frontend.
    //
    // - sameUsagesAcrossSubresources is used for optimization at backend. If the texture view
    // we are using covers all subresources, then the texture's usages of all subresources are
    // the same. Otherwise the texture's usages of all subresources are thought as different,
    // although we can deliberately design some particular cases in which we have a few texture
    // views and all of them have the same usages and they cover all subresources of the texture
    // altogether.

    // TODO(yunchao.he@intel.com): if sameUsagesAcrossSubresources is true, we don't need
    // the vector to record every single subresource's Usages. The texture usage is enough. And we
    // can decompress texture usage to a vector if necessary.
    struct PassTextureUsage {
        wgpu::TextureUsage usage = wgpu::TextureUsage::None;
        bool sameUsagesAcrossSubresources = true;
        std::vector<wgpu::TextureUsage> subresourceUsages;
    };

    // Which resources are used by pass and how they are used. The command buffer validation
    // pre-computes this information so that backends with explicit barriers don't have to
    // re-compute it.
    struct PassResourceUsage {
        PassType passType;
        std::vector<BufferBase*> buffers;
        std::vector<wgpu::BufferUsage> bufferUsages;

        std::vector<TextureBase*> textures;
        std::vector<PassTextureUsage> textureUsages;
    };

    using PerPassUsages = std::vector<PassResourceUsage>;

    struct CommandBufferResourceUsage {
        PerPassUsages perPass;
        std::set<BufferBase*> topLevelBuffers;
        std::set<TextureBase*> topLevelTextures;
        std::set<QuerySetBase*> usedQuerySets;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_PASSRESOURCEUSAGE_H
