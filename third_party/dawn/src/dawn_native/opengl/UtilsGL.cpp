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

    GLuint ToOpenGLCompareFunction(dawn::CompareFunction compareFunction) {
        switch (compareFunction) {
            case dawn::CompareFunction::Never:
                return GL_NEVER;
            case dawn::CompareFunction::Less:
                return GL_LESS;
            case dawn::CompareFunction::LessEqual:
                return GL_LEQUAL;
            case dawn::CompareFunction::Greater:
                return GL_GREATER;
            case dawn::CompareFunction::GreaterEqual:
                return GL_GEQUAL;
            case dawn::CompareFunction::NotEqual:
                return GL_NOTEQUAL;
            case dawn::CompareFunction::Equal:
                return GL_EQUAL;
            case dawn::CompareFunction::Always:
                return GL_ALWAYS;
            default:
                UNREACHABLE();
        }
    }

}}  // namespace dawn_native::opengl
