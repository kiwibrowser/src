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
#include "common/ityp_array.h"
#include "dawn_native/BindingInfo.h"
#include "dawn_native/CachedObject.h"
#include "dawn_native/Error.h"
#include "dawn_native/Format.h"
#include "dawn_native/Forward.h"
#include "dawn_native/PerStage.h"

#include "dawn_native/dawn_platform.h"

#include "spvc/spvc.hpp"

#include <bitset>
#include <map>
#include <vector>

namespace spirv_cross {
    class Compiler;
}

namespace dawn_native {

    MaybeError ValidateShaderModuleDescriptor(DeviceBase* device,
                                              const ShaderModuleDescriptor* descriptor);

    class ShaderModuleBase : public CachedObject {
      public:
        enum class Type { Undefined, Spirv, Wgsl };

        ShaderModuleBase(DeviceBase* device, const ShaderModuleDescriptor* descriptor);
        ~ShaderModuleBase() override;

        static ShaderModuleBase* MakeError(DeviceBase* device);

        MaybeError ExtractSpirvInfo(const spirv_cross::Compiler& compiler);

        struct ShaderBindingInfo : BindingInfo {
            // The SPIRV ID of the resource.
            uint32_t id;
            uint32_t base_type_id;

          private:
            // Disallow access to unused members.
            using BindingInfo::hasDynamicOffset;
            using BindingInfo::visibility;
        };

        using BindingInfoMap = std::map<BindingNumber, ShaderBindingInfo>;
        using ModuleBindingInfo = ityp::array<BindGroupIndex, BindingInfoMap, kMaxBindGroups>;

        const ModuleBindingInfo& GetBindingInfo() const;
        const std::bitset<kMaxVertexAttributes>& GetUsedVertexAttributes() const;
        SingleShaderStage GetExecutionModel() const;

        // An array to record the basic types (float, int and uint) of the fragment shader outputs
        // or Format::Type::Other means the fragment shader output is unused.
        using FragmentOutputBaseTypes = std::array<Format::Type, kMaxColorAttachments>;
        const FragmentOutputBaseTypes& GetFragmentOutputBaseTypes() const;

        MaybeError ValidateCompatibilityWithPipelineLayout(const PipelineLayoutBase* layout) const;

        RequiredBufferSizes ComputeRequiredBufferSizesForLayout(
            const PipelineLayoutBase* layout) const;

        // Functors necessary for the unordered_set<ShaderModuleBase*>-based cache.
        struct HashFunc {
            size_t operator()(const ShaderModuleBase* module) const;
        };
        struct EqualityFunc {
            bool operator()(const ShaderModuleBase* a, const ShaderModuleBase* b) const;
        };

        shaderc_spvc::Context* GetContext();
        const std::vector<uint32_t>& GetSpirv() const;

      protected:
        static MaybeError CheckSpvcSuccess(shaderc_spvc_status status, const char* error_msg);
        shaderc_spvc::CompileOptions GetCompileOptions() const;
        MaybeError InitializeBase();

        shaderc_spvc::Context mSpvcContext;

      private:
        ShaderModuleBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        MaybeError ValidateCompatibilityWithBindGroupLayout(
            BindGroupIndex group,
            const BindGroupLayoutBase* layout) const;

        std::vector<uint64_t> GetBindGroupMinBufferSizes(const BindingInfoMap& shaderMap,
                                                         const BindGroupLayoutBase* layout) const;

        // Different implementations reflection into the shader depending on
        // whether using spvc, or directly accessing spirv-cross.
        MaybeError ExtractSpirvInfoWithSpvc();
        MaybeError ExtractSpirvInfoWithSpirvCross(const spirv_cross::Compiler& compiler);

        Type mType;
        std::vector<uint32_t> mSpirv;
        std::string mWgsl;

        ModuleBindingInfo mBindingInfo;
        std::bitset<kMaxVertexAttributes> mUsedVertexAttributes;
        SingleShaderStage mExecutionModel;

        FragmentOutputBaseTypes mFragmentOutputFormatBaseTypes;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_SHADERMODULE_H_
