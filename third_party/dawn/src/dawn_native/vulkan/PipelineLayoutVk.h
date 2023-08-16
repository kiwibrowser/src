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

#ifndef DAWNNATIVE_VULKAN_PIPELINELAYOUTVK_H_
#define DAWNNATIVE_VULKAN_PIPELINELAYOUTVK_H_

#include "dawn_native/PipelineLayout.h"

#include "common/vulkan_platform.h"
#include "dawn_native/Error.h"

namespace dawn_native { namespace vulkan {

    class Device;

    class PipelineLayout final : public PipelineLayoutBase {
      public:
        static ResultOrError<PipelineLayout*> Create(Device* device,
                                                     const PipelineLayoutDescriptor* descriptor);

        VkPipelineLayout GetHandle() const;

      private:
        ~PipelineLayout() override;
        using PipelineLayoutBase::PipelineLayoutBase;
        MaybeError Initialize();

        VkPipelineLayout mHandle = VK_NULL_HANDLE;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_PIPELINELAYOUTVK_H_
