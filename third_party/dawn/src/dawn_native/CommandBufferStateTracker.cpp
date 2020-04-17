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

#include "dawn_native/CommandBufferStateTracker.h"

#include "common/Assert.h"
#include "common/BitSetIterator.h"
#include "dawn_native/BindGroup.h"
#include "dawn_native/ComputePipeline.h"
#include "dawn_native/Forward.h"
#include "dawn_native/PipelineLayout.h"
#include "dawn_native/RenderPipeline.h"

namespace dawn_native {

    enum ValidationAspect {
        VALIDATION_ASPECT_PIPELINE,
        VALIDATION_ASPECT_BIND_GROUPS,
        VALIDATION_ASPECT_VERTEX_BUFFERS,
        VALIDATION_ASPECT_INDEX_BUFFER,

        VALIDATION_ASPECT_COUNT
    };
    static_assert(VALIDATION_ASPECT_COUNT == CommandBufferStateTracker::kNumAspects, "");

    static constexpr CommandBufferStateTracker::ValidationAspects kDispatchAspects =
        1 << VALIDATION_ASPECT_PIPELINE | 1 << VALIDATION_ASPECT_BIND_GROUPS;

    static constexpr CommandBufferStateTracker::ValidationAspects kDrawAspects =
        1 << VALIDATION_ASPECT_PIPELINE | 1 << VALIDATION_ASPECT_BIND_GROUPS |
        1 << VALIDATION_ASPECT_VERTEX_BUFFERS;

    static constexpr CommandBufferStateTracker::ValidationAspects kDrawIndexedAspects =
        1 << VALIDATION_ASPECT_PIPELINE | 1 << VALIDATION_ASPECT_BIND_GROUPS |
        1 << VALIDATION_ASPECT_VERTEX_BUFFERS | 1 << VALIDATION_ASPECT_INDEX_BUFFER;

    static constexpr CommandBufferStateTracker::ValidationAspects kLazyAspects =
        1 << VALIDATION_ASPECT_BIND_GROUPS | 1 << VALIDATION_ASPECT_VERTEX_BUFFERS;

    MaybeError CommandBufferStateTracker::ValidateCanDispatch() {
        return ValidateOperation(kDispatchAspects);
    }

    MaybeError CommandBufferStateTracker::ValidateCanDraw() {
        return ValidateOperation(kDrawAspects);
    }

    MaybeError CommandBufferStateTracker::ValidateCanDrawIndexed() {
        return ValidateOperation(kDrawIndexedAspects);
    }

    MaybeError CommandBufferStateTracker::ValidateOperation(ValidationAspects requiredAspects) {
        // Fast return-true path if everything is good
        ValidationAspects missingAspects = requiredAspects & ~mAspects;
        if (missingAspects.none()) {
            return {};
        }

        // Generate an error immediately if a non-lazy aspect is missing as computing lazy aspects
        // requires the pipeline to be set.
        if ((missingAspects & ~kLazyAspects).any()) {
            return GenerateAspectError(missingAspects);
        }

        RecomputeLazyAspects(missingAspects);

        missingAspects = requiredAspects & ~mAspects;
        if (missingAspects.any()) {
            return GenerateAspectError(missingAspects);
        }

        return {};
    }

    void CommandBufferStateTracker::RecomputeLazyAspects(ValidationAspects aspects) {
        ASSERT(mAspects[VALIDATION_ASPECT_PIPELINE]);
        ASSERT((aspects & ~kLazyAspects).none());

        if (aspects[VALIDATION_ASPECT_BIND_GROUPS]) {
            bool matches = true;

            for (uint32_t i : IterateBitSet(mLastPipelineLayout->GetBindGroupLayoutsMask())) {
                if (mBindgroups[i] == nullptr ||
                    mLastPipelineLayout->GetBindGroupLayout(i) != mBindgroups[i]->GetLayout()) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                mAspects.set(VALIDATION_ASPECT_BIND_GROUPS);
            }
        }

        if (aspects[VALIDATION_ASPECT_VERTEX_BUFFERS]) {
            ASSERT(mLastRenderPipeline != nullptr);

            auto requiredInputs = mLastRenderPipeline->GetInputsSetMask();
            if ((mInputsSet & requiredInputs) == requiredInputs) {
                mAspects.set(VALIDATION_ASPECT_VERTEX_BUFFERS);
            }
        }
    }

    MaybeError CommandBufferStateTracker::GenerateAspectError(ValidationAspects aspects) {
        ASSERT(aspects.any());

        if (aspects[VALIDATION_ASPECT_INDEX_BUFFER]) {
            return DAWN_VALIDATION_ERROR("Missing index buffer");
        }

        if (aspects[VALIDATION_ASPECT_VERTEX_BUFFERS]) {
            return DAWN_VALIDATION_ERROR("Missing vertex buffer");
        }

        if (aspects[VALIDATION_ASPECT_BIND_GROUPS]) {
            return DAWN_VALIDATION_ERROR("Missing bind group");
        }

        if (aspects[VALIDATION_ASPECT_PIPELINE]) {
            return DAWN_VALIDATION_ERROR("Missing pipeline");
        }

        UNREACHABLE();
    }

    void CommandBufferStateTracker::SetComputePipeline(ComputePipelineBase* pipeline) {
        SetPipelineCommon(pipeline);
    }

    void CommandBufferStateTracker::SetRenderPipeline(RenderPipelineBase* pipeline) {
        mLastRenderPipeline = pipeline;
        SetPipelineCommon(pipeline);
    }

    void CommandBufferStateTracker::SetBindGroup(uint32_t index, BindGroupBase* bindgroup) {
        mBindgroups[index] = bindgroup;
    }

    void CommandBufferStateTracker::SetIndexBuffer() {
        mAspects.set(VALIDATION_ASPECT_INDEX_BUFFER);
    }

    void CommandBufferStateTracker::SetVertexBuffer(uint32_t start, uint32_t count) {
        for (uint32_t i = 0; i < count; ++i) {
            mInputsSet.set(start + i);
        }
    }

    void CommandBufferStateTracker::SetPipelineCommon(PipelineBase* pipeline) {
        mLastPipelineLayout = pipeline->GetLayout();

        mAspects.set(VALIDATION_ASPECT_PIPELINE);

        // Reset lazy aspects so they get recomputed on the next operation.
        mAspects &= ~kLazyAspects;
    }

}  // namespace dawn_native
