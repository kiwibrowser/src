// Copyright 2020 The Dawn Authors
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

#ifndef DAWNNATIVE_MAPREQUESTTRACKER_H_
#define DAWNNATIVE_MAPREQUESTTRACKER_H_

#include "common/SerialQueue.h"
#include "dawn_native/Device.h"

namespace dawn_native {

    // TODO(dawn:22) remove this enum once MapReadAsync/MapWriteAsync are removed.
    enum class MapType : uint32_t { Read, Write, Async };

    class MapRequestTracker {
      public:
        MapRequestTracker(DeviceBase* device);
        ~MapRequestTracker();

        void Track(BufferBase* buffer, uint32_t mapSerial, MapType type);
        void Tick(Serial finishedSerial);

      private:
        DeviceBase* mDevice;

        struct Request {
            Ref<BufferBase> buffer;
            uint32_t mapSerial;
            MapType type;
        };
        SerialQueue<Request> mInflightRequests;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_MAPREQUESTTRACKER_H
