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

#ifndef DAWNNATIVE_RENDERBUNDLEENCODER_H_
#define DAWNNATIVE_RENDERBUNDLEENCODER_H_

#include "dawn_native/AttachmentState.h"
#include "dawn_native/EncodingContext.h"
#include "dawn_native/Error.h"
#include "dawn_native/RenderBundle.h"
#include "dawn_native/RenderEncoderBase.h"

namespace dawn_native {

    MaybeError ValidateRenderBundleEncoderDescriptor(
        const DeviceBase* device,
        const RenderBundleEncoderDescriptor* descriptor);

    class RenderBundleEncoder final : public RenderEncoderBase {
      public:
        RenderBundleEncoder(DeviceBase* device, const RenderBundleEncoderDescriptor* descriptor);

        static RenderBundleEncoder* MakeError(DeviceBase* device);

        const AttachmentState* GetAttachmentState() const;

        RenderBundleBase* Finish(const RenderBundleDescriptor* descriptor);

        CommandIterator AcquireCommands();

      private:
        RenderBundleEncoder(DeviceBase* device, ErrorTag errorTag);

        MaybeError ValidateFinish(CommandIterator* commands, const PassResourceUsage& usages) const;

        EncodingContext mBundleEncodingContext;
        Ref<AttachmentState> mAttachmentState;
    };
}  // namespace dawn_native

#endif  // DAWNNATIVE_RENDERBUNDLEENCODER_H_
