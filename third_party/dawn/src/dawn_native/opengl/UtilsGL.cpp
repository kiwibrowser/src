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

#include "dawn_native/opengl/UtilsGL.h"

#include "common/Assert.h"

namespace dawn_native { namespace opengl {

    GLuint ToOpenGLCompareFunction(wgpu::CompareFunction compareFunction) {
        switch (compareFunction) {
            case wgpu::CompareFunction::Never:
                return GL_NEVER;
            case wgpu::CompareFunction::Less:
                return GL_LESS;
            case wgpu::CompareFunction::LessEqual:
                return GL_LEQUAL;
            case wgpu::CompareFunction::Greater:
                return GL_GREATER;
            case wgpu::CompareFunction::GreaterEqual:
                return GL_GEQUAL;
            case wgpu::CompareFunction::NotEqual:
                return GL_NOTEQUAL;
            case wgpu::CompareFunction::Equal:
                return GL_EQUAL;
            case wgpu::CompareFunction::Always:
                return GL_ALWAYS;
            default:
                UNREACHABLE();
        }
    }

    GLint GetStencilMaskFromStencilFormat(wgpu::TextureFormat depthStencilFormat) {
        switch (depthStencilFormat) {
            case wgpu::TextureFormat::Depth24PlusStencil8:
                return 0xFF;
            default:
                UNREACHABLE();
        }
    }
}}  // namespace dawn_native::opengl
