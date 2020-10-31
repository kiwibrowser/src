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

    Fence::Fence(QueueBase* queue, const FenceDescriptor* descriptor)
        : ObjectBase(queue->GetDevice()),
          mSignalValue(descriptor->initialValue),
          mCompletedValue(descriptor->initialValue),
          mQueue(queue) {
    }

    Fence::Fence(DeviceBase* device, ObjectBase::ErrorTag tag) : ObjectBase(device, tag) {
    }

    Fence::~Fence() {
        for (auto& request : mRequests.IterateAll()) {
            ASSERT(!IsError());
            request.completionCallback(WGPUFenceCompletionStatus_Unknown, request.userdata);
        }
        mRequests.Clear();
    }

    // static
    Fence* Fence::MakeError(DeviceBase* device) {
        return new Fence(device, ObjectBase::kError);
    }

    uint64_t Fence::GetCompletedValue() const {
        if (IsError()) {
            return 0;
        }
        return mCompletedValue;
    }

    void Fence::OnCompletion(uint64_t value,
                             wgpu::FenceOnCompletionCallback callback,
                             void* userdata) {
        WGPUFenceCompletionStatus status;
        if (GetDevice()->ConsumedError(ValidateOnCompletion(value, &status))) {
            callback(status, userdata);
            return;
        }
        ASSERT(!IsError());

        if (value <= mCompletedValue) {
            callback(WGPUFenceCompletionStatus_Success, userdata);
            return;
        }

        OnCompletionData request;
        request.completionCallback = callback;
        request.userdata = userdata;
        mRequests.Enqueue(std::move(request), value);
    }

    uint64_t Fence::GetSignaledValue() const {
        ASSERT(!IsError());
        return mSignalValue;
    }

    const QueueBase* Fence::GetQueue() const {
        ASSERT(!IsError());
        return mQueue.Get();
    }

    void Fence::SetSignaledValue(uint64_t signalValue) {
        ASSERT(!IsError());
        ASSERT(signalValue > mSignalValue);
        mSignalValue = signalValue;
    }

    void Fence::SetCompletedValue(uint64_t completedValue) {
        ASSERT(!IsError());
        ASSERT(completedValue <= mSignalValue);
        ASSERT(completedValue > mCompletedValue);
        mCompletedValue = completedValue;

        for (auto& request : mRequests.IterateUpTo(mCompletedValue)) {
            if (GetDevice()->IsLost()) {
                request.completionCallback(WGPUFenceCompletionStatus_DeviceLost, request.userdata);
            } else {
                request.completionCallback(WGPUFenceCompletionStatus_Success, request.userdata);
            }
        }
        mRequests.ClearUpTo(mCompletedValue);
    }

    MaybeError Fence::ValidateOnCompletion(uint64_t value,
                                           WGPUFenceCompletionStatus* status) const {
        *status = WGPUFenceCompletionStatus_DeviceLost;
        DAWN_TRY(GetDevice()->ValidateIsAlive());

        *status = WGPUFenceCompletionStatus_Error;
        DAWN_TRY(GetDevice()->ValidateObject(this));

        if (value > mSignalValue) {
            return DAWN_VALIDATION_ERROR("Value greater than fence signaled value");
        }

        *status = WGPUFenceCompletionStatus_Success;
        return {};
    }

}  // namespace dawn_native
