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

#include "dawn_wire/client/Fence.h"

#include "dawn_wire/client/Device.h"

namespace dawn_wire { namespace client {

    Fence::~Fence() {
        // Callbacks need to be fired in all cases, as they can handle freeing resources
        // so we call them with "Unknown" status.
        for (auto& request : mRequests.IterateAll()) {
            request.completionCallback(WGPUFenceCompletionStatus_Unknown, request.userdata);
        }
        mRequests.Clear();
    }

    void Fence::Initialize(Queue* queue, const WGPUFenceDescriptor* descriptor) {
        mQueue = queue;

        uint64_t initialValue = descriptor != nullptr ? descriptor->initialValue : 0u;
        mSignaledValue = initialValue;
        mCompletedValue = initialValue;
    }

    void Fence::CheckPassedFences() {
        for (auto& request : mRequests.IterateUpTo(mCompletedValue)) {
            request.completionCallback(WGPUFenceCompletionStatus_Success, request.userdata);
        }
        mRequests.ClearUpTo(mCompletedValue);
    }

    void Fence::OnCompletion(uint64_t value,
                             WGPUFenceOnCompletionCallback callback,
                             void* userdata) {
        if (value > mSignaledValue) {
            device->InjectError(WGPUErrorType_Validation,
                                "Value greater than fence signaled value");
            callback(WGPUFenceCompletionStatus_Error, userdata);
            return;
        }

        if (value <= mCompletedValue) {
            callback(WGPUFenceCompletionStatus_Success, userdata);
            return;
        }

        Fence::OnCompletionData request;
        request.completionCallback = callback;
        request.userdata = userdata;
        mRequests.Enqueue(std::move(request), value);
    }

    void Fence::OnUpdateCompletedValueCallback(uint64_t value) {
        mCompletedValue = value;
        CheckPassedFences();
    }

    uint64_t Fence::GetCompletedValue() const {
        return mCompletedValue;
    }

    uint64_t Fence::GetSignaledValue() const {
        return mSignaledValue;
    }

    Queue* Fence::GetQueue() const {
        return mQueue;
    }

    void Fence::SetSignaledValue(uint64_t value) {
        mSignaledValue = value;
    }

}}  // namespace dawn_wire::client
