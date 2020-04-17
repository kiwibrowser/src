// Copyright 2018 The Dawn Authors
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

#ifndef DAWNNATIVE_STAGINGBUFFER_H_
#define DAWNNATIVE_STAGINGBUFFER_H_

#include "dawn_native/Error.h"

namespace dawn_native {

    class StagingBufferBase {
      public:
        StagingBufferBase(size_t size);
        virtual ~StagingBufferBase() = default;

        virtual MaybeError Initialize() = 0;

        void* GetMappedPointer() const;
        size_t GetSize() const;

      protected:
        void* mMappedPointer = nullptr;

      private:
        const size_t mBufferSize;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_STAGINGBUFFER_H_