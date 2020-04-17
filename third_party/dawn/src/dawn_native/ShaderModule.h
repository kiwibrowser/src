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

#ifndef DAWNNATIVE_SHADERMODULE_H_
#define DAWNNATIVE_SHADERMODULE_H_

#include "common/Constants.h"
#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"

#include "dawn_native/dawn_platform.h"

#include <array>
#include <bitset>
#include <vector>

namespace spirv_cross {
    class Compiler;
}

namespace dawn_native {

    MaybeError ValidateShaderModuleDescriptor(DeviceBase* device,
                                              const ShaderModuleDescriptor* descriptor);

    class ShaderModuleBase : public ObjectBase {
      public:
        ShaderModuleBase(DeviceBase* device,
                         const ShaderModuleDescriptor* descriptor,
                         bool blueprint = false);
        ~ShaderModuleBase() override;

        static ShaderModuleBase* MakeError(DeviceBase* device);

        void ExtractSpirvInfo(const spirv_cross::Compiler& compiler);

        struct BindingInfo {
            // The SPIRV ID of the resource.
            uint32_t id;
            uint32_t base_type_id;
            dawn::BindingType type;
            bool used = false;
        };
        using ModuleBindingInfo =
            std::array<std::array<BindingInfo, kMaxBindingsPerGroup>, kMaxBindGroups>;

        const ModuleBindingInfo& GetBindingInfo() const;
        const std::bitset<kMaxVertexAttributes>& GetUsedVertexAttributes() const;
        dawn::ShaderStage GetExecutionModel() const;

        bool IsCompatibleWithPipelineLayout(const PipelineLayoutBase* layout);

        // Functors necessary for the unordered_set<ShaderModuleBase*>-based cache.
        struct HashFunc {
            size_t operator()(const ShaderModuleBase* module) const;
        };
        struct EqualityFunc {
            bool operator()(const ShaderModuleBase* a, const ShaderModuleBase* b) const;
        };

      private:
        ShaderModuleBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        bool IsCompatibleWithBindGroupLayout(size_t group, const BindGroupLayoutBase* layout);

        // TODO(cwallez@chromium.org): The code is only stored for deduplication. We could maybe
        // store a cryptographic hash of the code instead?
        std::vector<uint32_t> mCode;
        bool mIsBlueprint = false;

        ModuleBindingInfo mBindingInfo;
        std::bitset<kMaxVertexAttributes> mUsedVertexAttributes;
        dawn::ShaderStage mExecutionModel;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_SHADERMODULE_H_
