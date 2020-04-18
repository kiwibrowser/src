/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/svg/animation/smil_time_container.h"

#include <algorithm>
#include "third_party/blink/renderer/core/animation/animation_clock.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/svg/animation/smil_time.h"
#include "third_party/blink/renderer/core/svg/animation/svg_smil_element.h"
#include "third_party/blink/renderer/core/svg/svg_svg_element.h"

namespace blink {

static const double kAnimationPolicyOnceDuration = 3.000;

SMILTimeContainer::SMILTimeContainer(SVGSVGElement& owner)
    : presentation_time_(0),
      reference_time_(0),
      frame_scheduling_state_(kIdle),
      started_(false),
      paused_(false),
      document_order_indexes_dirty_(false),
      wakeup_timer_(
          owner.GetDocument().GetTaskRunner(TaskType::kInternalDefault),
          this,
          &SMILTimeContainer::WakeupTimerFired),
      animation_policy_once_timer_(
          owner.GetDocument().GetTaskRunner(TaskType::kInternalDefault),
          this,
          &SMILTimeContainer::AnimationPolicyTimerFired),
      owner_svg_element_(&owner) {}

SMILTimeContainer::~SMILTimeContainer() {
  CancelAnimationFrame();
  CancelAnimationPolicyTimer();
  DCHECK(!wakeup_timer_.IsActive());
#if DCHECK_IS_ON()
  DCHECK(!prevent_scheduled_animations_changes_);
#endif
}

void SMILTimeContainer::Schedule(SVGSMILElement* animation,
                                 SVGElement* target,
                                 const QualifiedName& attribute_name) {
  DCHECK_EQ(animation->TimeContainer(), this);
  DCHECK(target);
  DCHECK(animation->HasValidTarget());

#if DCHECK_IS_ON()
  DCHECK(!prevent_scheduled_animations_changes_);
#endif

  ElementAttributePair key(target, attribute_name);
  Member<AnimationsLinkedHashSet>& scheduled =
      scheduled_animations_.insert(key, nullptr).stored_value->value;
  if (!scheduled)
    scheduled = new AnimationsLinkedHashSet;
  DCHECK(!scheduled->Contains(animation));
  scheduled->insert(animation);

  SMILTime next_fire_time = animation->NextProgressTime();
  if (next_fire_time.IsFinite())
    NotifyIntervalsChanged();
}

void SMILTimeContainer::Unschedule(SVGSMILElement* animation,
                                   SVGElement* target,
                                   const QualifiedName& attribute_name) {
  DCHECK_EQ(animation->TimeContainer(), this);

#if DCHECK_IS_ON()
  DCHECK(!prevent_scheduled_animations_changes_);
#endif

  ElementAttributePair key(target, attribute_name);
  GroupedAnimationsMap::iterator it = scheduled_animations_.find(key);
  DCHECK_NE(it, scheduled_animations_.end());
  AnimationsLinkedHashSet* scheduled = it->value.Get();
  DCHECK(scheduled);
  AnimationsLinkedHashSet::iterator it_animation = scheduled->find(animation);
  DCHECK(it_animation != scheduled->end());
  scheduled->erase(it_animation);

  if (scheduled->IsEmpty())
    scheduled_animations_.erase(it);
}

bool SMILTimeContainer::HasAnimations() const {
  return !scheduled_animations_.IsEmpty();
}

bool SMILTimeContainer::HasPendingSynchronization() const {
  return frame_scheduling_state_ == kSynchronizeAnimations &&
         wakeup_timer_.IsActive() && !wakeup_timer_.NextFireInterval();
}

void SMILTimeContainer::NotifyIntervalsChanged() {
  if (!IsStarted())
    return;
  // Schedule updateAnimations() to be called asynchronously so multiple
  // intervals can change with updateAnimations() only called once at the end.
  if (HasPendingSynchronization())
    return;
  CancelAnimationFrame();
  ScheduleWakeUp(0, kSynchronizeAnimations);
}

double SMILTimeContainer::Elapsed() const {
  if (!IsStarted())
    return 0;

  if (IsPaused())
    return presentation_time_;

  return presentation_time_ +
         (GetDocument().Timeline().CurrentTimeInternal() - reference_time_);
}

void SMILTimeContainer::SynchronizeToDocumentTimeline() {
  reference_time_ = GetDocument().Timeline().CurrentTimeInternal();
}

bool SMILTimeContainer::IsPaused() const {
  // If animation policy is "none", the timeline is always paused.
  return paused_ || AnimationPolicy() == kImageAnimationPolicyNoAnimation;
}

bool SMILTimeContainer::IsStarted() const {
  return started_;
}

bool SMILTimeContainer::IsTimelineRunning() const {
  return IsStarted() && !IsPaused();
}

void SMILTimeContainer::Start() {
  CHECK(!IsStarted());

  if (!GetDocument().IsActive())
    return;

  if (!HandleAnimationPolicy(kRestartOnceTimerIfNotPaused))
    return;

  // Sample the document timeline to get a time reference for the "presentation
  // time".
  SynchronizeToDocumentTimeline();
  started_ = true;

  // If the "presentation time" is non-zero, the timeline was modified via
  // SetElapsed() before the document began. In this case pass on
  // seek_to_time=true to issue a seek.
  UpdateAnimationsAndScheduleFrameIfNeeded(presentation_time_,
                                           presentation_time_ ? true : false);
}

void SMILTimeContainer::Pause() {
  if (!HandleAnimationPolicy(kCancelOnceTimer))
    return;
  DCHECK(!IsPaused());

  if (IsStarted()) {
    presentation_time_ = Elapsed();
    CancelAnimationFrame();
  }
  // Update the flag after sampling elapsed().
  paused_ = true;
}

void SMILTimeContainer::Unpause() {
  if (!HandleAnimationPolicy(kRestartOnceTimer))
    return;
  DCHECK(IsPaused());

  paused_ = false;

  if (!IsStarted())
    return;

  SynchronizeToDocumentTimeline();
  ScheduleWakeUp(0, kSynchronizeAnimations);
}

void SMILTimeContainer::SetElapsed(double elapsed) {
  presentation_time_ = elapsed;

  // If the document hasn't finished loading, |m_presentationTime| will be
  // used as the start time to seek to once it's possible.
  if (!IsStarted())
    return;

  if (!HandleAnimationPolicy(kRestartOnceTimerIfNotPaused))
    return;

  CancelAnimationFrame();

  if (!IsPaused())
    SynchronizeToDocumentTimeline();

#if DCHECK_IS_ON()
  prevent_scheduled_animations_changes_ = true;
#endif
  for (const auto& entry : scheduled_animations_) {
    if (!entry.key.first)
      continue;

    AnimationsLinkedHashSet* scheduled = entry.value.Get();
    for (SVGSMILElement* element : *scheduled)
      element->Reset();
  }
#if DCHECK_IS_ON()
  prevent_scheduled_animations_changes_ = false;
#endif

  UpdateAnimationsAndScheduleFrameIfNeeded(elapsed, true);
}

void SMILTimeContainer::ScheduleAnimationFrame(double delay_time) {
  DCHECK(std::isfinite(delay_time));
  DCHECK(IsTimelineRunning());
  DCHECK(!wakeup_timer_.IsActive());

  if (delay_time < DocumentTimeline::kMinimumDelay) {
    ServiceOnNextFrame();
  } else {
    ScheduleWakeUp(delay_time - DocumentTimeline::kMinimumDelay,
                   kFutureAnimationFrame);
  }
}

void SMILTimeContainer::CancelAnimationFrame() {
  frame_scheduling_state_ = kIdle;
  wakeup_timer_.Stop();
}

void SMILTimeContainer::ScheduleWakeUp(
    double delay_time,
    FrameSchedulingState frame_scheduling_state) {
  DCHECK(frame_scheduling_state == kSynchronizeAnimations ||
         frame_scheduling_state == kFutureAnimationFrame);
  wakeup_timer_.StartOneShot(delay_time, FROM_HERE);
  frame_scheduling_state_ = frame_scheduling_state;
}

void SMILTimeContainer::WakeupTimerFired(TimerBase*) {
  DCHECK(frame_scheduling_state_ == kSynchronizeAnimations ||
         frame_scheduling_state_ == kFutureAnimationFrame);
  if (frame_scheduling_state_ == kFutureAnimationFrame) {
    DCHECK(IsTimelineRunning());
    frame_scheduling_state_ = kIdle;
    ServiceOnNextFrame();
  } else {
    frame_scheduling_state_ = kIdle;
    UpdateAnimationsAndScheduleFrameIfNeeded(Elapsed());
  }
}

void SMILTimeContainer::ScheduleAnimationPolicyTimer() {
  animation_policy_once_timer_.StartOneShot(kAnimationPolicyOnceDuration,
                                            FROM_HERE);
}

void SMILTimeContainer::CancelAnimationPolicyTimer() {
  animation_policy_once_timer_.Stop();
}

void SMILTimeContainer::AnimationPolicyTimerFired(TimerBase*) {
  Pause();
}

ImageAnimationPolicy SMILTimeContainer::AnimationPolicy() const {
  Settings* settings = GetDocument().GetSettings();
  if (!settings)
    return kImageAnimationPolicyAllowed;

  return settings->GetImageAnimationPolicy();
}

bool SMILTimeContainer::HandleAnimationPolicy(
    AnimationPolicyOnceAction once_action) {
  ImageAnimationPolicy policy = AnimationPolicy();
  // If the animation policy is "none", control is not allowed.
  // returns false to exit flow.
  if (policy == kImageAnimationPolicyNoAnimation)
    return false;
  // If the animation policy is "once",
  if (policy == kImageAnimationPolicyAnimateOnce) {
    switch (once_action) {
      case kRestartOnceTimerIfNotPaused:
        if (IsPaused())
          break;
        FALLTHROUGH;
      case kRestartOnceTimer:
        ScheduleAnimationPolicyTimer();
        break;
      case kCancelOnceTimer:
        CancelAnimationPolicyTimer();
        break;
    }
  }
  if (policy == kImageAnimationPolicyAllowed) {
    // When the SVG owner element becomes detached from its document,
    // the policy defaults to ImageAnimationPolicyAllowed; there's
    // no way back. If the policy had been "once" prior to that,
    // ensure cancellation of its timer.
    if (once_action == kCancelOnceTimer)
      CancelAnimationPolicyTimer();
  }
  return true;
}

void SMILTimeContainer::UpdateDocumentOrderIndexes() {
  unsigned timing_element_count = 0;
  for (SVGSMILElement& element :
       Traversal<SVGSMILElement>::DescendantsOf(OwnerSVGElement()))
    element.SetDocumentOrderIndex(timing_element_count++);
  document_order_indexes_dirty_ = false;
}

struct PriorityCompare {
  PriorityCompare(double elapsed) : elapsed_(elapsed) {}
  bool operator()(const Member<SVGSMILElement>& a,
                  const Member<SVGSMILElement>& b) {
    // FIXME: This should also consider possible timing relations between the
    // elements.
    SMILTime a_begin = a->IntervalBegin();
    SMILTime b_begin = b->IntervalBegin();
    // Frozen elements need to be prioritized based on their previous interval.
    a_begin = a->IsFrozen() && elapsed_ < a_begin ? a->PreviousIntervalBegin()
                                                  : a_begin;
    b_begin = b->IsFrozen() && elapsed_ < b_begin ? b->PreviousIntervalBegin()
                                                  : b_begin;
    if (a_begin == b_begin)
      return a->DocumentOrderIndex() < b->DocumentOrderIndex();
    return a_begin < b_begin;
  }
  double elapsed_;
};

SVGSVGElement& SMILTimeContainer::OwnerSVGElement() const {
  return *owner_svg_element_;
}

Document& SMILTimeContainer::GetDocument() const {
  return OwnerSVGElement().GetDocument();
}

void SMILTimeContainer::ServiceOnNextFrame() {
  if (GetDocument().View()) {
    GetDocument().View()->ScheduleAnimation();
    frame_scheduling_state_ = kAnimationFrame;
  }
}

void SMILTimeContainer::ServiceAnimations() {
  if (frame_scheduling_state_ != kAnimationFrame)
    return;

  frame_scheduling_state_ = kIdle;
  UpdateAnimationsAndScheduleFrameIfNeeded(Elapsed());
}

bool SMILTimeContainer::CanScheduleFrame(SMILTime earliest_fire_time) const {
  // If there's synchronization pending (most likely due to syncbases), then
  // let that complete first before attempting to schedule a frame.
  if (HasPendingSynchronization())
    return false;
  if (!IsTimelineRunning())
    return false;
  return earliest_fire_time.IsFinite();
}

void SMILTimeContainer::UpdateAnimationsAndScheduleFrameIfNeeded(
    double elapsed,
    bool seek_to_time) {
  if (!GetDocument().IsActive())
    return;

  SMILTime earliest_fire_time = UpdateAnimations(elapsed, seek_to_time);
  if (!CanScheduleFrame(earliest_fire_time))
    return;
  double delay_time = earliest_fire_time.Value() - elapsed;
  ScheduleAnimationFrame(delay_time);
}

SMILTime SMILTimeContainer::UpdateAnimations(double elapsed,
                                             bool seek_to_time) {
  DCHECK(GetDocument().IsActive());
  SMILTime earliest_fire_time = SMILTime::Unresolved();

#if DCHECK_IS_ON()
  // This boolean will catch any attempts to schedule/unschedule
  // scheduledAnimations during this critical section.  Similarly, any elements
  // removed will unschedule themselves, so this will catch modification of
  // animationsToApply.
  prevent_scheduled_animations_changes_ = true;
#endif

  if (document_order_indexes_dirty_)
    UpdateDocumentOrderIndexes();

  HeapHashSet<ElementAttributePair> invalid_keys;
  using AnimationsVector = HeapVector<Member<SVGSMILElement>>;
  AnimationsVector animations_to_apply;
  AnimationsVector scheduled_animations_in_same_group;
  for (const auto& entry : scheduled_animations_) {
    if (!entry.key.first || entry.value->IsEmpty()) {
      invalid_keys.insert(entry.key);
      continue;
    }

    // Sort according to priority. Elements with later begin time have higher
    // priority.  In case of a tie, document order decides.
    // FIXME: This should also consider timing relationships between the
    // elements. Dependents have higher priority.
    CopyToVector(*entry.value, scheduled_animations_in_same_group);
    std::sort(scheduled_animations_in_same_group.begin(),
              scheduled_animations_in_same_group.end(),
              PriorityCompare(elapsed));

    AnimationsVector sandwich;
    for (const auto& it_animation : scheduled_animations_in_same_group) {
      SVGSMILElement* animation = it_animation.Get();
      DCHECK_EQ(animation->TimeContainer(), this);
      DCHECK(animation->HasValidTarget());

      // This will calculate the contribution from the animation and update
      // timing.
      if (animation->Progress(elapsed, seek_to_time)) {
        sandwich.push_back(animation);
      } else {
        animation->ClearAnimatedType();
      }

      SMILTime next_fire_time = animation->NextProgressTime();
      if (next_fire_time.IsFinite())
        earliest_fire_time = std::min(next_fire_time, earliest_fire_time);
    }

    if (!sandwich.IsEmpty()) {
      // Results are accumulated to the first animation that animates and
      // contributes to a particular element/attribute pair.
      // Only reset the animated type to the base value once for
      // the lowest priority animation that animates and
      // contributes to a particular element/attribute pair.
      SVGSMILElement* result_element = sandwich.front();
      result_element->ResetAnimatedType();

      // Go through the sandwich from lowest prio to highest and generate
      // the animated value (if any.)
      for (const auto& animation : sandwich)
        animation->UpdateAnimatedValue(result_element);

      animations_to_apply.push_back(result_element);
    }
  }
  scheduled_animations_.RemoveAll(invalid_keys);

  if (animations_to_apply.IsEmpty()) {
#if DCHECK_IS_ON()
    prevent_scheduled_animations_changes_ = false;
#endif
    return earliest_fire_time;
  }

  UseCounter::Count(&GetDocument(), WebFeature::kSVGSMILAnimationAppliedEffect);

  std::sort(animations_to_apply.begin(), animations_to_apply.end(),
            PriorityCompare(elapsed));

  // Apply results to target elements.
  for (const auto& timed_element : animations_to_apply)
    timed_element->ApplyResultsToTarget();

#if DCHECK_IS_ON()
  prevent_scheduled_animations_changes_ = false;
#endif

  for (const auto& timed_element : animations_to_apply) {
    if (timed_element->isConnected() && timed_element->IsSVGDiscardElement()) {
      SVGElement* target_element = timed_element->targetElement();
      if (target_element && target_element->isConnected()) {
        target_element->remove(IGNORE_EXCEPTION_FOR_TESTING);
        DCHECK(!target_element->isConnected());
      }

      if (timed_element->isConnected()) {
        timed_element->remove(IGNORE_EXCEPTION_FOR_TESTING);
        DCHECK(!timed_element->isConnected());
      }
    }
  }
  return earliest_fire_time;
}

void SMILTimeContainer::AdvanceFrameForTesting() {
  const double kInitialFrameDelay = 0.025;
  SetElapsed(Elapsed() + kInitialFrameDelay);
}

void SMILTimeContainer::Trace(blink::Visitor* visitor) {
  visitor->Trace(scheduled_animations_);
  visitor->Trace(owner_svg_element_);
}

}  // namespace blink
