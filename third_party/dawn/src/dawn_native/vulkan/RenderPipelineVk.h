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

#ifndef DAWNNATIVE_VULKAN_RENDERPIPELINEVK_H_
#define DAWNNATIVE_VULKAN_RENDERPIPELINEVK_H_

#include "dawn_native/RenderPipeline.h"

#include "common/vulkan_platform.h"
#include "dawn_native/Error.h"

namespace dawn_native { namespace vulkan {

    class Device;

    class RenderPipeline final : public RenderPipelineBase {
      public:
        static ResultOrError<RenderPipeline*> Create(Device* device,
                                                     const RenderPipelineDescriptor* descriptor);

        VkPipeline GetHandle() const;

      private:
        ~RenderPipeline() override;
        using RenderPipelineBase::RenderPipelineBase;
        MaybeError Initialize(const RenderPipelineDescriptor* descriptor);

        struct PipelineVertexInputStateCreateInfoTemporaryAllocations {
            std::array<VkVertexInputBindingDescription, kMaxVertexBuffers> bindings;
            std::array<VkVertexInputAttributeDescription, kMaxVertexAttributes> attributes;
        };
        VkPipelineVertexInputStateCreateInfo ComputeVertexInputDesc(
            PipelineVertexInputStateCreateInfoTemporaryAllocations* temporaryAllocations);

        VkPipeline mHandle = VK_NULL_HANDLE;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_RENDERPIPELINEVK_H_
