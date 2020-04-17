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

#include "dawn_native/Pipeline.h"

#include "dawn_native/Device.h"
#include "dawn_native/PipelineLayout.h"
#include "dawn_native/ShaderModule.h"

namespace dawn_native {

    MaybeError ValidatePipelineStageDescriptor(DeviceBase* device,
                                               const PipelineStageDescriptor* descriptor,
                                               const PipelineLayoutBase* layout,
                                               dawn::ShaderStage stage) {
        DAWN_TRY(device->ValidateObject(descriptor->module));

        if (descriptor->entryPoint != std::string("main")) {
            return DAWN_VALIDATION_ERROR("Entry point must be \"main\"");
        }
        if (descriptor->module->GetExecutionModel() != stage) {
            return DAWN_VALIDATION_ERROR("Setting module with wrong stages");
        }
        if (!descriptor->module->IsCompatibleWithPipelineLayout(layout)) {
            return DAWN_VALIDATION_ERROR("Stage not compatible with layout");
        }
        return {};
    }

    // PipelineBase

    PipelineBase::PipelineBase(DeviceBase* device,
                               PipelineLayoutBase* layout,
                               dawn::ShaderStageBit stages)
        : ObjectBase(device), mStageMask(stages), mLayout(layout) {
    }

    PipelineBase::PipelineBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag) {
    }

    dawn::ShaderStageBit PipelineBase::GetStageMask() const {
        ASSERT(!IsError());
        return mStageMask;
    }

    PipelineLayoutBase* PipelineBase::GetLayout() {
        ASSERT(!IsError());
        return mLayout.Get();
    }

    const PipelineLayoutBase* PipelineBase::GetLayout() const {
        ASSERT(!IsError());
        return mLayout.Get();
    }

}  // namespace dawn_native
