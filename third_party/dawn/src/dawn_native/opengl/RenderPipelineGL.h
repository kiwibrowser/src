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

#ifndef DAWNNATIVE_OPENGL_RENDERPIPELINEGL_H_
#define DAWNNATIVE_OPENGL_RENDERPIPELINEGL_H_

#include "dawn_native/RenderPipeline.h"

#include "dawn_native/opengl/PipelineGL.h"

#include "glad/glad.h"

#include <vector>

namespace dawn_native { namespace opengl {

    class Device;
    class PersistentPipelineState;

    class RenderPipeline : public RenderPipelineBase, public PipelineGL {
      public:
        RenderPipeline(Device* device, const RenderPipelineDescriptor* descriptor);
        ~RenderPipeline();

        GLenum GetGLPrimitiveTopology() const;

        void ApplyNow(PersistentPipelineState& persistentPipelineState);

      private:
        void CreateVAOForVertexInput(const VertexInputDescriptor* vertexInput);

        // TODO(yunchao.he@intel.com): vao need to be deduplicated between pipelines.
        GLuint mVertexArrayObject;
        GLenum mGlPrimitiveTopology;
    };

}}  // namespace dawn_native::opengl

#endif  // DAWNNATIVE_OPENGL_RENDERPIPELINEGL_H_
