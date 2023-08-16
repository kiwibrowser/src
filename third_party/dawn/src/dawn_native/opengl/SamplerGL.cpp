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

#include "dawn_native/opengl/SamplerGL.h"

#include "common/Assert.h"
#include "dawn_native/opengl/DeviceGL.h"
#include "dawn_native/opengl/UtilsGL.h"

namespace dawn_native { namespace opengl {

    namespace {
        GLenum MagFilterMode(wgpu::FilterMode filter) {
            switch (filter) {
                case wgpu::FilterMode::Nearest:
                    return GL_NEAREST;
                case wgpu::FilterMode::Linear:
                    return GL_LINEAR;
                default:
                    UNREACHABLE();
            }
        }

        GLenum MinFilterMode(wgpu::FilterMode minFilter, wgpu::FilterMode mipMapFilter) {
            switch (minFilter) {
                case wgpu::FilterMode::Nearest:
                    switch (mipMapFilter) {
                        case wgpu::FilterMode::Nearest:
                            return GL_NEAREST_MIPMAP_NEAREST;
                        case wgpu::FilterMode::Linear:
                            return GL_NEAREST_MIPMAP_LINEAR;
                        default:
                            UNREACHABLE();
                    }
                case wgpu::FilterMode::Linear:
                    switch (mipMapFilter) {
                        case wgpu::FilterMode::Nearest:
                            return GL_LINEAR_MIPMAP_NEAREST;
                        case wgpu::FilterMode::Linear:
                            return GL_LINEAR_MIPMAP_LINEAR;
                        default:
                            UNREACHABLE();
                    }
                default:
                    UNREACHABLE();
            }
        }

        GLenum WrapMode(wgpu::AddressMode mode) {
            switch (mode) {
                case wgpu::AddressMode::Repeat:
                    return GL_REPEAT;
                case wgpu::AddressMode::MirrorRepeat:
                    return GL_MIRRORED_REPEAT;
                case wgpu::AddressMode::ClampToEdge:
                    return GL_CLAMP_TO_EDGE;
                default:
                    UNREACHABLE();
            }
        }

    }  // namespace

    Sampler::Sampler(Device* device, const SamplerDescriptor* descriptor)
        : SamplerBase(device, descriptor) {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;

        gl.GenSamplers(1, &mFilteringHandle);
        SetupGLSampler(mFilteringHandle, descriptor, false);

        gl.GenSamplers(1, &mNonFilteringHandle);
        SetupGLSampler(mNonFilteringHandle, descriptor, true);
    }

    Sampler::~Sampler() {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;
        gl.DeleteSamplers(1, &mFilteringHandle);
        gl.DeleteSamplers(1, &mNonFilteringHandle);
    }

    void Sampler::SetupGLSampler(GLuint sampler,
                                 const SamplerDescriptor* descriptor,
                                 bool forceNearest) {
        const OpenGLFunctions& gl = ToBackend(GetDevice())->gl;

        if (forceNearest) {
            gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        } else {
            gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER,
                                 MagFilterMode(descriptor->magFilter));
            gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER,
                                 MinFilterMode(descriptor->minFilter, descriptor->mipmapFilter));
        }
        gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_R, WrapMode(descriptor->addressModeW));
        gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, WrapMode(descriptor->addressModeU));
        gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, WrapMode(descriptor->addressModeV));

        gl.SamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, descriptor->lodMinClamp);
        gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, descriptor->lodMaxClamp);

        if (descriptor->compare != wgpu::CompareFunction::Undefined) {
            gl.SamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            gl.SamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC,
                                 ToOpenGLCompareFunction(descriptor->compare));
        }
    }

    GLuint Sampler::GetFilteringHandle() const {
        return mFilteringHandle;
    }

    GLuint Sampler::GetNonFilteringHandle() const {
        return mNonFilteringHandle;
    }

}}  // namespace dawn_native::opengl
