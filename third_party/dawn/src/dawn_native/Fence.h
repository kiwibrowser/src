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

#ifndef DAWNNATIVE_FENCE_H_
#define DAWNNATIVE_FENCE_H_

#include "common/SerialMap.h"
#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"

#include "dawn_native/dawn_platform.h"

#include <map>

namespace dawn_native {

    MaybeError ValidateFenceDescriptor(const FenceDescriptor* descriptor);

    class FenceBase : public ObjectBase {
      public:
        FenceBase(QueueBase* queue, const FenceDescriptor* descriptor);
        ~FenceBase();

        static FenceBase* MakeError(DeviceBase* device);

        uint64_t GetSignaledValue() const;
        const QueueBase* GetQueue() const;

        // Dawn API
        uint64_t GetCompletedValue() const;
        void OnCompletion(uint64_t value, dawn::FenceOnCompletionCallback callback, void* userdata);

      protected:
        friend class QueueBase;
        friend class FenceSignalTracker;
        void SetSignaledValue(uint64_t signalValue);
        void SetCompletedValue(uint64_t completedValue);

      private:
        FenceBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        MaybeError ValidateOnCompletion(uint64_t value) const;

        struct OnCompletionData {
            dawn::FenceOnCompletionCallback completionCallback = nullptr;
            void* userdata = nullptr;
        };

        uint64_t mSignalValue;
        uint64_t mCompletedValue;
        Ref<QueueBase> mQueue;
        SerialMap<OnCompletionData> mRequests;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_FENCE_H_
