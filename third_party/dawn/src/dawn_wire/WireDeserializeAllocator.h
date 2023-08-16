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

#ifndef DAWNWIRE_WIREDESERIALIZEALLOCATOR_H_
#define DAWNWIRE_WIREDESERIALIZEALLOCATOR_H_

#include "dawn_wire/WireCmd_autogen.h"

#include <vector>

namespace dawn_wire {
    // A really really simple implementation of the DeserializeAllocator. It's main feature
    // is that it has some inline storage so as to avoid allocations for the majority of
    // commands.
    class WireDeserializeAllocator : public DeserializeAllocator {
      public:
        WireDeserializeAllocator();
        virtual ~WireDeserializeAllocator();

        void* GetSpace(size_t size) override;

        void Reset();

      private:
        size_t mRemainingSize = 0;
        char* mCurrentBuffer = nullptr;
        char mStaticBuffer[2048];
        std::vector<char*> mAllocations;
    };
}  // namespace dawn_wire

#endif  // DAWNWIRE_WIREDESERIALIZEALLOCATOR_H_
