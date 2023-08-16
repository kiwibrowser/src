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

#include "dawn_native/ErrorInjector.h"

#include "common/Assert.h"
#include "dawn_native/DawnNative.h"

namespace dawn_native {

    namespace {

        bool sIsEnabled = false;
        uint64_t sNextIndex = 0;
        uint64_t sInjectedFailureIndex = 0;
        bool sHasPendingInjectedError = false;

    }  // anonymous namespace

    void EnableErrorInjector() {
        sIsEnabled = true;
    }

    void DisableErrorInjector() {
        sIsEnabled = false;
    }

    void ClearErrorInjector() {
        sNextIndex = 0;
        sHasPendingInjectedError = false;
    }

    bool ErrorInjectorEnabled() {
        return sIsEnabled;
    }

    uint64_t AcquireErrorInjectorCallCount() {
        uint64_t count = sNextIndex;
        ClearErrorInjector();
        return count;
    }

    bool ShouldInjectError() {
        uint64_t index = sNextIndex++;
        if (sHasPendingInjectedError && index == sInjectedFailureIndex) {
            sHasPendingInjectedError = false;
            return true;
        }
        return false;
    }

    void InjectErrorAt(uint64_t index) {
        // Only one error can be injected at a time.
        ASSERT(!sHasPendingInjectedError);
        sInjectedFailureIndex = index;
        sHasPendingInjectedError = true;
    }

}  // namespace dawn_native
