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

#include "dawn_native/FenceSignalTracker.h"

#include "dawn_native/Device.h"
#include "dawn_native/Fence.h"

namespace dawn_native {

    FenceSignalTracker::FenceSignalTracker(DeviceBase* device) : mDevice(device) {
    }

    FenceSignalTracker::~FenceSignalTracker() {
        ASSERT(mFencesInFlight.Empty());
    }

    void FenceSignalTracker::UpdateFenceOnComplete(Fence* fence, uint64_t value) {
        // Because we currently only have a single queue, we can simply update
        // the fence completed value once the last submitted serial has passed.
        mFencesInFlight.Enqueue(FenceInFlight{fence, value},
                                mDevice->GetLastSubmittedCommandSerial());
        mDevice->AddFutureCallbackSerial(mDevice->GetPendingCommandSerial());
    }

    void FenceSignalTracker::Tick(Serial finishedSerial) {
        for (auto& fenceInFlight : mFencesInFlight.IterateUpTo(finishedSerial)) {
            fenceInFlight.fence->SetCompletedValue(fenceInFlight.value);
        }
        mFencesInFlight.ClearUpTo(finishedSerial);
    }

}  // namespace dawn_native
