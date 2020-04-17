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

#include "dawn_native/Fence.h"

#include "common/Assert.h"
#include "dawn_native/Device.h"
#include "dawn_native/Queue.h"
#include "dawn_native/ValidationUtils_autogen.h"

#include <utility>

namespace dawn_native {

    MaybeError ValidateFenceDescriptor(const FenceDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        return {};
    }

    // Fence

    FenceBase::FenceBase(QueueBase* queue, const FenceDescriptor* descriptor)
        : ObjectBase(queue->GetDevice()),
          mSignalValue(descriptor->initialValue),
          mCompletedValue(descriptor->initialValue),
          mQueue(queue) {
    }

    FenceBase::FenceBase(DeviceBase* device, ObjectBase::ErrorTag tag) : ObjectBase(device, tag) {
    }

    FenceBase::~FenceBase() {
        for (auto& request : mRequests.IterateAll()) {
            ASSERT(!IsError());
            request.completionCallback(DAWN_FENCE_COMPLETION_STATUS_UNKNOWN, request.userdata);
        }
        mRequests.Clear();
    }

    // static
    FenceBase* FenceBase::MakeError(DeviceBase* device) {
        return new FenceBase(device, ObjectBase::kError);
    }

    uint64_t FenceBase::GetCompletedValue() const {
        if (IsError()) {
            return 0;
        }
        return mCompletedValue;
    }

    void FenceBase::OnCompletion(uint64_t value,
                                 dawn::FenceOnCompletionCallback callback,
                                 void* userdata) {
        if (GetDevice()->ConsumedError(ValidateOnCompletion(value))) {
            callback(DAWN_FENCE_COMPLETION_STATUS_ERROR, userdata);
            return;
        }
        ASSERT(!IsError());

        if (value <= mCompletedValue) {
            callback(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, userdata);
            return;
        }

        OnCompletionData request;
        request.completionCallback = callback;
        request.userdata = userdata;
        mRequests.Enqueue(std::move(request), value);
    }

    uint64_t FenceBase::GetSignaledValue() const {
        ASSERT(!IsError());
        return mSignalValue;
    }

    const QueueBase* FenceBase::GetQueue() const {
        ASSERT(!IsError());
        return mQueue.Get();
    }

    void FenceBase::SetSignaledValue(uint64_t signalValue) {
        ASSERT(!IsError());
        ASSERT(signalValue > mSignalValue);
        mSignalValue = signalValue;
    }

    void FenceBase::SetCompletedValue(uint64_t completedValue) {
        ASSERT(!IsError());
        ASSERT(completedValue <= mSignalValue);
        ASSERT(completedValue > mCompletedValue);
        mCompletedValue = completedValue;

        for (auto& request : mRequests.IterateUpTo(mCompletedValue)) {
            request.completionCallback(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, request.userdata);
        }
        mRequests.ClearUpTo(mCompletedValue);
    }

    MaybeError FenceBase::ValidateOnCompletion(uint64_t value) const {
        DAWN_TRY(GetDevice()->ValidateObject(this));
        if (value > mSignalValue) {
            return DAWN_VALIDATION_ERROR("Value greater than fence signaled value");
        }
        return {};
    }

}  // namespace dawn_native
