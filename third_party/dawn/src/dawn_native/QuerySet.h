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

#ifndef DAWNNATIVE_QUERYSET_H_
#define DAWNNATIVE_QUERYSET_H_

#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"

#include "dawn_native/dawn_platform.h"

namespace dawn_native {

    MaybeError ValidateQuerySetDescriptor(DeviceBase* device, const QuerySetDescriptor* descriptor);

    class QuerySetBase : public ObjectBase {
      public:
        QuerySetBase(DeviceBase* device, const QuerySetDescriptor* descriptor);

        static QuerySetBase* MakeError(DeviceBase* device);

        wgpu::QueryType GetQueryType() const;
        uint32_t GetQueryCount() const;
        const std::vector<wgpu::PipelineStatisticName>& GetPipelineStatistics() const;

        MaybeError ValidateCanUseInSubmitNow() const;

        void Destroy();

      protected:
        QuerySetBase(DeviceBase* device, ObjectBase::ErrorTag tag);
        ~QuerySetBase() override;

        void DestroyInternal();

      private:
        virtual void DestroyImpl() = 0;

        MaybeError ValidateDestroy() const;

        wgpu::QueryType mQueryType;
        uint32_t mQueryCount;
        std::vector<wgpu::PipelineStatisticName> mPipelineStatistics;

        enum class QuerySetState { Unavailable, Available, Destroyed };
        QuerySetState mState = QuerySetState::Unavailable;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_QUERYSET_H_
