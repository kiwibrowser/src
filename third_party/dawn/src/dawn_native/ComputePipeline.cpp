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

#include "dawn_native/ComputePipeline.h"

#include "common/HashUtils.h"
#include "dawn_native/Device.h"

namespace dawn_native {

    MaybeError ValidateComputePipelineDescriptor(DeviceBase* device,
                                                 const ComputePipelineDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        DAWN_TRY(device->ValidateObject(descriptor->layout));
        DAWN_TRY(ValidatePipelineStageDescriptor(device, descriptor->computeStage,
                                                 descriptor->layout, dawn::ShaderStage::Compute));
        return {};
    }

    // ComputePipelineBase

    ComputePipelineBase::ComputePipelineBase(DeviceBase* device,
                                             const ComputePipelineDescriptor* descriptor,
                                             bool blueprint)
        : PipelineBase(device, descriptor->layout, dawn::ShaderStageBit::Compute),
          mModule(descriptor->computeStage->module),
          mEntryPoint(descriptor->computeStage->entryPoint),
          mIsBlueprint(blueprint) {
    }

    ComputePipelineBase::ComputePipelineBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : PipelineBase(device, tag) {
    }

    ComputePipelineBase::~ComputePipelineBase() {
        // Do not uncache the actual cached object if we are a blueprint
        if (!mIsBlueprint && !IsError()) {
            GetDevice()->UncacheComputePipeline(this);
        }
    }

    // static
    ComputePipelineBase* ComputePipelineBase::MakeError(DeviceBase* device) {
        return new ComputePipelineBase(device, ObjectBase::kError);
    }

    size_t ComputePipelineBase::HashFunc::operator()(const ComputePipelineBase* pipeline) const {
        size_t hash = 0;
        HashCombine(&hash, pipeline->mModule.Get(), pipeline->mEntryPoint, pipeline->GetLayout());
        return hash;
    }

    bool ComputePipelineBase::EqualityFunc::operator()(const ComputePipelineBase* a,
                                                       const ComputePipelineBase* b) const {
        return a->mModule.Get() == b->mModule.Get() && a->mEntryPoint == b->mEntryPoint &&
               a->GetLayout() == b->GetLayout();
    }

}  // namespace dawn_native
