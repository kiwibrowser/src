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

#include "dawn_native/opengl/ShaderModuleGL.h"

#include "common/Assert.h"
#include "common/Platform.h"
#include "dawn_native/opengl/DeviceGL.h"

#include <spirv-cross/spirv_glsl.hpp>

#include <sstream>

namespace dawn_native { namespace opengl {

    std::string GetBindingName(uint32_t group, uint32_t binding) {
        std::ostringstream o;
        o << "dawn_binding_" << group << "_" << binding;
        return o.str();
    }

    bool operator<(const BindingLocation& a, const BindingLocation& b) {
        return std::tie(a.group, a.binding) < std::tie(b.group, b.binding);
    }

    bool operator<(const CombinedSampler& a, const CombinedSampler& b) {
        return std::tie(a.samplerLocation, a.textureLocation) <
               std::tie(b.samplerLocation, b.textureLocation);
    }

    std::string CombinedSampler::GetName() const {
        std::ostringstream o;
        o << "dawn_combined";
        o << "_" << samplerLocation.group << "_" << samplerLocation.binding;
        o << "_with_" << textureLocation.group << "_" << textureLocation.binding;
        return o.str();
    }

    ShaderModule::ShaderModule(Device* device, const ShaderModuleDescriptor* descriptor)
        : ShaderModuleBase(device, descriptor) {
        spirv_cross::CompilerGLSL compiler(descriptor->code, descriptor->codeSize);
        // If these options are changed, the values in DawnSPIRVCrossGLSLFastFuzzer.cpp need to be
        // updated.
        spirv_cross::CompilerGLSL::Options options;

        // The range of Z-coordinate in the clipping volume of OpenGL is [-w, w], while it is [0, w]
        // in D3D12, Metal and Vulkan, so we should normalize it in shaders in all backends.
        // See the documentation of spirv_cross::CompilerGLSL::Options::vertex::fixup_clipspace for
        // more details.
        options.vertex.fixup_clipspace = true;

        // TODO(cwallez@chromium.org): discover the backing context version and use that.
#if defined(DAWN_PLATFORM_APPLE)
        options.version = 410;
#else
        options.version = 440;
#endif
        compiler.set_common_options(options);

        // Rename the push constant block to be prefixed with the shader stage type so that uniform
        // names don't match between the FS and the VS.
        const auto& resources = compiler.get_shader_resources();
        if (resources.push_constant_buffers.size() > 0) {
            const char* prefix = nullptr;
            switch (compiler.get_execution_model()) {
                case spv::ExecutionModelVertex:
                    prefix = "vs_";
                    break;
                case spv::ExecutionModelFragment:
                    prefix = "fs_";
                    break;
                case spv::ExecutionModelGLCompute:
                    prefix = "cs_";
                    break;
                default:
                    UNREACHABLE();
            }
            auto interfaceBlock = resources.push_constant_buffers[0];
            compiler.set_name(interfaceBlock.id, prefix + interfaceBlock.name);
        }

        ExtractSpirvInfo(compiler);

        const auto& bindingInfo = GetBindingInfo();

        // Extract bindings names so that it can be used to get its location in program.
        // Now translate the separate sampler / textures into combined ones and store their info.
        // We need to do this before removing the set and binding decorations.
        compiler.build_combined_image_samplers();

        for (const auto& combined : compiler.get_combined_image_samplers()) {
            mCombinedInfo.emplace_back();

            auto& info = mCombinedInfo.back();
            info.samplerLocation.group =
                compiler.get_decoration(combined.sampler_id, spv::DecorationDescriptorSet);
            info.samplerLocation.binding =
                compiler.get_decoration(combined.sampler_id, spv::DecorationBinding);
            info.textureLocation.group =
                compiler.get_decoration(combined.image_id, spv::DecorationDescriptorSet);
            info.textureLocation.binding =
                compiler.get_decoration(combined.image_id, spv::DecorationBinding);
            compiler.set_name(combined.combined_id, info.GetName());
        }

        // Change binding names to be "dawn_binding_<group>_<binding>".
        // Also unsets the SPIRV "Binding" decoration as it outputs "layout(binding=)" which
        // isn't supported on OSX's OpenGL.
        for (uint32_t group = 0; group < kMaxBindGroups; ++group) {
            for (uint32_t binding = 0; binding < kMaxBindingsPerGroup; ++binding) {
                const auto& info = bindingInfo[group][binding];
                if (info.used) {
                    compiler.set_name(info.base_type_id, GetBindingName(group, binding));
                    compiler.unset_decoration(info.id, spv::DecorationBinding);
                    compiler.unset_decoration(info.id, spv::DecorationDescriptorSet);
                }
            }
        }

        mGlslSource = compiler.compile();
    }

    const char* ShaderModule::GetSource() const {
        return mGlslSource.c_str();
    }

    const ShaderModule::CombinedSamplerInfo& ShaderModule::GetCombinedSamplerInfo() const {
        return mCombinedInfo;
    }

}}  // namespace dawn_native::opengl
