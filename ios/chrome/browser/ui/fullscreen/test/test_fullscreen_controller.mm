// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_controller.h"

#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TestFullscreenController::TestFullscreenController(FullscreenModel* model)
    : FullscreenController(), model_(model) {}

TestFullscreenController::~TestFullscreenController() = default;

ChromeBroadcaster* TestFullscreenController::broadcaster() {
  return nil;
}

void TestFullscreenController::SetWebStateList(WebStateList* web_state_list) {}

void TestFullscreenController::AddObserver(
    FullscreenControllerObserver* observer) {}

void TestFullscreenController::RemoveObserver(
    FullscreenControllerObserver* observer) {}

bool TestFullscreenController::IsEnabled() const {
  return model_->enabled();
}

void TestFullscreenController::IncrementDisabledCounter() {
  model_->IncrementDisabledCounter();
}

void TestFullscreenController::DecrementDisabledCounter() {
  model_->DecrementDisabledCounter();
}

CGFloat TestFullscreenController::GetProgress() const {
  return model_->progress();
}

void TestFullscreenController::ResetModel() {
  model_->ResetForNavigation();
}
