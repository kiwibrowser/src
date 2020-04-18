// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/test/toolbar_test_web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

ToolbarTestWebState::ToolbarTestWebState() : loading_progress_(0){};

double ToolbarTestWebState::GetLoadingProgress() const {
  return loading_progress_;
}

void ToolbarTestWebState::set_loading_progress(double loading_progress) {
  loading_progress_ = loading_progress;
}
