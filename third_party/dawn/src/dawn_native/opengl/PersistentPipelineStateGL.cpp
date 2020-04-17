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

#include "dawn_native/opengl/PersistentPipelineStateGL.h"

#include "dawn_native/opengl/OpenGLFunctions.h"

namespace dawn_native { namespace opengl {

    void PersistentPipelineState::SetDefaultState(const OpenGLFunctions& gl) {
        CallGLStencilFunc(gl);
    }

    void PersistentPipelineState::SetStencilFuncsAndMask(const OpenGLFunctions& gl,
                                                         GLenum stencilBackCompareFunction,
                                                         GLenum stencilFrontCompareFunction,
                                                         uint32_t stencilReadMask) {
        if (mStencilBackCompareFunction == stencilBackCompareFunction &&
            mStencilFrontCompareFunction == stencilFrontCompareFunction &&
            mStencilReadMask == stencilReadMask) {
            return;
        }

        mStencilBackCompareFunction = stencilBackCompareFunction;
        mStencilFrontCompareFunction = stencilFrontCompareFunction;
        mStencilReadMask = stencilReadMask;
        CallGLStencilFunc(gl);
    }

    void PersistentPipelineState::SetStencilReference(const OpenGLFunctions& gl,
                                                      uint32_t stencilReference) {
        if (mStencilReference == stencilReference) {
            return;
        }

        mStencilReference = stencilReference;
        CallGLStencilFunc(gl);
    }

    void PersistentPipelineState::CallGLStencilFunc(const OpenGLFunctions& gl) {
        gl.StencilFuncSeparate(GL_BACK, mStencilBackCompareFunction, mStencilReference,
                               mStencilReadMask);
        gl.StencilFuncSeparate(GL_FRONT, mStencilFrontCompareFunction, mStencilReference,
                               mStencilReadMask);
    }

}}  // namespace dawn_native::opengl
