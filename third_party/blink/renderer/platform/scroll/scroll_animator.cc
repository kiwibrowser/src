/*
 * Copyright (c) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/scroll/scroll_animator.h"

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/layers/layer.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/animation/compositor_keyframe_model.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/scroll/main_thread_scrolling_reason.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

cc::Layer* ToCcLayer(GraphicsLayer* layer) {
  return layer ? layer->CcLayer() : nullptr;
}

}  // namespace

ScrollAnimatorBase* ScrollAnimatorBase::Create(
    ScrollableArea* scrollable_area) {
  if (scrollable_area && scrollable_area->ScrollAnimatorEnabled())
    return new ScrollAnimator(scrollable_area);
  return new ScrollAnimatorBase(scrollable_area);
}

ScrollAnimator::ScrollAnimator(ScrollableArea* scrollable_area,
                               WTF::TimeFunction time_function)
    : ScrollAnimatorBase(scrollable_area),
      time_function_(time_function),
      last_granularity_(kScrollByPixel) {}

ScrollAnimator::~ScrollAnimator() = default;

ScrollOffset ScrollAnimator::DesiredTargetOffset() const {
  if (run_state_ == RunState::kWaitingToCancelOnCompositor)
    return CurrentOffset();
  return (animation_curve_ ||
          run_state_ == RunState::kWaitingToSendToCompositor)
             ? target_offset_
             : CurrentOffset();
}

bool ScrollAnimator::HasRunningAnimation() const {
  return run_state_ != RunState::kPostAnimationCleanup &&
         (animation_curve_ ||
          run_state_ == RunState::kWaitingToSendToCompositor);
}

ScrollOffset ScrollAnimator::ComputeDeltaToConsume(
    const ScrollOffset& delta) const {
  ScrollOffset pos = DesiredTargetOffset();
  ScrollOffset new_pos = scrollable_area_->ClampScrollOffset(pos + delta);
  return new_pos - pos;
}

void ScrollAnimator::ResetAnimationState() {
  ScrollAnimatorCompositorCoordinator::ResetAnimationState();
  if (animation_curve_)
    animation_curve_.reset();
  start_time_ = 0.0;
}

ScrollResult ScrollAnimator::UserScroll(ScrollGranularity granularity,
                                        const ScrollOffset& delta) {
  if (!scrollable_area_->ScrollAnimatorEnabled())
    return ScrollAnimatorBase::UserScroll(granularity, delta);

  TRACE_EVENT0("blink", "ScrollAnimator::scroll");

  if (granularity == kScrollByPrecisePixel) {
    // Cancel scroll animation because asked to instant scroll.
    if (HasRunningAnimation())
      CancelAnimation();
    return ScrollAnimatorBase::UserScroll(granularity, delta);
  }

  bool needs_post_animation_cleanup =
      run_state_ == RunState::kPostAnimationCleanup;
  if (run_state_ == RunState::kPostAnimationCleanup)
    ResetAnimationState();

  ScrollOffset consumed_delta = ComputeDeltaToConsume(delta);
  ScrollOffset target_offset = DesiredTargetOffset();
  target_offset += consumed_delta;

  if (WillAnimateToOffset(target_offset)) {
    last_granularity_ = granularity;
    // Report unused delta only if there is no animation running. See
    // comment below regarding scroll latching.
    // TODO(bokan): Need to standardize how ScrollAnimators report
    // unusedDelta. This differs from ScrollAnimatorMac currently.
    return ScrollResult(true, true, 0, 0);
  }

  // If the run state when this method was called was PostAnimationCleanup and
  // we're not starting an animation, stay in PostAnimationCleanup state so
  // that the main thread scrolling reason can be removed.
  if (needs_post_animation_cleanup)
    run_state_ = RunState::kPostAnimationCleanup;

  // Report unused delta only if there is no animation and we are not
  // starting one. This ensures we latch for the duration of the
  // animation rather than animating multiple scrollers at the same time.
  return ScrollResult(false, false, delta.Width(), delta.Height());
}

bool ScrollAnimator::WillAnimateToOffset(const ScrollOffset& target_offset) {
  if (run_state_ == RunState::kPostAnimationCleanup)
    ResetAnimationState();

  if (run_state_ == RunState::kWaitingToCancelOnCompositor ||
      run_state_ == RunState::kWaitingToCancelOnCompositorButNewScroll) {
    DCHECK(animation_curve_);
    target_offset_ = target_offset;
    if (RegisterAndScheduleAnimation())
      run_state_ = RunState::kWaitingToCancelOnCompositorButNewScroll;
    return true;
  }

  if (animation_curve_) {
    if ((target_offset - target_offset_).IsZero())
      return true;

    target_offset_ = target_offset;
    DCHECK(run_state_ == RunState::kRunningOnMainThread ||
           run_state_ == RunState::kRunningOnCompositor ||
           run_state_ == RunState::kRunningOnCompositorButNeedsUpdate ||
           run_state_ == RunState::kRunningOnCompositorButNeedsTakeover ||
           run_state_ == RunState::kRunningOnCompositorButNeedsAdjustment);

    // Running on the main thread, simply update the target offset instead
    // of sending to the compositor.
    if (run_state_ == RunState::kRunningOnMainThread) {
      animation_curve_->UpdateTarget(
          time_function_() - start_time_,
          CompositorOffsetFromBlinkOffset(target_offset));
      return true;
    }

    if (RegisterAndScheduleAnimation())
      run_state_ = RunState::kRunningOnCompositorButNeedsUpdate;
    return true;
  }

  if ((target_offset - CurrentOffset()).IsZero())
    return false;

  target_offset_ = target_offset;
  start_time_ = time_function_();

  if (RegisterAndScheduleAnimation())
    run_state_ = RunState::kWaitingToSendToCompositor;

  return true;
}

void ScrollAnimator::AdjustAnimationAndSetScrollOffset(
    const ScrollOffset& offset,
    ScrollType scroll_type) {
  IntSize adjustment = RoundedIntSize(offset) -
                       RoundedIntSize(scrollable_area_->GetScrollOffset());
  ScrollOffsetChanged(offset, scroll_type);

  if (run_state_ == RunState::kIdle) {
    AdjustImplOnlyScrollOffsetAnimation(adjustment);
  } else if (HasRunningAnimation()) {
    target_offset_ += ScrollOffset(adjustment);
    if (animation_curve_) {
      animation_curve_->ApplyAdjustment(adjustment);
      if (run_state_ != RunState::kRunningOnMainThread &&
          RegisterAndScheduleAnimation())
        run_state_ = RunState::kRunningOnCompositorButNeedsAdjustment;
    }
  }
}

void ScrollAnimator::ScrollToOffsetWithoutAnimation(
    const ScrollOffset& offset) {
  current_offset_ = offset;

  ResetAnimationState();
  NotifyOffsetChanged();
}

void ScrollAnimator::TickAnimation(double monotonic_time) {
  if (run_state_ != RunState::kRunningOnMainThread)
    return;

  TRACE_EVENT0("blink", "ScrollAnimator::tickAnimation");
  double elapsed_time = monotonic_time - start_time_;

  bool is_finished = (elapsed_time > animation_curve_->Duration());
  ScrollOffset offset = BlinkOffsetFromCompositorOffset(
      is_finished ? animation_curve_->TargetValue()
                  : animation_curve_->GetValue(elapsed_time));

  offset = scrollable_area_->ClampScrollOffset(offset);

  current_offset_ = offset;

  if (is_finished)
    run_state_ = RunState::kPostAnimationCleanup;
  else
    GetScrollableArea()->ScheduleAnimation();

  TRACE_EVENT0("blink", "ScrollAnimator::notifyOffsetChanged");
  NotifyOffsetChanged();
}

void ScrollAnimator::PostAnimationCleanupAndReset() {
  // Remove the temporary main thread scrolling reason that was added while
  // main thread had scheduled an animation.
  RemoveMainThreadScrollingReason();

  ResetAnimationState();
}

bool ScrollAnimator::SendAnimationToCompositor() {
  if (scrollable_area_->ShouldScrollOnMainThread())
    return false;

  std::unique_ptr<CompositorKeyframeModel> animation =
      CompositorKeyframeModel::Create(
          *animation_curve_, CompositorTargetProperty::SCROLL_OFFSET, 0, 0);
  // Being here means that either there is an animation that needs
  // to be sent to the compositor, or an animation that needs to
  // be updated (a new scroll event before the previous animation
  // is finished). In either case, the start time is when the
  // first animation was initiated. This re-targets the animation
  // using the current time on main thread.
  animation->SetStartTime(start_time_);

  int animation_id = animation->Id();
  int animation_group_id = animation->Group();

  bool sent_to_compositor = AddAnimation(std::move(animation));
  if (sent_to_compositor) {
    run_state_ = RunState::kRunningOnCompositor;
    compositor_animation_id_ = animation_id;
    compositor_animation_group_id_ = animation_group_id;
  }

  return sent_to_compositor;
}

void ScrollAnimator::CreateAnimationCurve() {
  DCHECK(!animation_curve_);
  animation_curve_ = CompositorScrollOffsetAnimationCurve::Create(
      CompositorOffsetFromBlinkOffset(target_offset_),
      last_granularity_ == kScrollByPixel
          ? CompositorScrollOffsetAnimationCurve::kScrollDurationInverseDelta
          : CompositorScrollOffsetAnimationCurve::kScrollDurationConstant);
  animation_curve_->SetInitialValue(
      CompositorOffsetFromBlinkOffset(CurrentOffset()));
}

void ScrollAnimator::UpdateCompositorAnimations() {
  ScrollAnimatorCompositorCoordinator::UpdateCompositorAnimations();

  if (run_state_ == RunState::kPostAnimationCleanup) {
    PostAnimationCleanupAndReset();
    return;
  }

  if (run_state_ == RunState::kWaitingToCancelOnCompositor) {
    DCHECK(compositor_animation_id_);
    AbortAnimation();
    PostAnimationCleanupAndReset();
    return;
  }

  if (run_state_ == RunState::kRunningOnCompositorButNeedsTakeover) {
    // The call to ::takeOverCompositorAnimation aborted the animation and
    // put us in this state. The assumption is that takeOver is called
    // because a main thread scrolling reason is added, and simply trying
    // to ::sendAnimationToCompositor will fail and we will run on the main
    // thread.
    ResetAnimationIds();
    run_state_ = RunState::kWaitingToSendToCompositor;
  }

  if (run_state_ == RunState::kRunningOnCompositorButNeedsUpdate ||
      run_state_ == RunState::kWaitingToCancelOnCompositorButNewScroll ||
      run_state_ == RunState::kRunningOnCompositorButNeedsAdjustment) {
    // Abort the running animation before a new one with an updated
    // target is added.
    AbortAnimation();
    ResetAnimationIds();

    if (run_state_ != RunState::kRunningOnCompositorButNeedsAdjustment) {
      // When in RunningOnCompositorButNeedsAdjustment, the call to
      // ::adjustScrollOffsetAnimation should have made the necessary
      // adjustment to the curve.
      animation_curve_->UpdateTarget(
          time_function_() - start_time_,
          CompositorOffsetFromBlinkOffset(target_offset_));
    }

    if (run_state_ == RunState::kWaitingToCancelOnCompositorButNewScroll) {
      animation_curve_->SetInitialValue(
          CompositorOffsetFromBlinkOffset(CurrentOffset()));
    }

    run_state_ = RunState::kWaitingToSendToCompositor;
  }

  if (run_state_ == RunState::kWaitingToSendToCompositor) {
    if (!element_id_)
      ReattachCompositorAnimationIfNeeded(
          GetScrollableArea()->GetCompositorAnimationTimeline());

    if (!animation_curve_)
      CreateAnimationCurve();

    bool running_on_main_thread = false;
    bool sent_to_compositor = SendAnimationToCompositor();
    if (!sent_to_compositor) {
      running_on_main_thread = RegisterAndScheduleAnimation();
      if (running_on_main_thread)
        run_state_ = RunState::kRunningOnMainThread;
    }

    // Main thread should deal with the scroll animations it started.
    if (sent_to_compositor || running_on_main_thread)
      AddMainThreadScrollingReason();
    else
      RemoveMainThreadScrollingReason();
  }
}

void ScrollAnimator::AddMainThreadScrollingReason() {
  // Usually main thread scrolling reasons should be updated from
  // one frame to all its descendants. khandlingScrollFromMainThread
  // is a special case because its subframes cannot be scrolled
  // when the reason is set. When the subframes are ready to scroll
  // the reason has benn reset.
  if (cc::Layer* scroll_layer =
          ToCcLayer(GetScrollableArea()->LayerForScrolling())) {
    scroll_layer->AddMainThreadScrollingReasons(
        MainThreadScrollingReason::kHandlingScrollFromMainThread);
  }
}

void ScrollAnimator::RemoveMainThreadScrollingReason() {
  if (cc::Layer* scroll_layer =
          ToCcLayer(GetScrollableArea()->LayerForScrolling())) {
    scroll_layer->ClearMainThreadScrollingReasons(
        MainThreadScrollingReason::kHandlingScrollFromMainThread);
  }
}

void ScrollAnimator::NotifyCompositorAnimationAborted(int group_id) {
  // An animation aborted by the compositor is treated as a finished
  // animation.
  ScrollAnimatorCompositorCoordinator::CompositorAnimationFinished(group_id);
}

void ScrollAnimator::NotifyCompositorAnimationFinished(int group_id) {
  ScrollAnimatorCompositorCoordinator::CompositorAnimationFinished(group_id);
}

void ScrollAnimator::NotifyAnimationTakeover(
    double monotonic_time,
    double animation_start_time,
    std::unique_ptr<cc::AnimationCurve> curve) {
  // If there is already an animation running and the compositor asks to take
  // over an animation, do nothing to avoid judder.
  if (HasRunningAnimation())
    return;

  cc::ScrollOffsetAnimationCurve* scroll_offset_animation_curve =
      curve->ToScrollOffsetAnimationCurve();
  ScrollOffset target_value(scroll_offset_animation_curve->target_value().x(),
                            scroll_offset_animation_curve->target_value().y());
  if (WillAnimateToOffset(target_value)) {
    animation_curve_ = CompositorScrollOffsetAnimationCurve::Create(
        scroll_offset_animation_curve);
    start_time_ = animation_start_time;
  }
}

void ScrollAnimator::CancelAnimation() {
  ScrollAnimatorCompositorCoordinator::CancelAnimation();
}

void ScrollAnimator::TakeOverCompositorAnimation() {
  if (run_state_ == RunState::kRunningOnCompositor ||
      run_state_ == RunState::kRunningOnCompositorButNeedsUpdate)
    RemoveMainThreadScrollingReason();

  ScrollAnimatorCompositorCoordinator::TakeOverCompositorAnimation();
}

void ScrollAnimator::LayerForCompositedScrollingDidChange(
    CompositorAnimationTimeline* timeline) {
  if (ReattachCompositorAnimationIfNeeded(timeline) && animation_curve_)
    AddMainThreadScrollingReason();
}

bool ScrollAnimator::RegisterAndScheduleAnimation() {
  GetScrollableArea()->RegisterForAnimation();
  if (!scrollable_area_->ScheduleAnimation()) {
    ScrollToOffsetWithoutAnimation(target_offset_);
    ResetAnimationState();
    return false;
  }
  return true;
}

void ScrollAnimator::Trace(blink::Visitor* visitor) {
  ScrollAnimatorBase::Trace(visitor);
}

}  // namespace blink
