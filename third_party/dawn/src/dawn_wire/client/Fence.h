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

#ifndef DAWNWIRE_CLIENT_FENCE_H_
#define DAWNWIRE_CLIENT_FENCE_H_

#include <dawn/webgpu.h>

#include "common/SerialMap.h"
#include "dawn_wire/client/ObjectBase.h"

namespace dawn_wire { namespace client {

    class Queue;
    class Fence : public ObjectBase {
      public:
        using ObjectBase::ObjectBase;

        ~Fence();
        void Initialize(Queue* queue, const WGPUFenceDescriptor* descriptor);

        void CheckPassedFences();
        void OnCompletion(uint64_t value, WGPUFenceOnCompletionCallback callback, void* userdata);
        void OnUpdateCompletedValueCallback(uint64_t value);

        uint64_t GetCompletedValue() const;
        uint64_t GetSignaledValue() const;
        Queue* GetQueue() const;

        void SetSignaledValue(uint64_t value);

      private:
        struct OnCompletionData {
            WGPUFenceOnCompletionCallback completionCallback = nullptr;
            void* userdata = nullptr;
        };
        Queue* mQueue = nullptr;
        uint64_t mSignaledValue = 0;
        uint64_t mCompletedValue = 0;
        SerialMap<OnCompletionData> mRequests;
    };

}}  // namespace dawn_wire::client

#endif  // DAWNWIRE_CLIENT_FENCE_H_
