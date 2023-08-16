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

#include "dawn_native/opengl/PipelineGL.h"

#include "common/BitSetIterator.h"
#include "common/Log.h"
#include "dawn_native/BindGroupLayout.h"
#include "dawn_native/opengl/Forward.h"
#include "dawn_native/opengl/OpenGLFunctions.h"
#include "dawn_native/opengl/PipelineLayoutGL.h"
#include "dawn_native/opengl/ShaderModuleGL.h"

#include <set>

namespace dawn_native { namespace opengl {

    namespace {

        GLenum GLShaderType(SingleShaderStage stage) {
            switch (stage) {
                case SingleShaderStage::Vertex:
                    return GL_VERTEX_SHADER;
                case SingleShaderStage::Fragment:
                    return GL_FRAGMENT_SHADER;
                case SingleShaderStage::Compute:
                    return GL_COMPUTE_SHADER;
                default:
                    UNREACHABLE();
            }
        }

    }  // namespace

    PipelineGL::PipelineGL() {
    }

    void PipelineGL::Initialize(const OpenGLFunctions& gl,
                                const PipelineLayout* layout,
                                const PerStage<const ShaderModule*>& modules) {
        auto CreateShader = [](const OpenGLFunctions& gl, GLenum type,
                               const char* source) -> GLuint {
            GLuint shader = gl.CreateShader(type);
            gl.ShaderSource(shader, 1, &source, nullptr);
            gl.CompileShader(shader);

            GLint compileStatus = GL_FALSE;
            gl.GetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
            if (compileStatus == GL_FALSE) {
                GLint infoLogLength = 0;
                gl.GetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

                if (infoLogLength > 1) {
                    std::vector<char> buffer(infoLogLength);
                    gl.GetShaderInfoLog(shader, infoLogLength, nullptr, &buffer[0]);
                    dawn::ErrorLog() << source << "\nProgram compilation failed:\n"
                                     << buffer.data();
                }
            }
            return shader;
        };

        mProgram = gl.CreateProgram();

        wgpu::ShaderStage activeStages = wgpu::ShaderStage::None;
        for (SingleShaderStage stage : IterateStages(kAllStages)) {
            if (modules[stage] != nullptr) {
                activeStages |= StageBit(stage);
            }
        }

        for (SingleShaderStage stage : IterateStages(activeStages)) {
            GLuint shader = CreateShader(gl, GLShaderType(stage), modules[stage]->GetSource());
            gl.AttachShader(mProgram, shader);
        }

        gl.LinkProgram(mProgram);

        GLint linkStatus = GL_FALSE;
        gl.GetProgramiv(mProgram, GL_LINK_STATUS, &linkStatus);
        if (linkStatus == GL_FALSE) {
            GLint infoLogLength = 0;
            gl.GetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &infoLogLength);

            if (infoLogLength > 1) {
                std::vector<char> buffer(infoLogLength);
                gl.GetProgramInfoLog(mProgram, infoLogLength, nullptr, &buffer[0]);
                dawn::ErrorLog() << "Program link failed:\n" << buffer.data();
            }
        }

        gl.UseProgram(mProgram);

        // The uniforms are part of the program state so we can pre-bind buffer units, texture units
        // etc.
        const auto& indices = layout->GetBindingIndexInfo();

