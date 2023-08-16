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

#ifndef DAWNNATIVE_PASSRESOURCEUSAGETRACKER_H_
#define DAWNNATIVE_PASSRESOURCEUSAGETRACKER_H_

#include "dawn_native/PassResourceUsage.h"

#include "dawn_native/dawn_platform.h"

#include <map>

namespace dawn_native {

    class BufferBase;
    class TextureBase;

    // Helper class to encapsulate the logic of tracking per-resource usage during the
    // validation of command buffer passes. It is used both to know if there are validation
    // errors, and to get a list of resources used per pass for backends that need the
    // information.
    class PassResourceUsageTracker {
      public:
        PassResourceUsageTracker(PassType passType);
        void BufferUsedAs(BufferBase* buffer, wgpu::BufferUsage usage);
        void TextureViewUsedAs(TextureViewBase* texture, wgpu::TextureUsage usage);
        void AddTextureUsage(TextureBase* texture, const PassTextureUsage& textureUsage);

        // Returns the per-pass usage for use by backends for APIs with explicit barriers.
        PassResourceUsage AcquireResourceUsage();

      private:
        PassType mPassType;
        std::map<BufferBase*, wgpu::BufferUsage> mBufferUsages;
        std::map<TextureBase*, PassTextureUsage> mTextureUsages;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_PASSRESOURCEUSAGETRACKER_H_
