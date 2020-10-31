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

#include "dawn_native/ErrorScopeTracker.h"

#include "dawn_native/Device.h"
#include "dawn_native/ErrorScope.h"

#include <limits>

namespace dawn_native {

    ErrorScopeTracker::ErrorScopeTracker(DeviceBase* device) : mDevice(device) {
    }

    ErrorScopeTracker::~ErrorScopeTracker() {
        // The tracker is destroyed when the Device is destroyed. We need to
        // call Destroy on all in-flight error scopes so they resolve their callbacks
        // with UNKNOWN.
        constexpr Serial maxSerial = std::numeric_limits<Serial>::max();
        for (Ref<ErrorScope>& scope : mScopesInFlight.IterateUpTo(maxSerial)) {
            scope->UnlinkForShutdown();
        }
        Tick(maxSerial);
    }

    void ErrorScopeTracker::TrackUntilLastSubmitComplete(ErrorScope* scope) {
        mScopesInFlight.Enqueue(scope, mDevice->GetLastSubmittedCommandSerial());
        mDevice->AddFutureCallbackSerial(mDevice->GetPendingCommandSerial());
    }

    void ErrorScopeTracker::Tick(Serial completedSerial) {
        mScopesInFlight.ClearUpTo(completedSerial);
    }

}  // namespace dawn_native
