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

#include "dawn_native/vulkan/UtilsVulkan.h"

#include "common/Assert.h"

namespace dawn_native { namespace vulkan {

    VkCompareOp ToVulkanCompareOp(dawn::CompareFunction op) {
        switch (op) {
            case dawn::CompareFunction::Always:
                return VK_COMPARE_OP_ALWAYS;
            case dawn::CompareFunction::Equal:
                return VK_COMPARE_OP_EQUAL;
            case dawn::CompareFunction::Greater:
                return VK_COMPARE_OP_GREATER;
            case dawn::CompareFunction::GreaterEqual:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case dawn::CompareFunction::Less:
                return VK_COMPARE_OP_LESS;
            case dawn::CompareFunction::LessEqual:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case dawn::CompareFunction::Never:
                return VK_COMPARE_OP_NEVER;
            case dawn::CompareFunction::NotEqual:
                return VK_COMPARE_OP_NOT_EQUAL;
            default:
                UNREACHABLE();
        }
    }

}}  // namespace dawn_native::vulkan
