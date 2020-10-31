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

#ifndef DAWNNATIVE_OPENGL_SHADERMODULEGL_H_
#define DAWNNATIVE_OPENGL_SHADERMODULEGL_H_

#include "dawn_native/ShaderModule.h"

#include "dawn_native/opengl/opengl_platform.h"

namespace dawn_native { namespace opengl {

    class Device;

    std::string GetBindingName(BindGroupIndex group, BindingNumber bindingNumber);

    struct BindingLocation {
        BindGroupIndex group;
        BindingNumber binding;
    };
    bool operator<(const BindingLocation& a, const BindingLocation& b);

    struct CombinedSampler {
        BindingLocation samplerLocation;
        BindingLocation textureLocation;
        std::string GetName() const;
    };
    bool operator<(const CombinedSampler& a, const CombinedSampler& b);

    class ShaderModule final : public ShaderModuleBase {
      public:
        static ResultOrError<ShaderModule*> Create(Device* device,
                                                   const ShaderModuleDescriptor* descriptor);

        using CombinedSamplerInfo = std::vector<CombinedSampler>;

        const char* GetSource() const;
        const CombinedSamplerInfo& GetCombinedSamplerInfo() const;

      private:
        ShaderModule(Device* device, const ShaderModuleDescriptor* descriptor);
        ~ShaderModule() override = default;
        MaybeError Initialize();

        CombinedSamplerInfo mCombinedInfo;
        std::string mGlslSource;
    };

}}  // namespace dawn_native::opengl

#endif  // DAWNNATIVE_OPENGL_SHADERMODULEGL_H_
