// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/animation_ticker.h"

#include "base/stl_util.h"
#include "base/time/time.h"
#include "cc/animation/animation.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_player.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/animation/transform_operations.h"
#include "cc/trees/property_animation_state.h"

namespace cc {

AnimationTicker::AnimationTicker(AnimationPlayer* animation_player)
    : animation_player_(animation_player),
      element_animations_(),
      needs_to_start_animations_(false),
      scroll_offset_animation_was_interrupted_(false),
      is_ticking_(false),
      needs_push_properties_(false) {
  DCHECK(animation_player_);
}

AnimationTicker::~AnimationTicker() {
  DCHECK(!has_bound_element_animations());
}

void AnimationTicker::SetNeedsPushProperties() {
  needs_push_properties_ = true;

  // TODO(smcgruer): We only need the below calls when needs_push_properties_
  // goes from false to true - see http://crbug.com/764405
  DCHECK(element_animations());
  element_animations()->SetNeedsPushProperties();

  animation_player_->SetNeedsPushProperties();
}

void AnimationTicker::BindElementAnimations(
    ElementAnimations* element_animations) {
  DCHECK(element_animations);
  DCHECK(!element_animations_);
  element_animations_ = element_animations;

  if (has_any_animation())
    AnimationAdded();
  SetNeedsPushProperties();
}

void AnimationTicker::UnbindElementAnimations() {
  SetNeedsPushProperties();
  element_animations_ = nullptr;
}

void AnimationTicker::AttachElement(ElementId element_id) {
  DCHECK(!element_id_);
  DCHECK(element_id);
  element_id_ = element_id;
}

void AnimationTicker::DetachElement() {
  DCHECK(element_id_);
  element_id_ = ElementId();
}

void AnimationTicker::Tick(base::TimeTicks monotonic_time) {
  DCHECK(has_bound_element_animations());
  if (!element_animations_->has_element_in_any_list())
    return;

  if (needs_to_start_animations_)
    StartAnimations(monotonic_time);

  for (auto& animation : animations_)
    TickAnimation(monotonic_time, animation.get(), element_animations_.get());

  last_tick_time_ = monotonic_time;
  element_animations_->UpdateClientAnimationState();
}

void AnimationTicker::TickAnimation(base::TimeTicks monotonic_time,
                                    Animation* animation,
                                    AnimationTarget* target) {
  if ((animation->run_state() != Animation::STARTING &&
       animation->run_state() != Animation::RUNNING &&
       animation->run_state() != Animation::PAUSED) ||
      !animation->InEffect(monotonic_time)) {
    return;
  }

  AnimationCurve* curve = animation->curve();
  base::TimeDelta trimmed =
      animation->TrimTimeToCurrentIteration(monotonic_time);

  switch (curve->Type()) {
    case AnimationCurve::TRANSFORM:
      target->NotifyClientTransformOperationsAnimated(
          curve->ToTransformAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::FLOAT:
      target->NotifyClientFloatAnimated(
          curve->ToFloatAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::FILTER:
      target->NotifyClientFilterAnimated(
          curve->ToFilterAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::COLOR:
      target->NotifyClientColorAnimated(
          curve->ToColorAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::SCROLL_OFFSET:
      target->NotifyClientScrollOffsetAnimated(
          curve->ToScrollOffsetAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::SIZE:
      target->NotifyClientSizeAnimated(
          curve->ToSizeAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
  }
}

void AnimationTicker::RemoveFromTicking() {
  is_ticking_ = false;
  // Resetting last_tick_time_ here ensures that calling ::UpdateState
  // before ::Animate doesn't start an animation.
  last_tick_time_ = base::TimeTicks();
  animation_player_->AnimationRemovedFromTicking();
}

void AnimationTicker::UpdateState(bool start_ready_animations,
                                  AnimationEvents* events) {
  DCHECK(has_bound_element_animations());
  if (!element_animations_->has_element_in_active_list())
    return;

  // Animate hasn't been called, this happens if an element has been added
  // between the Commit and Draw phases.
  if (last_tick_time_ == base::TimeTicks())
    return;

  if (start_ready_animations)
    PromoteStartedAnimations(events);

  MarkFinishedAnimations(last_tick_time_);
  MarkAnimationsForDeletion(last_tick_time_, events);
  PurgeAnimationsMarkedForDeletion(/* impl_only */ true);

  if (start_ready_animations) {
    if (needs_to_start_animations_) {
      StartAnimations(last_tick_time_);
      PromoteStartedAnimations(events);
    }
  }
}

void AnimationTicker::UpdateTickingState(UpdateTickingType type) {
  bool force = type == UpdateTickingType::FORCE;
  if (animation_player_->has_animation_host()) {
    bool was_ticking = is_ticking_;
    is_ticking_ = HasNonDeletedAnimation();

    bool has_element_in_any_list =
        element_animations_->has_element_in_any_list();

    if (is_ticking_ && ((!was_ticking && has_element_in_any_list) || force)) {
      animation_player_->AddToTicking();
    } else if (!is_ticking_ && (was_ticking || force)) {
      RemoveFromTicking();
    }
  }
}

void AnimationTicker::AddAnimation(std::unique_ptr<Animation> animation) {
  AnimationHost* animation_host = animation_player_->animation_host();
  DCHECK(animation->target_property_id() != TargetProperty::SCROLL_OFFSET ||
         (animation_host && animation_host->SupportsScrollAnimations()));
  DCHECK(!animation->is_impl_only() ||
         animation->target_property_id() == TargetProperty::SCROLL_OFFSET);

  animations_.push_back(std::move(animation));

  if (has_bound_element_animations()) {
    AnimationAdded();
    SetNeedsPushProperties();
  }
}

void AnimationTicker::PauseAnimation(int animation_id, double time_offset) {
  const base::TimeDelta time_delta = base::TimeDelta::FromSecondsD(time_offset);
  for (auto& animation : animations_) {
    if (animation->id() == animation_id) {
      animation->SetRunState(
          Animation::PAUSED,
          time_delta + animation->start_time() + animation->time_offset());
    }
  }

  if (has_bound_element_animations()) {
    animation_player_->SetNeedsCommit();
    SetNeedsPushProperties();
  }
}

void AnimationTicker::RemoveAnimation(int animation_id) {
  bool animation_removed = false;

  // Since we want to use the animations that we're going to remove, we need to
  // use a stable_parition here instead of remove_if. Remove_if leaves the
  // removed items in an unspecified state.
  auto animations_to_remove = std::stable_partition(
      animations_.begin(), animations_.end(),
      [animation_id](const std::unique_ptr<Animation>& animation) {
        return animation->id() != animation_id;
      });
  for (auto it = animations_to_remove; it != animations_.end(); ++it) {
    if ((*it)->target_property_id() == TargetProperty::SCROLL_OFFSET) {
      if (has_bound_element_animations())
        scroll_offset_animation_was_interrupted_ = true;
    } else if (!(*it)->is_finished()) {
      animation_removed = true;
    }
  }

  animations_.erase(animations_to_remove, animations_.end());

  if (has_bound_element_animations()) {
    UpdateTickingState(UpdateTickingType::NORMAL);
    if (animation_removed)
      element_animations_->UpdateClientAnimationState();
    animation_player_->SetNeedsCommit();
    SetNeedsPushProperties();
  }
}

void AnimationTicker::AbortAnimation(int animation_id) {
  if (Animation* animation = GetAnimationById(animation_id)) {
    if (!animation->is_finished()) {
      animation->SetRunState(Animation::ABORTED, last_tick_time_);
      if (has_bound_element_animations())
        element_animations_->UpdateClientAnimationState();
    }
  }

  if (has_bound_element_animations()) {
    animation_player_->SetNeedsCommit();
    SetNeedsPushProperties();
  }
}

void AnimationTicker::AbortAnimations(TargetProperty::Type target_property,
                                      bool needs_completion) {
  if (needs_completion)
    DCHECK(target_property == TargetProperty::SCROLL_OFFSET);

  bool aborted_animation = false;
  for (auto& animation : animations_) {
    if (animation->target_property_id() == target_property &&
        !animation->is_finished()) {
      // Currently only impl-only scroll offset animations can be completed on
      // the main thread.
      if (needs_completion && animation->is_impl_only()) {
        animation->SetRunState(Animation::ABORTED_BUT_NEEDS_COMPLETION,
                               last_tick_time_);
      } else {
        animation->SetRunState(Animation::ABORTED, last_tick_time_);
      }
      aborted_animation = true;
    }
  }

  if (has_bound_element_animations()) {
    if (aborted_animation)
      element_animations_->UpdateClientAnimationState();
    animation_player_->SetNeedsCommit();
    SetNeedsPushProperties();
  }
}

void AnimationTicker::ActivateAnimations() {
  DCHECK(has_bound_element_animations());

  bool animation_activated = false;
  for (auto& animation : animations_) {
    if (animation->affects_active_elements() !=
        animation->affects_pending_elements()) {
      animation_activated = true;
    }
    animation->set_affects_active_elements(
        animation->affects_pending_elements());
  }

  if (animation_activated)
    element_animations_->UpdateClientAnimationState();

  scroll_offset_animation_was_interrupted_ = false;
}

void AnimationTicker::AnimationAdded() {
  DCHECK(has_bound_element_animations());

  animation_player_->SetNeedsCommit();
  needs_to_start_animations_ = true;

  UpdateTickingState(UpdateTickingType::NORMAL);
  element_animations_->UpdateClientAnimationState();
}

bool AnimationTicker::NotifyAnimationStarted(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);
  for (auto& animation : animations_) {
    if (animation->group() == event.group_id &&
        animation->target_property_id() == event.target_property &&
        animation->needs_synchronized_start_time()) {
      animation->set_needs_synchronized_start_time(false);
      if (!animation->has_set_start_time())
        animation->set_start_time(event.monotonic_time);
      animation_player_->NotifyAnimationStarted(event);
      return true;
    }
  }
  return false;
}

bool AnimationTicker::NotifyAnimationFinished(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);
  for (auto& animation : animations_) {
    if (animation->group() == event.group_id &&
        animation->target_property_id() == event.target_property) {
      animation->set_received_finished_event(true);
      animation_player_->NotifyAnimationFinished(event);
      return true;
    }
  }

  // This is for the case when an animation is already removed on main thread,
  // but the impl version of it sent a finished event and is now waiting for
  // deletion. We would need to delete that animation during push properties.
  SetNeedsPushProperties();
  return false;
}

void AnimationTicker::NotifyAnimationTakeover(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);

  // We need to purge animations marked for deletion on CT.
  SetNeedsPushProperties();

  animation_player_->NotifyAnimationTakeover(event);
}

bool AnimationTicker::NotifyAnimationAborted(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);
  for (auto& animation : animations_) {
    if (animation->group() == event.group_id &&
        animation->target_property_id() == event.target_property) {
      animation->SetRunState(Animation::ABORTED, event.monotonic_time);
      animation->set_received_finished_event(true);
      animation_player_->NotifyAnimationAborted(event);
      return true;
    }
  }
  return false;
}

bool AnimationTicker::HasTickingAnimation() const {
  for (const auto& animation : animations_) {
    if (!animation->is_finished())
      return true;
  }
  return false;
}

bool AnimationTicker::HasNonDeletedAnimation() const {
  for (const auto& animation : animations_) {
    if (animation->run_state() != Animation::WAITING_FOR_DELETION)
      return true;
  }
  return false;
}

bool AnimationTicker::HasOnlyTranslationTransforms(
    ElementListType list_type) const {
  for (const auto& animation : animations_) {
    if (animation->is_finished() ||
        animation->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    if ((list_type == ElementListType::ACTIVE &&
         !animation->affects_active_elements()) ||
        (list_type == ElementListType::PENDING &&
         !animation->affects_pending_elements()))
      continue;

    const TransformAnimationCurve* transform_animation_curve =
        animation->curve()->ToTransformAnimationCurve();
    if (!transform_animation_curve->IsTranslation())
      return false;
  }
  return true;
}

bool AnimationTicker::AnimationsPreserveAxisAlignment() const {
  for (const auto& animation : animations_) {
    if (animation->is_finished() ||
        animation->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    const TransformAnimationCurve* transform_animation_curve =
        animation->curve()->ToTransformAnimationCurve();
    if (!transform_animation_curve->PreservesAxisAlignment())
      return false;
  }
  return true;
}

bool AnimationTicker::AnimationStartScale(ElementListType list_type,
                                          float* start_scale) const {
  *start_scale = 0.f;
  for (const auto& animation : animations_) {
    if (animation->is_finished() ||
        animation->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    if ((list_type == ElementListType::ACTIVE &&
         !animation->affects_active_elements()) ||
        (list_type == ElementListType::PENDING &&
         !animation->affects_pending_elements()))
      continue;

    bool forward_direction = true;
    switch (animation->direction()) {
      case Animation::Direction::NORMAL:
      case Animation::Direction::ALTERNATE_NORMAL:
        forward_direction = animation->playback_rate() >= 0.0;
        break;
      case Animation::Direction::REVERSE:
      case Animation::Direction::ALTERNATE_REVERSE:
        forward_direction = animation->playback_rate() < 0.0;
        break;
    }

    const TransformAnimationCurve* transform_animation_curve =
        animation->curve()->ToTransformAnimationCurve();
    float animation_start_scale = 0.f;
    if (!transform_animation_curve->AnimationStartScale(forward_direction,
                                                        &animation_start_scale))
      return false;
    *start_scale = std::max(*start_scale, animation_start_scale);
  }
  return true;
}

bool AnimationTicker::MaximumTargetScale(ElementListType list_type,
                                         float* max_scale) const {
  *max_scale = 0.f;
  for (const auto& animation : animations_) {
    if (animation->is_finished() ||
        animation->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    if ((list_type == ElementListType::ACTIVE &&
         !animation->affects_active_elements()) ||
        (list_type == ElementListType::PENDING &&
         !animation->affects_pending_elements()))
      continue;

    bool forward_direction = true;
    switch (animation->direction()) {
      case Animation::Direction::NORMAL:
      case Animation::Direction::ALTERNATE_NORMAL:
        forward_direction = animation->playback_rate() >= 0.0;
        break;
      case Animation::Direction::REVERSE:
      case Animation::Direction::ALTERNATE_REVERSE:
        forward_direction = animation->playback_rate() < 0.0;
        break;
    }

    const TransformAnimationCurve* transform_animation_curve =
        animation->curve()->ToTransformAnimationCurve();
    float animation_scale = 0.f;
    if (!transform_animation_curve->MaximumTargetScale(forward_direction,
                                                       &animation_scale))
      return false;
    *max_scale = std::max(*max_scale, animation_scale);
  }

  return true;
}

bool AnimationTicker::IsPotentiallyAnimatingProperty(
    TargetProperty::Type target_property,
    ElementListType list_type) const {
  for (const auto& animation : animations_) {
    if (!animation->is_finished() &&
        animation->target_property_id() == target_property) {
      if ((list_type == ElementListType::ACTIVE &&
           animation->affects_active_elements()) ||
          (list_type == ElementListType::PENDING &&
           animation->affects_pending_elements()))
        return true;
    }
  }
  return false;
}

bool AnimationTicker::IsCurrentlyAnimatingProperty(
    TargetProperty::Type target_property,
    ElementListType list_type) const {
  for (const auto& animation : animations_) {
    if (!animation->is_finished() && animation->InEffect(last_tick_time_) &&
        animation->target_property_id() == target_property) {
      if ((list_type == ElementListType::ACTIVE &&
           animation->affects_active_elements()) ||
          (list_type == ElementListType::PENDING &&
           animation->affects_pending_elements()))
        return true;
    }
  }
  return false;
}

Animation* AnimationTicker::GetAnimation(
    TargetProperty::Type target_property) const {
  for (size_t i = 0; i < animations_.size(); ++i) {
    size_t index = animations_.size() - i - 1;
    if (animations_[index]->target_property_id() == target_property)
      return animations_[index].get();
  }
  return nullptr;
}

Animation* AnimationTicker::GetAnimationById(int animation_id) const {
  for (auto& animation : animations_)
    if (animation->id() == animation_id)
      return animation.get();
  return nullptr;
}

void AnimationTicker::GetPropertyAnimationState(
    PropertyAnimationState* pending_state,
    PropertyAnimationState* active_state) const {
  pending_state->Clear();
  active_state->Clear();

  for (const auto& animation : animations_) {
    if (!animation->is_finished()) {
      bool in_effect = animation->InEffect(last_tick_time_);
      bool active = animation->affects_active_elements();
      bool pending = animation->affects_pending_elements();
      int property = animation->target_property_id();

      if (pending)
        pending_state->potentially_animating[property] = true;
      if (pending && in_effect)
        pending_state->currently_running[property] = true;

      if (active)
        active_state->potentially_animating[property] = true;
      if (active && in_effect)
        active_state->currently_running[property] = true;
    }
  }
}

void AnimationTicker::MarkAbortedAnimationsForDeletion(
    AnimationTicker* animation_ticker_impl) {
  bool animation_aborted = false;

  auto& animations_impl = animation_ticker_impl->animations_;
  for (const auto& animation_impl : animations_impl) {
    // If the animation has been aborted on the main thread, mark it for
    // deletion.
    if (Animation* animation = GetAnimationById(animation_impl->id())) {
      if (animation->run_state() == Animation::ABORTED) {
        animation_impl->SetRunState(Animation::WAITING_FOR_DELETION,
                                    animation_ticker_impl->last_tick_time_);
        animation->SetRunState(Animation::WAITING_FOR_DELETION,
                               last_tick_time_);
        animation_aborted = true;
      }
    }
  }

  if (has_bound_element_animations() && animation_aborted)
    element_animations_->SetNeedsPushProperties();
}

void AnimationTicker::PurgeAnimationsMarkedForDeletion(bool impl_only) {
  base::EraseIf(
      animations_, [impl_only](const std::unique_ptr<Animation>& animation) {
        return animation->run_state() == Animation::WAITING_FOR_DELETION &&
               (!impl_only || animation->is_impl_only());
      });
}

void AnimationTicker::PushNewAnimationsToImplThread(
    AnimationTicker* animation_ticker_impl) const {
  // Any new animations owned by the main thread's AnimationPlayer are cloned
  // and added to the impl thread's AnimationPlayer.
  for (const auto& animation : animations_) {
    // If the animation is already running on the impl thread, there is no
    // need to copy it over.
    if (animation_ticker_impl->GetAnimationById(animation->id()))
      continue;

    if (animation->target_property_id() == TargetProperty::SCROLL_OFFSET &&
        !animation->curve()
             ->ToScrollOffsetAnimationCurve()
             ->HasSetInitialValue()) {
      gfx::ScrollOffset current_scroll_offset;
      if (animation_ticker_impl->HasElementInActiveList()) {
        current_scroll_offset =
            animation_ticker_impl->ScrollOffsetForAnimation();
      } else {
        // The owning layer isn't yet in the active tree, so the main thread
        // scroll offset will be up to date.
        current_scroll_offset = ScrollOffsetForAnimation();
      }
      animation->curve()->ToScrollOffsetAnimationCurve()->SetInitialValue(
          current_scroll_offset);
    }

    // The new animation should be set to run as soon as possible.
    Animation::RunState initial_run_state =
        Animation::WAITING_FOR_TARGET_AVAILABILITY;
    std::unique_ptr<Animation> to_add(
        animation->CloneAndInitialize(initial_run_state));
    DCHECK(!to_add->needs_synchronized_start_time());
    to_add->set_affects_active_elements(false);
    animation_ticker_impl->AddAnimation(std::move(to_add));
  }
}

namespace {
bool IsCompleted(Animation* animation,
                 const AnimationTicker* main_thread_ticker) {
  if (animation->is_impl_only()) {
    return (animation->run_state() == Animation::WAITING_FOR_DELETION);
  } else {
    Animation* main_thread_animation =
        main_thread_ticker->GetAnimationById(animation->id());
    return !main_thread_animation || main_thread_animation->is_finished();
  }
}
}  // namespace

void AnimationTicker::RemoveAnimationsCompletedOnMainThread(
    AnimationTicker* animation_ticker_impl) const {
  bool animation_completed = false;

  // Animations removed on the main thread should no longer affect pending
  // elements, and should stop affecting active elements after the next call
  // to ActivateAnimations. If already WAITING_FOR_DELETION, they can be removed
  // immediately.
  auto& animations = animation_ticker_impl->animations_;
  for (const auto& animation : animations) {
    if (IsCompleted(animation.get(), this)) {
      animation->set_affects_pending_elements(false);
      animation_completed = true;
    }
  }
  auto affects_active_only_and_is_waiting_for_deletion =
      [](const std::unique_ptr<Animation>& animation) {
        return animation->run_state() == Animation::WAITING_FOR_DELETION &&
               !animation->affects_pending_elements();
      };
  base::EraseIf(animations, affects_active_only_and_is_waiting_for_deletion);

  if (has_bound_element_animations() && animation_completed)
    element_animations_->SetNeedsPushProperties();
}

void AnimationTicker::PushPropertiesTo(AnimationTicker* animation_ticker_impl) {
  if (!needs_push_properties_)
    return;
  needs_push_properties_ = false;

  // Synchronize the animation target between main and impl size.
  if (element_id_ != animation_ticker_impl->element_id_) {
    // We have to detach/attach via the AnimationPlayer as it may need to inform
    // the host as well.
    if (animation_ticker_impl->has_attached_element())
      animation_ticker_impl->animation_player_->DetachElement();
    if (element_id_)
      animation_ticker_impl->animation_player_->AttachElement(element_id_);
  }

  // If neither main nor impl have any animations, there is nothing further to
  // synchronize.
  if (!has_any_animation() && !animation_ticker_impl->has_any_animation())
    return;

  // Synchronize the main-thread and impl-side animation lists, removing aborted
  // animations and pushing any new animations.
  MarkAbortedAnimationsForDeletion(animation_ticker_impl);
  PurgeAnimationsMarkedForDeletion(/* impl_only */ false);
  PushNewAnimationsToImplThread(animation_ticker_impl);

  // Remove finished impl side animations only after pushing,
  // and only after the animations are deleted on the main thread
  // this insures we will never push an animation twice.
  RemoveAnimationsCompletedOnMainThread(animation_ticker_impl);

  // Now that the animation lists are synchronized, push the properties for the
  // individual animations.
  for (const auto& animation : animations_) {
    Animation* current_impl =
        animation_ticker_impl->GetAnimationById(animation->id());
    if (current_impl)
      animation->PushPropertiesTo(current_impl);
  }
  animation_ticker_impl->scroll_offset_animation_was_interrupted_ =
      scroll_offset_animation_was_interrupted_;
  scroll_offset_animation_was_interrupted_ = false;

  animation_ticker_impl->UpdateTickingState(UpdateTickingType::NORMAL);
}

std::string AnimationTicker::AnimationsToString() const {
  std::string str;
  for (size_t i = 0; i < animations_.size(); i++) {
    if (i > 0)
      str.append(", ");
    str.append(animations_[i]->ToString());
  }
  return str;
}

void AnimationTicker::StartAnimations(base::TimeTicks monotonic_time) {
  DCHECK(needs_to_start_animations_);
  needs_to_start_animations_ = false;

  // First collect running properties affecting each type of element.
  TargetProperties blocked_properties_for_active_elements;
  TargetProperties blocked_properties_for_pending_elements;
  std::vector<size_t> animations_waiting_for_target;

  animations_waiting_for_target.reserve(animations_.size());
  for (size_t i = 0; i < animations_.size(); ++i) {
    auto& animation = animations_[i];
    if (animation->run_state() == Animation::STARTING ||
        animation->run_state() == Animation::RUNNING) {
      int property = animation->target_property_id();
      if (animation->affects_active_elements()) {
        blocked_properties_for_active_elements[property] = true;
      }
      if (animation->affects_pending_elements()) {
        blocked_properties_for_pending_elements[property] = true;
      }
    } else if (animation->run_state() ==
               Animation::WAITING_FOR_TARGET_AVAILABILITY) {
      animations_waiting_for_target.push_back(i);
    }
  }

  for (size_t i = 0; i < animations_waiting_for_target.size(); ++i) {
    // Collect all properties for animations with the same group id (they
    // should all also be in the list of animations).
    size_t animation_index = animations_waiting_for_target[i];
    Animation* animation_waiting_for_target =
        animations_[animation_index].get();
    // Check for the run state again even though the animation was waiting
    // for target because it might have changed the run state while handling
    // previous animation in this loop (if they belong to same group).
    if (animation_waiting_for_target->run_state() ==
        Animation::WAITING_FOR_TARGET_AVAILABILITY) {
      TargetProperties enqueued_properties;
      bool affects_active_elements =
          animation_waiting_for_target->affects_active_elements();
      bool affects_pending_elements =
          animation_waiting_for_target->affects_pending_elements();
      enqueued_properties[animation_waiting_for_target->target_property_id()] =
          true;
      for (size_t j = animation_index + 1; j < animations_.size(); ++j) {
        if (animation_waiting_for_target->group() == animations_[j]->group()) {
          enqueued_properties[animations_[j]->target_property_id()] = true;
          affects_active_elements |= animations_[j]->affects_active_elements();
          affects_pending_elements |=
              animations_[j]->affects_pending_elements();
        }
      }

      // Check to see if intersection of the list of properties affected by
      // the group and the list of currently blocked properties is null, taking
      // into account the type(s) of elements affected by the group. In any
      // case, the group's target properties need to be added to the lists of
      // blocked properties.
      bool null_intersection = true;
      for (int property = TargetProperty::FIRST_TARGET_PROPERTY;
           property <= TargetProperty::LAST_TARGET_PROPERTY; ++property) {
        if (enqueued_properties[property]) {
          if (affects_active_elements) {
            if (blocked_properties_for_active_elements[property])
              null_intersection = false;
            else
              blocked_properties_for_active_elements[property] = true;
          }
          if (affects_pending_elements) {
            if (blocked_properties_for_pending_elements[property])
              null_intersection = false;
            else
              blocked_properties_for_pending_elements[property] = true;
          }
        }
      }

      // If the intersection is null, then we are free to start the animations
      // in the group.
      if (null_intersection) {
        animation_waiting_for_target->SetRunState(Animation::STARTING,
                                                  monotonic_time);
        for (size_t j = animation_index + 1; j < animations_.size(); ++j) {
          if (animation_waiting_for_target->group() ==
              animations_[j]->group()) {
            animations_[j]->SetRunState(Animation::STARTING, monotonic_time);
          }
        }
      } else {
        needs_to_start_animations_ = true;
      }
    }
  }
}

void AnimationTicker::PromoteStartedAnimations(AnimationEvents* events) {
  for (auto& animation : animations_) {
    if (animation->run_state() == Animation::STARTING &&
        animation->affects_active_elements()) {
      animation->SetRunState(Animation::RUNNING, last_tick_time_);
      if (!animation->has_set_start_time() &&
          !animation->needs_synchronized_start_time())
        animation->set_start_time(last_tick_time_);
      if (events) {
        base::TimeTicks start_time;
        if (animation->has_set_start_time())
          start_time = animation->start_time();
        else
          start_time = last_tick_time_;
        AnimationEvent started_event(
            AnimationEvent::STARTED, element_id_, animation->group(),
            animation->target_property_id(), start_time);
        started_event.is_impl_only = animation->is_impl_only();
        if (started_event.is_impl_only) {
          // Notify delegate directly, do not record the event.
          animation_player_->NotifyAnimationStarted(started_event);
        } else {
          events->events_.push_back(started_event);
        }
      }
    }
  }
}

void AnimationTicker::MarkAnimationsForDeletion(base::TimeTicks monotonic_time,
                                                AnimationEvents* events) {
  bool marked_animations_for_deletions = false;
  std::vector<size_t> animations_with_same_group_id;

  animations_with_same_group_id.reserve(animations_.size());
  // Non-aborted animations are marked for deletion after a corresponding
  // AnimationEvent::FINISHED event is sent or received. This means that if
  // we don't have an events vector, we must ensure that non-aborted animations
  // have received a finished event before marking them for deletion.
  for (size_t i = 0; i < animations_.size(); i++) {
    auto& animation = animations_[i];
    int group_id = animation->group();
    if (animation->run_state() == Animation::ABORTED) {
      if (events && !animation->is_impl_only()) {
        AnimationEvent aborted_event(AnimationEvent::ABORTED, element_id_,
                                     group_id, animation->target_property_id(),
                                     monotonic_time);
        events->events_.push_back(aborted_event);
      }
      // If on the compositor or on the main thread and received finish event,
      // animation can be marked for deletion.
      if (events || animation->received_finished_event()) {
        animation->SetRunState(Animation::WAITING_FOR_DELETION, monotonic_time);
        marked_animations_for_deletions = true;
      }
      continue;
    }

    // If running on the compositor and need to complete an aborted animation
    // on the main thread.
    if (events &&
        animation->run_state() == Animation::ABORTED_BUT_NEEDS_COMPLETION) {
      AnimationEvent aborted_event(AnimationEvent::TAKEOVER, element_id_,
                                   group_id, animation->target_property_id(),
                                   monotonic_time);
      aborted_event.animation_start_time = animation->start_time();
      const ScrollOffsetAnimationCurve* scroll_offset_animation_curve =
          animation->curve()->ToScrollOffsetAnimationCurve();
      aborted_event.curve = scroll_offset_animation_curve->Clone();
      // Notify the compositor that the animation is finished.
      animation_player_->NotifyAnimationFinished(aborted_event);
      // Notify main thread.
      events->events_.push_back(aborted_event);

      // Remove the animation from the compositor.
      animation->SetRunState(Animation::WAITING_FOR_DELETION, monotonic_time);
      marked_animations_for_deletions = true;
      continue;
    }

    bool all_anims_with_same_id_are_finished = false;

    // Since deleting an animation on the main thread leads to its deletion
    // on the impl thread, we only mark a FINISHED main thread animation for
    // deletion once it has received a FINISHED event from the impl thread.
    bool animation_i_will_send_or_has_received_finish_event =
        animation->is_controlling_instance() || animation->is_impl_only() ||
        animation->received_finished_event();
    // If an animation is finished, and not already marked for deletion,
    // find out if all other animations in the same group are also finished.
    if (animation->run_state() == Animation::FINISHED &&
        animation_i_will_send_or_has_received_finish_event) {
      // Clear the animations_with_same_group_id if it was added for
      // the previous animation's iteration.
      if (animations_with_same_group_id.size() > 0)
        animations_with_same_group_id.clear();
      all_anims_with_same_id_are_finished = true;
      for (size_t j = 0; j < animations_.size(); ++j) {
        auto& animation_j = animations_[j];
        bool animation_j_will_send_or_has_received_finish_event =
            animation_j->is_controlling_instance() ||
            animation_j->is_impl_only() ||
            animation_j->received_finished_event();
        if (group_id == animation_j->group()) {
          if (!animation_j->is_finished() ||
              (animation_j->run_state() == Animation::FINISHED &&
               !animation_j_will_send_or_has_received_finish_event)) {
            all_anims_with_same_id_are_finished = false;
            break;
          } else if (j >= i && animation_j->run_state() != Animation::ABORTED) {
            // Mark down the animations which belong to the same group
            // and is not yet aborted. If this current iteration finds that all
            // animations with same ID are finished, then the marked
            // animations below will be set to WAITING_FOR_DELETION in next
            // iteration.
            animations_with_same_group_id.push_back(j);
          }
        }
      }
    }

    if (all_anims_with_same_id_are_finished) {
      // We now need to remove all animations with the same group id as
      // group_id (and send along animation finished notifications, if
      // necessary).
      for (size_t j = 0; j < animations_with_same_group_id.size(); j++) {
        size_t animation_index = animations_with_same_group_id[j];
        auto& grouped_animation = animations_[animation_index];
        if (events) {
          AnimationEvent finished_event(
              AnimationEvent::FINISHED, element_id_, grouped_animation->group(),
              grouped_animation->target_property_id(), monotonic_time);
          finished_event.is_impl_only = grouped_animation->is_impl_only();
          if (finished_event.is_impl_only) {
            // Notify delegate directly, do not record the event.
            animation_player_->NotifyAnimationFinished(finished_event);
          } else {
            events->events_.push_back(finished_event);
          }
        }
        grouped_animation->SetRunState(Animation::WAITING_FOR_DELETION,
                                       monotonic_time);
      }
      marked_animations_for_deletions = true;
    }
  }

  // We need to purge animations marked for deletion, which happens in
  // PushPropertiesTo().
  if (marked_animations_for_deletions)
    SetNeedsPushProperties();
}

void AnimationTicker::MarkFinishedAnimations(base::TimeTicks monotonic_time) {
  DCHECK(has_bound_element_animations());

  bool animation_finished = false;
  for (auto& animation : animations_) {
    if (!animation->is_finished() && animation->IsFinishedAt(monotonic_time)) {
      animation->SetRunState(Animation::FINISHED, monotonic_time);
      animation_finished = true;
      SetNeedsPushProperties();
    }
    if (!animation->affects_active_elements() &&
        !animation->affects_pending_elements()) {
      switch (animation->run_state()) {
        case Animation::WAITING_FOR_TARGET_AVAILABILITY:
        case Animation::STARTING:
        case Animation::RUNNING:
        case Animation::PAUSED:
          animation->SetRunState(Animation::FINISHED, monotonic_time);
          animation_finished = true;
          break;
        default:
          break;
      }
    }
  }
  if (animation_finished)
    element_animations_->UpdateClientAnimationState();
}

bool AnimationTicker::HasElementInActiveList() const {
  DCHECK(has_bound_element_animations());
  return element_animations_->has_element_in_active_list();
}

gfx::ScrollOffset AnimationTicker::ScrollOffsetForAnimation() const {
  DCHECK(has_bound_element_animations());
  return element_animations_->ScrollOffsetForAnimation();
}

}  // namespace cc
