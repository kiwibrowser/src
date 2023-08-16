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

#ifndef DAWNNATIVE_COMPUTEPIPELINE_H_
#define DAWNNATIVE_COMPUTEPIPELINE_H_

#include "dawn_native/Pipeline.h"

namespace dawn_native {

    class DeviceBase;

    MaybeError ValidateComputePipelineDescriptor(DeviceBase* device,
                                                 const ComputePipelineDescriptor* descriptor);

    class ComputePipelineBase : public PipelineBase {
      public:
        ComputePipelineBase(DeviceBase* device, const ComputePipelineDescriptor* descriptor);
        ~ComputePipelineBase() override;

        static ComputePipelineBase* MakeError(DeviceBase* device);

        // Functors necessary for the unordered_set<ComputePipelineBase*>-based cache.
        struct HashFunc {
            size_t operator()(const ComputePipelineBase* pipeline) const;
        };
        struct EqualityFunc {
            bool operator()(const ComputePipelineBase* a, const ComputePipelineBase* b) const;
        };

      private:
        ComputePipelineBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        // TODO(cwallez@chromium.org): Store a crypto hash of the module instead.
        Ref<ShaderModuleBase> mModule;
        std::string mEntryPoint;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_COMPUTEPIPELINE_H_