        for (BindGroupIndex group : IterateBitSet(layout->GetBindGroupLayoutsMask())) {
            const BindGroupLayoutBase* bgl = layout->GetBindGroupLayout(group);

            for (const auto& it : bgl->GetBindingMap()) {
                BindingNumber bindingNumber = it.first;
                BindingIndex bindingIndex = it.second;

                std::string name = GetBindingName(group, bindingNumber);
                switch (bgl->GetBindingInfo(bindingIndex).type) {
                    case wgpu::BindingType::UniformBuffer: {
                        GLint location = gl.GetUniformBlockIndex(mProgram, name.c_str());
                        if (location != -1) {
                            gl.UniformBlockBinding(mProgram, location,
                                                   indices[group][bindingIndex]);
                        }
                        break;
                    }

                    case wgpu::BindingType::StorageBuffer:
                    case wgpu::BindingType::ReadonlyStorageBuffer: {
                        GLuint location = gl.GetProgramResourceIndex(
                            mProgram, GL_SHADER_STORAGE_BLOCK, name.c_str());
                        if (location != GL_INVALID_INDEX) {
                            gl.ShaderStorageBlockBinding(mProgram, location,
                                                         indices[group][bindingIndex]);
                        }
                        break;
                    }

                    case wgpu::BindingType::Sampler:
                    case wgpu::BindingType::ComparisonSampler:
                    case wgpu::BindingType::SampledTexture:
                        // These binding types are handled in the separate sampler and texture
                        // emulation
                        break;

                    case wgpu::BindingType::ReadonlyStorageTexture:
                    case wgpu::BindingType::WriteonlyStorageTexture: {
                        GLint location = gl.GetUniformLocation(mProgram, name.c_str());
                        if (location != -1) {
                            gl.Uniform1i(location, indices[group][bindingIndex]);
                        }
                        break;
                    }

                    case wgpu::BindingType::StorageTexture:
                        UNREACHABLE();
                        break;

                        // TODO(shaobo.yan@intel.com): Implement dynamic buffer offset.
                }
            }
        }

        // Compute links between stages for combined samplers, then bind them to texture units
        {
            std::set<CombinedSampler> combinedSamplersSet;
            for (SingleShaderStage stage : IterateStages(activeStages)) {
                for (const auto& combined : modules[stage]->GetCombinedSamplerInfo()) {
                    combinedSamplersSet.insert(combined);
                }
            }

            mUnitsForSamplers.resize(layout->GetNumSamplers());
            mUnitsForTextures.resize(layout->GetNumSampledTextures());

            GLuint textureUnit = layout->GetTextureUnitsUsed();
            for (const auto& combined : combinedSamplersSet) {
                std::string name = combined.GetName();
                GLint location = gl.GetUniformLocation(mProgram, name.c_str());

                if (location == -1) {
                    continue;
                }

                gl.Uniform1i(location, textureUnit);

                bool shouldUseFiltering;
                {
                    const BindGroupLayoutBase* bgl =
                        layout->GetBindGroupLayout(combined.textureLocation.group);
                    BindingIndex bindingIndex =
                        bgl->GetBindingIndex(combined.textureLocation.binding);

                    GLuint textureIndex = indices[combined.textureLocation.group][bindingIndex];
                    mUnitsForTextures[textureIndex].push_back(textureUnit);

                    Format::Type componentType =
                        bgl->GetBindingInfo(bindingIndex).textureComponentType;
                    shouldUseFiltering = componentType == Format::Type::Float;
                }
                {
                    const BindGroupLayoutBase* bgl =
                        layout->GetBindGroupLayout(combined.samplerLocation.group);
                    BindingIndex bindingIndex =
                        bgl->GetBindingIndex(combined.samplerLocation.binding);

                    GLuint samplerIndex = indices[combined.samplerLocation.group][bindingIndex];
                    mUnitsForSamplers[samplerIndex].push_back({textureUnit, shouldUseFiltering});
                }

                textureUnit++;
            }
        }
    }

    const std::vector<PipelineGL::SamplerUnit>& PipelineGL::GetTextureUnitsForSampler(
        GLuint index) const {
        ASSERT(index < mUnitsForSamplers.size());
        return mUnitsForSamplers[index];
    }

    const std::vector<GLuint>& PipelineGL::GetTextureUnitsForTextureView(GLuint index) const {
        ASSERT(index < mUnitsForTextures.size());
        return mUnitsForTextures[index];
    }

    GLuint PipelineGL::GetProgramHandle() const {
        return mProgram;
    }

    void PipelineGL::ApplyNow(const OpenGLFunctions& gl) {
        gl.UseProgram(mProgram);
    }

}}  // namespace dawn_native::opengl
