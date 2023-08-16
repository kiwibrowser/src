// Copyright 2017 The Dawn Authors
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

#ifndef DAWNWIRE_WIRE_H_
#define DAWNWIRE_WIRE_H_

#include <cstdint>

#include "dawn/webgpu.h"
#include "dawn_wire/dawn_wire_export.h"

namespace dawn_wire {

    class DAWN_WIRE_EXPORT CommandSerializer {
      public:
        virtual ~CommandSerializer() = default;
        virtual void* GetCmdSpace(size_t size) = 0;
        virtual bool Flush() = 0;
    };

    class DAWN_WIRE_EXPORT CommandHandler {
      public:
        virtual ~CommandHandler() = default;
        virtual const volatile char* HandleCommands(const volatile char* commands, size_t size) = 0;
    };

    DAWN_WIRE_EXPORT size_t
    SerializedWGPUDevicePropertiesSize(const WGPUDeviceProperties* deviceProperties);

    DAWN_WIRE_EXPORT void SerializeWGPUDeviceProperties(
        const WGPUDeviceProperties* deviceProperties,
        char* serializeBuffer);

    DAWN_WIRE_EXPORT bool DeserializeWGPUDeviceProperties(WGPUDeviceProperties* deviceProperties,
                                                          const volatile char* deserializeBuffer);

}  // namespace dawn_wire

#endif  // DAWNWIRE_WIRE_H_
