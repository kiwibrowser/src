// Copyright 2019 The Dawn Authors
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

#include "dawn_native/PassResourceUsageTracker.h"

#include "dawn_native/Buffer.h"
#include "dawn_native/Texture.h"

namespace dawn_native {
    PassResourceUsageTracker::PassResourceUsageTracker(PassType passType) : mPassType(passType) {
    }

    void PassResourceUsageTracker::BufferUsedAs(BufferBase* buffer, wgpu::BufferUsage usage) {
        // std::map's operator[] will create the key and return 0 if the key didn't exist
        // before.
        mBufferUsages[buffer] |= usage;
    }

    void PassResourceUsageTracker::TextureViewUsedAs(TextureViewBase* view,
                                                     wgpu::TextureUsage usage) {
        TextureBase* texture = view->GetTexture();
        uint32_t baseMipLevel = view->GetBaseMipLevel();
        uint32_t levelCount = view->GetLevelCount();
        uint32_t baseArrayLayer = view->GetBaseArrayLayer();
        uint32_t layerCount = view->GetLayerCount();

        // std::map's operator[] will create the key and return a PassTextureUsage with usage = 0
        // and an empty vector for subresourceUsages.
        // TODO (yunchao.he@intel.com): optimize this
        PassTextureUsage& textureUsage = mTextureUsages[texture];

        // Set parameters for the whole texture
        textureUsage.usage |= usage;
        uint32_t subresourceCount = texture->GetSubresourceCount();
        textureUsage.sameUsagesAcrossSubresources &= levelCount * layerCount == subresourceCount;

        // Set usages for subresources
        if (!textureUsage.subresourceUsages.size()) {
            textureUsage.subresourceUsages =
                std::vector<wgpu::TextureUsage>(subresourceCount, wgpu::TextureUsage::None);
        }
        for (uint32_t arrayLayer = baseArrayLayer; arrayLayer < baseArrayLayer + layerCount;
             ++arrayLayer) {
            for (uint32_t mipLevel = baseMipLevel; mipLevel < baseMipLevel + levelCount;
                 ++mipLevel) {
                uint32_t subresourceIndex = texture->GetSubresourceIndex(mipLevel, arrayLayer);
                textureUsage.subresourceUsages[subresourceIndex] |= usage;
            }
        }
    }

    void PassResourceUsageTracker::AddTextureUsage(TextureBase* texture,
                                                   const PassTextureUsage& textureUsage) {
        PassTextureUsage& passTextureUsage = mTextureUsages[texture];
        passTextureUsage.usage |= textureUsage.usage;
        passTextureUsage.sameUsagesAcrossSubresources &= textureUsage.sameUsagesAcrossSubresources;

        uint32_t subresourceCount = texture->GetSubresourceCount();
        ASSERT(textureUsage.subresourceUsages.size() == subresourceCount);
        if (!passTextureUsage.subresourceUsages.size()) {
            passTextureUsage.subresourceUsages = textureUsage.subresourceUsages;
            return;
        }
        for (uint32_t i = 0; i < subresourceCount; ++i) {
            passTextureUsage.subresourceUsages[i] |= textureUsage.subresourceUsages[i];
        }
    }

    // Returns the per-pass usage for use by backends for APIs with explicit barriers.
    PassResourceUsage PassResourceUsageTracker::AcquireResourceUsage() {
        PassResourceUsage result;
        result.passType = mPassType;
        result.buffers.reserve(mBufferUsages.size());
        result.bufferUsages.reserve(mBufferUsages.size());
        result.textures.reserve(mTextureUsages.size());
        result.textureUsages.reserve(mTextureUsages.size());

        for (auto& it : mBufferUsages) {
            result.buffers.push_back(it.first);
            result.bufferUsages.push_back(it.second);
        }

        for (auto& it : mTextureUsages) {
            result.textures.push_back(it.first);
            result.textureUsages.push_back(std::move(it.second));
        }

        mBufferUsages.clear();
        mTextureUsages.clear();

        return result;
    }

}  // namespace dawn_native
