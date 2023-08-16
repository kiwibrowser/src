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

#ifndef DAWNNATIVE_ATTACHMENTSTATE_H_
#define DAWNNATIVE_ATTACHMENTSTATE_H_

#include "common/Constants.h"
#include "dawn_native/CachedObject.h"

#include "dawn_native/dawn_platform.h"

#include <array>
#include <bitset>

namespace dawn_native {

    class DeviceBase;

    // AttachmentStateBlueprint and AttachmentState are separated so the AttachmentState
    // can be constructed by copying the blueprint state instead of traversing descriptors.
    // Also, AttachmentStateBlueprint does not need a refcount like AttachmentState.
    class AttachmentStateBlueprint {
      public:
        // Note: Descriptors must be validated before the AttachmentState is constructed.
        explicit AttachmentStateBlueprint(const RenderBundleEncoderDescriptor* descriptor);
        explicit AttachmentStateBlueprint(const RenderPipelineDescriptor* descriptor);
        explicit AttachmentStateBlueprint(const RenderPassDescriptor* descriptor);

        AttachmentStateBlueprint(const AttachmentStateBlueprint& rhs);

        // Functors necessary for the unordered_set<AttachmentState*>-based cache.
        struct HashFunc {
            size_t operator()(const AttachmentStateBlueprint* attachmentState) const;
        };
        struct EqualityFunc {
            bool operator()(const AttachmentStateBlueprint* a,
                            const AttachmentStateBlueprint* b) const;
        };

      protected:
        std::bitset<kMaxColorAttachments> mColorAttachmentsSet;
        std::array<wgpu::TextureFormat, kMaxColorAttachments> mColorFormats;
        // Default (texture format Undefined) indicates there is no depth stencil attachment.
        wgpu::TextureFormat mDepthStencilFormat = wgpu::TextureFormat::Undefined;
        uint32_t mSampleCount = 0;
    };

    class AttachmentState final : public AttachmentStateBlueprint, public CachedObject {
      public:
        AttachmentState(DeviceBase* device, const AttachmentStateBlueprint& blueprint);

        std::bitset<kMaxColorAttachments> GetColorAttachmentsMask() const;
        wgpu::TextureFormat GetColorAttachmentFormat(uint32_t index) const;
        bool HasDepthStencilAttachment() const;
        wgpu::TextureFormat GetDepthStencilFormat() const;
        uint32_t GetSampleCount() const;

      private:
        ~AttachmentState() override;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_ATTACHMENTSTATE_H_
