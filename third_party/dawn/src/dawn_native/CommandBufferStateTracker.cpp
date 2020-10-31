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

    namespace {
        bool BufferSizesAtLeastAsBig(const ityp::span<uint32_t, uint64_t> unverifiedBufferSizes,
                                     const std::vector<uint64_t>& pipelineMinimumBufferSizes) {
            ASSERT(unverifiedBufferSizes.size() == pipelineMinimumBufferSizes.size());

            for (uint32_t i = 0; i < unverifiedBufferSizes.size(); ++i) {
                if (unverifiedBufferSizes[i] < pipelineMinimumBufferSizes[i]) {
                    return false;
                }
            }

            return true;
        }
    }  // namespace

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
        DAWN_TRY(CheckMissingAspects(missingAspects & ~kLazyAspects));

        RecomputeLazyAspects(missingAspects);

        DAWN_TRY(CheckMissingAspects(requiredAspects & ~mAspects));

        return {};
    }

    void CommandBufferStateTracker::RecomputeLazyAspects(ValidationAspects aspects) {
        ASSERT(mAspects[VALIDATION_ASPECT_PIPELINE]);
        ASSERT((aspects & ~kLazyAspects).none());

        if (aspects[VALIDATION_ASPECT_BIND_GROUPS]) {
            bool matches = true;

            for (BindGroupIndex i : IterateBitSet(mLastPipelineLayout->GetBindGroupLayoutsMask())) {
                if (mBindgroups[i] == nullptr ||
                    mLastPipelineLayout->GetBindGroupLayout(i) != mBindgroups[i]->GetLayout() ||
                    !BufferSizesAtLeastAsBig(mBindgroups[i]->GetUnverifiedBufferSizes(),
                                             (*mMinimumBufferSizes)[i])) {
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

            const std::bitset<kMaxVertexBuffers>& requiredVertexBuffers =
                mLastRenderPipeline->GetVertexBufferSlotsUsed();
            if ((mVertexBufferSlotsUsed & requiredVertexBuffers) == requiredVertexBuffers) {
                mAspects.set(VALIDATION_ASPECT_VERTEX_BUFFERS);
            }
        }
    }

    MaybeError CommandBufferStateTracker::CheckMissingAspects(ValidationAspects aspects) {
        if (!aspects.any()) {
            return {};
        }

        if (aspects[VALIDATION_ASPECT_INDEX_BUFFER]) {
            return DAWN_VALIDATION_ERROR("Missing index buffer");
        }

        if (aspects[VALIDATION_ASPECT_VERTEX_BUFFERS]) {
            return DAWN_VALIDATION_ERROR("Missing vertex buffer");
        }

        if (aspects[VALIDATION_ASPECT_BIND_GROUPS]) {
            for (BindGroupIndex i : IterateBitSet(mLastPipelineLayout->GetBindGroupLayoutsMask())) {
                if (mBindgroups[i] == nullptr) {
                    return DAWN_VALIDATION_ERROR("Missing bind group " +
                                                 std::to_string(static_cast<uint32_t>(i)));
                } else if (mLastPipelineLayout->GetBindGroupLayout(i) !=
                           mBindgroups[i]->GetLayout()) {
                    return DAWN_VALIDATION_ERROR(
                        "Pipeline and bind group layout doesn't match for bind group " +
                        std::to_string(static_cast<uint32_t>(i)));
                } else if (!BufferSizesAtLeastAsBig(mBindgroups[i]->GetUnverifiedBufferSizes(),
                                                    (*mMinimumBufferSizes)[i])) {
                    return DAWN_VALIDATION_ERROR("Binding sizes too small for bind group " +
                                                 std::to_string(static_cast<uint32_t>(i)));
                }
            }

            // The chunk of code above should be similar to the one in |RecomputeLazyAspects|.
            // It returns the first invalid state found. We shouldn't be able to reach this line
            // because to have invalid aspects one of the above conditions must have failed earlier.
            // If this is reached, make sure lazy aspects and the error checks above are consistent.
            UNREACHABLE();
            return DAWN_VALIDATION_ERROR("Bind groups invalid");
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

    void CommandBufferStateTracker::SetBindGroup(BindGroupIndex index, BindGroupBase* bindgroup) {
        mBindgroups[index] = bindgroup;
        mAspects.reset(VALIDATION_ASPECT_BIND_GROUPS);
    }

    void CommandBufferStateTracker::SetIndexBuffer() {
        mAspects.set(VALIDATION_ASPECT_INDEX_BUFFER);
    }

    void CommandBufferStateTracker::SetVertexBuffer(uint32_t slot) {
        mVertexBufferSlotsUsed.set(slot);
    }

    void CommandBufferStateTracker::SetPipelineCommon(PipelineBase* pipeline) {
        mLastPipelineLayout = pipeline->GetLayout();
        mMinimumBufferSizes = &pipeline->GetMinimumBufferSizes();

        mAspects.set(VALIDATION_ASPECT_PIPELINE);

        // Reset lazy aspects so they get recomputed on the next operation.
        mAspects &= ~kLazyAspects;
    }

}  // namespace dawn_native
