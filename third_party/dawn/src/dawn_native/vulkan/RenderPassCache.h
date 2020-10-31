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

#ifndef DAWNNATIVE_VULKAN_RENDERPASSCACHE_H_
#define DAWNNATIVE_VULKAN_RENDERPASSCACHE_H_

#include "common/Constants.h"
#include "common/vulkan_platform.h"
#include "dawn_native/Error.h"
#include "dawn_native/dawn_platform.h"

#include <array>
#include <bitset>
#include <unordered_map>

namespace dawn_native { namespace vulkan {

    class Device;

    // This is a key to query the RenderPassCache, it can be sparse meaning that only the
    // information for bits set in colorMask or hasDepthStencil need to be provided and the rest can
    // be uninintialized.
    struct RenderPassCacheQuery {
        // Use these helpers to build the query, they make sure all relevant data is initialized and
        // masks set.
        void SetColor(uint32_t index,
                      wgpu::TextureFormat format,
                      wgpu::LoadOp loadOp,
                      bool hasResolveTarget);
        void SetDepthStencil(wgpu::TextureFormat format,
                             wgpu::LoadOp depthLoadOp,
                             wgpu::LoadOp stencilLoadOp);
        void SetSampleCount(uint32_t sampleCount);

        std::bitset<kMaxColorAttachments> colorMask;
        std::bitset<kMaxColorAttachments> resolveTargetMask;
        std::array<wgpu::TextureFormat, kMaxColorAttachments> colorFormats;
        std::array<wgpu::LoadOp, kMaxColorAttachments> colorLoadOp;

        bool hasDepthStencil = false;
        wgpu::TextureFormat depthStencilFormat;
        wgpu::LoadOp depthLoadOp;
        wgpu::LoadOp stencilLoadOp;

        uint32_t sampleCount;
    };

    // Caches VkRenderPasses so that we don't create duplicate ones for every RenderPipeline or
    // render pass. We always arrange the order of attachments in "color-depthstencil-resolve" order
    // when creating render pass and framebuffer so that we can always make sure the order of
    // attachments in the rendering pipeline matches the one of the framebuffer.
    // TODO(cwallez@chromium.org): Make it an LRU cache somehow?
    class RenderPassCache {
      public:
        RenderPassCache(Device* device);
        ~RenderPassCache();

        ResultOrError<VkRenderPass> GetRenderPass(const RenderPassCacheQuery& query);

      private:
        // Does the actual VkRenderPass creation on a cache miss.
        ResultOrError<VkRenderPass> CreateRenderPassForQuery(
            const RenderPassCacheQuery& query) const;

        // Implements the functors necessary for to use RenderPassCacheQueries as unordered_map
        // keys.
        struct CacheFuncs {
            size_t operator()(const RenderPassCacheQuery& query) const;
            bool operator()(const RenderPassCacheQuery& a, const RenderPassCacheQuery& b) const;
        };
        using Cache =
            std::unordered_map<RenderPassCacheQuery, VkRenderPass, CacheFuncs, CacheFuncs>;

        Device* mDevice = nullptr;
        Cache mCache;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_RENDERPASSCACHE_H_
