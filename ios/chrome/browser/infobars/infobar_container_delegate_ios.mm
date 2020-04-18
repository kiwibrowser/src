// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/infobars/infobar_container_delegate_ios.h"

#include "base/logging.h"
#include "ios/chrome/browser/infobars/infobar_container_state_delegate.h"
#include "ui/gfx/animation/slide_animation.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void InfoBarContainerDelegateIOS::InfoBarContainerStateChanged(
    bool is_animating) {
  [delegate_ infoBarContainerStateDidChangeAnimated:is_animating];
}
