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

#ifndef DAWNNATIVE_OPENGL_UTILSGL_H_
#define DAWNNATIVE_OPENGL_UTILSGL_H_

#include "dawn_native/dawn_platform.h"
#include "dawn_native/opengl/opengl_platform.h"

namespace dawn_native { namespace opengl {

    GLuint ToOpenGLCompareFunction(wgpu::CompareFunction compareFunction);
    GLint GetStencilMaskFromStencilFormat(wgpu::TextureFormat depthStencilFormat);
}}  // namespace dawn_native::opengl

#endif  // DAWNNATIVE_OPENGL_UTILSGL_H_
