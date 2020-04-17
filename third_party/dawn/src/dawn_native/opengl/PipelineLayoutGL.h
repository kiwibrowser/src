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

#ifndef DAWNNATIVE_OPENGL_PIPELINELAYOUTGL_H_
#define DAWNNATIVE_OPENGL_PIPELINELAYOUTGL_H_

#include "dawn_native/PipelineLayout.h"

#include "glad/glad.h"

namespace dawn_native { namespace opengl {

    class Device;

    class PipelineLayout : public PipelineLayoutBase {
      public:
        PipelineLayout(Device* device, const PipelineLayoutDescriptor* descriptor);

        using BindingIndexInfo =
            std::array<std::array<GLuint, kMaxBindingsPerGroup>, kMaxBindGroups>;
        const BindingIndexInfo& GetBindingIndexInfo() const;

        GLuint GetTextureUnitsUsed() const;
        size_t GetNumSamplers() const;
        size_t GetNumSampledTextures() const;

      private:
        BindingIndexInfo mIndexInfo;
        size_t mNumSamplers;
        size_t mNumSampledTextures;
    };

}}  // namespace dawn_native::opengl

#endif  // DAWNNATIVE_OPENGL_PIPELINELAYOUTGL_H_
