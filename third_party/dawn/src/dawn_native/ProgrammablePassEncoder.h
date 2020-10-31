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

#ifndef DAWNNATIVE_PROGRAMMABLEPASSENCODER_H_
#define DAWNNATIVE_PROGRAMMABLEPASSENCODER_H_

#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Error.h"
#include "dawn_native/ObjectBase.h"
#include "dawn_native/PassResourceUsageTracker.h"

#include "dawn_native/dawn_platform.h"

namespace dawn_native {

    class CommandAllocator;
    class DeviceBase;

    // Base class for shared functionality between ComputePassEncoder and RenderPassEncoder.
    class ProgrammablePassEncoder : public ObjectBase {
      public:
        ProgrammablePassEncoder(DeviceBase* device,
                                EncodingContext* encodingContext,
                                PassType passType);

        void InsertDebugMarker(const char* groupLabel);
        void PopDebugGroup();
        void PushDebugGroup(const char* groupLabel);

        void SetBindGroup(uint32_t groupIndex,
                          BindGroupBase* group,
                          uint32_t dynamicOffsetCount,
                          const uint32_t* dynamicOffsets);

      protected:
        // Construct an "error" programmable pass encoder.
        ProgrammablePassEncoder(DeviceBase* device,
                                EncodingContext* encodingContext,
                                ErrorTag errorTag,
                                PassType passType);

        EncodingContext* mEncodingContext = nullptr;
        PassResourceUsageTracker mUsageTracker;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_PROGRAMMABLEPASSENCODER_H_
