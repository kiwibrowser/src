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

#ifndef DAWNNATIVE_OPENGL_TEXTUREGL_H_
#define DAWNNATIVE_OPENGL_TEXTUREGL_H_

#include "dawn_native/Texture.h"

#include "dawn_native/opengl/opengl_platform.h"

namespace dawn_native { namespace opengl {

    class Device;
    struct GLFormat;

    class Texture final : public TextureBase {
      public:
        Texture(Device* device, const TextureDescriptor* descriptor);
        Texture(Device* device,
                const TextureDescriptor* descriptor,
                GLuint handle,
                TextureState state);

        GLuint GetHandle() const;
        GLenum GetGLTarget() const;
        const GLFormat& GetGLFormat() const;

        void EnsureSubresourceContentInitialized(const SubresourceRange& range);

      private:
        ~Texture() override;

        void DestroyImpl() override;
        MaybeError ClearTexture(const SubresourceRange& range, TextureBase::ClearValue clearValue);

        GLuint mHandle;
        GLenum mTarget;
    };

    class TextureView final : public TextureViewBase {
      public:
        TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor);

        GLuint GetHandle() const;
        GLenum GetGLTarget() const;

      private:
        ~TextureView() override;

        GLuint mHandle;
        GLenum mTarget;
        bool mOwnsHandle;
    };

}}  // namespace dawn_native::opengl

#endif  // DAWNNATIVE_OPENGL_TEXTUREGL_H_
