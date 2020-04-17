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

#ifndef DAWNNATIVE_COMMANDBUFFER_H_
#define DAWNNATIVE_COMMANDBUFFER_H_

#include "dawn_native/dawn_platform.h"

#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"
#include "dawn_native/PassResourceUsage.h"

namespace dawn_native {

    class CommandBufferBase : public ObjectBase {
      public:
        CommandBufferBase(DeviceBase* device, CommandEncoderBase* encoder);
        static CommandBufferBase* MakeError(DeviceBase* device);

        const CommandBufferResourceUsage& GetResourceUsages() const;

      private:
        CommandBufferBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        CommandBufferResourceUsage mResourceUsages;
    };
    bool IsCompleteSubresourceCopiedTo(const TextureBase* texture,
                                       const Extent3D copySize,
                                       const uint32_t mipLevel);

}  // namespace dawn_native

#endif  // DAWNNATIVE_COMMANDBUFFER_H_
