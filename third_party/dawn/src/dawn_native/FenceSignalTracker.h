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

#ifndef DAWNNATIVE_FENCESIGNALTRACKER_H_
#define DAWNNATIVE_FENCESIGNALTRACKER_H_

#include "common/SerialQueue.h"
#include "dawn_native/RefCounted.h"

namespace dawn_native {

    class DeviceBase;
    class FenceBase;

    class FenceSignalTracker {
        struct FenceInFlight {
            Ref<FenceBase> fence;
            uint64_t value;
        };

      public:
        FenceSignalTracker(DeviceBase* device);
        ~FenceSignalTracker();

        void UpdateFenceOnComplete(FenceBase* fence, uint64_t value);

        void Tick(Serial finishedSerial);

      private:
        DeviceBase* mDevice;
        SerialQueue<FenceInFlight> mFencesInFlight;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_FENCESIGNALTRACKER_H_
