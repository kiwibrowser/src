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

#ifndef DAWNNATIVE_OPENGL_BUFFERGL_H_
#define DAWNNATIVE_OPENGL_BUFFERGL_H_

#include "dawn_native/Buffer.h"

#include "dawn_native/opengl/opengl_platform.h"

namespace dawn_native { namespace opengl {

    class Device;

    class Buffer final : public BufferBase {
      public:
        Buffer(Device* device, const BufferDescriptor* descriptor);

        GLuint GetHandle() const;

        void EnsureDataInitialized();
        void EnsureDataInitializedAsDestination(uint64_t offset, uint64_t size);
        void EnsureDataInitializedAsDestination(const CopyTextureToBufferCmd* copy);

      private:
        ~Buffer() override;
        // Dawn API
        MaybeError MapReadAsyncImpl() override;
        MaybeError MapWriteAsyncImpl() override;
        MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) override;
        void UnmapImpl() override;
        void DestroyImpl() override;

        bool IsMappableAtCreation() const override;
        MaybeError MapAtCreationImpl() override;
        void* GetMappedPointerImpl() override;
        uint64_t GetAppliedSize() const;

        void InitializeToZero();

        GLuint mBuffer = 0;
        void* mMappedData = nullptr;
    };

}}  // namespace dawn_native::opengl

#endif  // DAWNNATIVE_OPENGL_BUFFERGL_H_
