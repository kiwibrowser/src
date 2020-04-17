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

#ifndef DAWNNATIVE_OPENGL_PIPELINEGL_H_
#define DAWNNATIVE_OPENGL_PIPELINEGL_H_

#include "dawn_native/Pipeline.h"

#include "glad/glad.h"

#include <vector>

namespace dawn_native { namespace opengl {

    struct OpenGLFunctions;
    class PersistentPipelineState;
    class PipelineLayout;
    class ShaderModule;

    class PipelineGL {
      public:
        PipelineGL();

        void Initialize(const OpenGLFunctions& gl,
                        const PipelineLayout* layout,
                        const PerStage<const ShaderModule*>& modules);

        using BindingLocations =
            std::array<std::array<GLint, kMaxBindingsPerGroup>, kMaxBindGroups>;

        const std::vector<GLuint>& GetTextureUnitsForSampler(GLuint index) const;
        const std::vector<GLuint>& GetTextureUnitsForTextureView(GLuint index) const;
        GLuint GetProgramHandle() const;

        void ApplyNow(const OpenGLFunctions& gl);

      private:
        GLuint mProgram;
        std::vector<std::vector<GLuint>> mUnitsForSamplers;
        std::vector<std::vector<GLuint>> mUnitsForTextures;
    };

}}  // namespace dawn_native::opengl

#endif  // DAWNNATIVE_OPENGL_PIPELINEGL_H_
