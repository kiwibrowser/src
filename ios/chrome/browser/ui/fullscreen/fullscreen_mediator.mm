// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_mediator.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_animator.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_observer.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#include "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FullscreenMediator::FullscreenMediator(FullscreenController* controller,
                                       FullscreenModel* model)
    : controller_(controller), model_(model) {
  DCHECK(controller_);
  DCHECK(model_);
  model_->AddObserver(this);
}

FullscreenMediator::~FullscreenMediator() {
  // Disconnect() is expected to be called before deallocation.
  DCHECK(!controller_);
  DCHECK(!model_);
}

void FullscreenMediator::ScrollToTop() {
  FullscreenAnimatorStyle scrollToTopStyle =
      FullscreenAnimatorStyle::EXIT_FULLSCREEN;
  if (animator_ && animator_.style == scrollToTopStyle)
    return;
  StopAnimating(true);

  SetUpAnimator(scrollToTopStyle);
  for (auto& observer : observers_) {
    observer.FullscreenWillScrollToTop(controller_, animator_);
  }
  StartAnimator();
}

void FullscreenMediator::WillEnterForeground() {
  FullscreenAnimatorStyle enterForegroundStyle =
      FullscreenAnimatorStyle::EXIT_FULLSCREEN;
  if (animator_ && animator_.style == enterForegroundStyle)
    return;
  StopAnimating(true);

  SetUpAnimator(enterForegroundStyle);
  for (auto& observer : observers_) {
    observer.FullscreenWillEnterForeground(controller_, animator_);
  }
  StartAnimator();
}

void FullscreenMediator::AnimateModelReset() {
  FullscreenAnimatorStyle resetStyle = FullscreenAnimatorStyle::EXIT_FULLSCREEN;
  if (animator_ && animator_.style == resetStyle)
    return;
  StopAnimating(true);

  SetUpAnimator(resetStyle);
  for (auto& observer : observers_) {
    observer.FullscreenModelWasReset(controller_, animator_);
  }
  StartAnimator();
}

void FullscreenMediator::Disconnect() {
  [animator_ stopAnimation:YES];
  animator_ = nil;
  model_->RemoveObserver(this);
  model_ = nullptr;
  controller_ = nullptr;
}

void FullscreenMediator::FullscreenModelProgressUpdated(
    FullscreenModel* model) {
  DCHECK_EQ(model_, model);
  StopAnimating(true /* update_model */);
  for (auto& observer : observers_) {
    observer.FullscreenProgressUpdated(controller_, model_->progress());
  }
}

void FullscreenMediator::FullscreenModelEnabledStateChanged(
    FullscreenModel* model) {
  DCHECK_EQ(model_, model);
  StopAnimating(true /* update_model */);
  for (auto& observer : observers_) {
    observer.FullscreenEnabledStateChanged(controller_, model->enabled());
  }
}

void FullscreenMediator::FullscreenModelScrollEventStarted(
    FullscreenModel* model) {
  DCHECK_EQ(model_, model);
  StopAnimating(true /* update_model */);
}

void FullscreenMediator::FullscreenModelScrollEventEnded(
    FullscreenModel* model) {
  DCHECK_EQ(model_, model);
  FullscreenAnimatorStyle scrollEndStyle =
      model_->progress() >= 0.5 ? FullscreenAnimatorStyle::EXIT_FULLSCREEN
                                : FullscreenAnimatorStyle::ENTER_FULLSCREEN;
  if (animator_ && animator_.style == scrollEndStyle)
    return;
  StopAnimating(true);

  SetUpAnimator(scrollEndStyle);
  for (auto& observer : observers_) {
    observer.FullscreenScrollEventEnded(controller_, animator_);
  }
  StartAnimator();
}

void FullscreenMediator::FullscreenModelWasReset(FullscreenModel* model) {
  // Stop any in-progress animations.  Don't update the model because this
  // callback occurs after the model's state is reset, and updating the model
  // the with active animator's current value would overwrite the reset value.
  StopAnimating(false /* update_model */);
  // Update observers for the reset progress value.
  for (auto& observer : observers_) {
    observer.FullscreenProgressUpdated(controller_, model_->progress());
  }
}

void FullscreenMediator::SetUpAnimator(FullscreenAnimatorStyle style) {
  DCHECK(!animator_);
  animator_ =
      [[FullscreenAnimator alloc] initWithStartProgress:model_->progress()
                                                  style:style];
  __weak FullscreenAnimator* weakAnimator = animator_;
  FullscreenModel** modelPtr = &model_;
  [animator_ addCompletion:^(UIViewAnimatingPosition finalPosition) {
    DCHECK_EQ(finalPosition, UIViewAnimatingPositionEnd);
    if (!weakAnimator || !*modelPtr)
      return;
    model_->AnimationEndedWithProgress(
        [weakAnimator progressForAnimatingPosition:finalPosition]);
    animator_ = nil;
  }];
}

void FullscreenMediator::StartAnimator() {
  // Only start the animator if animations have been added and it has a non-zero
  // progress change.
  if (animator_.hasAnimations &&
      !AreCGFloatsEqual(animator_.startProgress, animator_.finalProgress)) {
    [animator_ startAnimation];
  } else {
    animator_ = nil;
  }
}

void FullscreenMediator::StopAnimating(bool update_model) {
  if (!animator_)
    return;

  DCHECK_EQ(animator_.state, UIViewAnimatingStateActive);
  if (update_model)
    model_->AnimationEndedWithProgress(animator_.currentProgress);
  [animator_ stopAnimation:YES];
  animator_ = nil;
}
