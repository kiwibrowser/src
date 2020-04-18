// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/animation/bounds_animator.h"

#include <memory>

#include "ui/gfx/animation/animation_container.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/animation/bounds_animator_observer.h"
#include "ui/views/view.h"

// Duration in milliseconds for animations.
static const int kDefaultAnimationDuration = 200;

using gfx::Animation;
using gfx::AnimationContainer;
using gfx::SlideAnimation;
using gfx::Tween;

namespace views {

BoundsAnimator::BoundsAnimator(View* parent)
    : parent_(parent),
      container_(new AnimationContainer()),
      animation_duration_ms_(kDefaultAnimationDuration),
      tween_type_(Tween::EASE_OUT) {
  container_->set_observer(this);
}

BoundsAnimator::~BoundsAnimator() {
  // Reset the delegate so that we don't attempt to notify our observer from
  // the destructor.
  container_->set_observer(nullptr);

  // Delete all the animations, but don't remove any child views. We assume the
  // view owns us and is going to be deleted anyway.
  for (ViewToDataMap::iterator i = data_.begin(); i != data_.end(); ++i)
    CleanupData(false, &(i->second), i->first);
}

void BoundsAnimator::AnimateViewTo(View* view, const gfx::Rect& target) {
  DCHECK(view);
  DCHECK_EQ(view->parent(), parent_);

  Data existing_data;

  if (IsAnimating(view)) {
    // Don't immediatly delete the animation, that might trigger a callback from
    // the animationcontainer.
    existing_data = RemoveFromMaps(view);
  }

  // NOTE: we don't check if the view is already at the target location. Doing
  // so leads to odd cases where no animations may be present after invoking
  // AnimateViewTo. AnimationProgressed does nothing when the bounds of the
  // view don't change.

  Data& data = data_[view];
  data.start_bounds = view->bounds();
  data.target_bounds = target;
  data.animation = CreateAnimation();

  animation_to_view_[data.animation.get()] = view;

  data.animation->Show();

  CleanupData(true, &existing_data, nullptr);
}

void BoundsAnimator::SetTargetBounds(View* view, const gfx::Rect& target) {
  if (!IsAnimating(view)) {
    AnimateViewTo(view, target);
    return;
  }

  data_[view].target_bounds = target;
}

gfx::Rect BoundsAnimator::GetTargetBounds(View* view) {
  if (!IsAnimating(view))
    return view->bounds();
  return data_[view].target_bounds;
}

void BoundsAnimator::SetAnimationForView(
    View* view,
    std::unique_ptr<SlideAnimation> animation) {
  DCHECK(animation);

  if (!IsAnimating(view))
    return;

  // We delay deleting the animation until the end so that we don't prematurely
  // send out notification that we're done.
  std::unique_ptr<Animation> old_animation = ResetAnimationForView(view);

  SlideAnimation* animation_ptr = animation.get();
  data_[view].animation = std::move(animation);
  animation_to_view_[animation_ptr] = view;

  animation_ptr->set_delegate(this);
  animation_ptr->SetContainer(container_.get());
  animation_ptr->Show();
}

const SlideAnimation* BoundsAnimator::GetAnimationForView(View* view) {
  return !IsAnimating(view) ? nullptr : data_[view].animation.get();
}

void BoundsAnimator::SetAnimationDelegate(
    View* view,
    std::unique_ptr<AnimationDelegate> delegate) {
  DCHECK(IsAnimating(view));

  data_[view].delegate = std::move(delegate);
}

void BoundsAnimator::StopAnimatingView(View* view) {
  if (!IsAnimating(view))
    return;

  data_[view].animation->Stop();
}

bool BoundsAnimator::IsAnimating(View* view) const {
  return data_.find(view) != data_.end();
}

bool BoundsAnimator::IsAnimating() const {
  return !data_.empty();
}

void BoundsAnimator::Cancel() {
  if (data_.empty())
    return;

  while (!data_.empty())
    data_.begin()->second.animation->Stop();

  // Invoke AnimationContainerProgressed to force a repaint and notify delegate.
  AnimationContainerProgressed(container_.get());
}

void BoundsAnimator::SetAnimationDuration(int duration_ms) {
  animation_duration_ms_ = duration_ms;
}

void BoundsAnimator::AddObserver(BoundsAnimatorObserver* observer) {
  observers_.AddObserver(observer);
}

void BoundsAnimator::RemoveObserver(BoundsAnimatorObserver* observer) {
  observers_.RemoveObserver(observer);
}

std::unique_ptr<gfx::SlideAnimation> BoundsAnimator::CreateAnimation() {
  std::unique_ptr<gfx::SlideAnimation> animation =
      std::make_unique<SlideAnimation>(this);
  animation->SetContainer(container_.get());
  animation->SetSlideDuration(animation_duration_ms_);
  animation->SetTweenType(tween_type_);
  return animation;
}

BoundsAnimator::Data::Data() = default;
BoundsAnimator::Data::Data(Data&&) = default;
BoundsAnimator::Data& BoundsAnimator::Data::operator=(Data&&) = default;
BoundsAnimator::Data::~Data() = default;

BoundsAnimator::Data BoundsAnimator::RemoveFromMaps(View* view) {
  DCHECK(data_.count(view) > 0);
  DCHECK(animation_to_view_.count(data_[view].animation.get()) > 0);

  Data old_data = std::move(data_[view]);
  data_.erase(view);
  animation_to_view_.erase(old_data.animation.get());
  return old_data;
}

void BoundsAnimator::CleanupData(bool send_cancel, Data* data, View* view) {
  if (send_cancel && data->delegate)
    data->delegate->AnimationCanceled(data->animation.get());

  data->delegate.reset();

  if (data->animation) {
    data->animation->set_delegate(nullptr);
    data->animation.reset();
  }
}

std::unique_ptr<Animation> BoundsAnimator::ResetAnimationForView(View* view) {
  if (!IsAnimating(view))
    return nullptr;

  std::unique_ptr<Animation> old_animation = std::move(data_[view].animation);
  animation_to_view_.erase(old_animation.get());
  // Reset the delegate so that we don't attempt any processing when the
  // animation calls us back.
  old_animation->set_delegate(nullptr);
  return old_animation;
}

void BoundsAnimator::AnimationEndedOrCanceled(const Animation* animation,
                                              AnimationEndType type) {
  DCHECK(animation_to_view_.find(animation) != animation_to_view_.end());

  View* view = animation_to_view_[animation];
  DCHECK(view);

  // Save the data for later clean up.
  Data data = RemoveFromMaps(view);

  if (data.delegate) {
    if (type == ANIMATION_ENDED) {
      data.delegate->AnimationEnded(animation);
    } else {
      DCHECK_EQ(ANIMATION_CANCELED, type);
      data.delegate->AnimationCanceled(animation);
    }
  }

  CleanupData(false, &data, view);
}

void BoundsAnimator::AnimationProgressed(const Animation* animation) {
  DCHECK(animation_to_view_.find(animation) != animation_to_view_.end());

  View* view = animation_to_view_[animation];
  DCHECK(view);
  const Data& data = data_[view];
  gfx::Rect new_bounds =
      animation->CurrentValueBetween(data.start_bounds, data.target_bounds);
  if (new_bounds != view->bounds()) {
    gfx::Rect total_bounds = gfx::UnionRects(new_bounds, view->bounds());

    // Build up the region to repaint in repaint_bounds_. We'll do the repaint
    // when all animations complete (in AnimationContainerProgressed).
    repaint_bounds_.Union(total_bounds);

    view->SetBoundsRect(new_bounds);
  }

  if (data.delegate)
    data.delegate->AnimationProgressed(animation);
}

void BoundsAnimator::AnimationEnded(const Animation* animation) {
  AnimationEndedOrCanceled(animation, ANIMATION_ENDED);
}

void BoundsAnimator::AnimationCanceled(const Animation* animation) {
  AnimationEndedOrCanceled(animation, ANIMATION_CANCELED);
}

void BoundsAnimator::AnimationContainerProgressed(
    AnimationContainer* container) {
  if (!repaint_bounds_.IsEmpty()) {
    // Adjust for rtl.
    repaint_bounds_.set_x(parent_->GetMirroredXWithWidthInView(
        repaint_bounds_.x(), repaint_bounds_.width()));
    parent_->SchedulePaintInRect(repaint_bounds_);
    repaint_bounds_.SetRect(0, 0, 0, 0);
  }

  for (BoundsAnimatorObserver& observer : observers_)
    observer.OnBoundsAnimatorProgressed(this);

  if (!IsAnimating()) {
    // Notify here rather than from AnimationXXX to avoid deleting the animation
    // while the animation is calling us.
    for (BoundsAnimatorObserver& observer : observers_)
      observer.OnBoundsAnimatorDone(this);
  }
}

void BoundsAnimator::AnimationContainerEmpty(AnimationContainer* container) {
}

}  // namespace views
