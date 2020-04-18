/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/layout/layout_progress.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/html/html_progress_element.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

LayoutProgress::LayoutProgress(HTMLProgressElement* element)
    : LayoutBlockFlow(element),
      position_(HTMLProgressElement::kInvalidPosition),
      animation_start_time_(0),
      animation_repeat_interval_(0),
      animation_duration_(0),
      animating_(false),
      animation_timer_(
          element->GetDocument().GetTaskRunner(TaskType::kInternalDefault),
          this,
          &LayoutProgress::AnimationTimerFired) {}

LayoutProgress::~LayoutProgress() = default;

void LayoutProgress::WillBeDestroyed() {
  if (animating_) {
    animation_timer_.Stop();
    animating_ = false;
  }
  LayoutBlockFlow::WillBeDestroyed();
}

void LayoutProgress::UpdateFromElement() {
  HTMLProgressElement* element = ProgressElement();
  if (position_ == element->position())
    return;
  position_ = element->position();

  UpdateAnimationState();
  SetShouldDoFullPaintInvalidation();
  LayoutBlockFlow::UpdateFromElement();
}

double LayoutProgress::AnimationProgress() const {
  return animating_ ? (fmod((CurrentTime() - animation_start_time_),
                            animation_duration_) /
                       animation_duration_)
                    : 0;
}

bool LayoutProgress::IsDeterminate() const {
  return (HTMLProgressElement::kIndeterminatePosition != GetPosition() &&
          HTMLProgressElement::kInvalidPosition != GetPosition());
}

bool LayoutProgress::IsAnimationTimerActive() const {
  return animation_timer_.IsActive();
}

bool LayoutProgress::IsAnimating() const {
  return animating_;
}

void LayoutProgress::AnimationTimerFired(TimerBase*) {
  SetShouldDoFullPaintInvalidation();
  if (!animation_timer_.IsActive() && animating_)
    animation_timer_.StartOneShot(animation_repeat_interval_, FROM_HERE);
}

void LayoutProgress::UpdateAnimationState() {
  animation_duration_ =
      LayoutTheme::GetTheme().AnimationDurationForProgressBar();
  animation_repeat_interval_ =
      LayoutTheme::GetTheme().AnimationRepeatIntervalForProgressBar();

  bool animating =
      !IsDeterminate() && Style()->HasAppearance() && animation_duration_ > 0;
  if (animating == animating_)
    return;

  animating_ = animating;
  if (animating_) {
    animation_start_time_ = CurrentTime();
    animation_timer_.StartOneShot(animation_repeat_interval_, FROM_HERE);
  } else {
    animation_timer_.Stop();
  }
}

HTMLProgressElement* LayoutProgress::ProgressElement() const {
  return ToHTMLProgressElement(GetNode());
}

}  // namespace blink
