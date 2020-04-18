// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/infobars/infobar_container_ios.h"

#include <stddef.h>
#import <UIKit/UIKit.h>

#include "base/logging.h"
#include "ios/chrome/browser/infobars/infobar.h"
#include "ios/chrome/browser/infobars/infobar_container_view.h"
#import "ios/chrome/common/material_timing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
void SetViewAlphaWithAnimation(UIView* view, float alpha) {
  CGFloat oldAlpha = [view alpha];
  if (oldAlpha > 0 && alpha == 0) {
    [view setUserInteractionEnabled:NO];
  }
  [UIView cr_transitionWithView:view
      duration:ios::material::kDuration3
      curve:ios::material::CurveEaseInOut
      options:0
      animations:^{
        [view setAlpha:alpha];
      }
      completion:^(BOOL) {
        if (oldAlpha == 0 && alpha > 0) {
          [view setUserInteractionEnabled:YES];
        };
      }];
}
}  // namespace

InfoBarContainerIOS::InfoBarContainerIOS(
    infobars::InfoBarContainer::Delegate* delegate)
    : InfoBarContainer(delegate), delegate_(delegate) {
  DCHECK(delegate);
  container_view_ = [[InfoBarContainerView alloc] init];
  [container_view_ setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                       UIViewAutoresizingFlexibleTopMargin];
}

InfoBarContainerIOS::~InfoBarContainerIOS() {
  delegate_ = nullptr;
  RemoveAllInfoBarsForDestruction();
}

InfoBarContainerView* InfoBarContainerIOS::view() {
  return container_view_;
}

void InfoBarContainerIOS::PlatformSpecificAddInfoBar(infobars::InfoBar* infobar,
                                                     size_t position) {
  InfoBarIOS* infobar_ios = static_cast<InfoBarIOS*>(infobar);
  [container_view_ addInfoBar:infobar_ios position:position];
}

void InfoBarContainerIOS::PlatformSpecificRemoveInfoBar(
    infobars::InfoBar* infobar) {
  InfoBarIOS* infobar_ios = static_cast<InfoBarIOS*>(infobar);
  infobar_ios->RemoveView();
  // If computed_height() is 0, then the infobar was removed after an animation.
  // In this case, signal the delegate that the state changed.
  // Otherwise, the infobar is being replaced by another one. Do not call the
  // delegate in this case, as the delegate will be updated when the new infobar
  // is added.
  if (infobar->computed_height() == 0 && delegate_)
    delegate_->InfoBarContainerStateChanged(false);
}

void InfoBarContainerIOS::PlatformSpecificInfoBarStateChanged(
    bool is_animating) {
  [container_view_ setUserInteractionEnabled:!is_animating];
  [container_view_ setNeedsLayout];
}

void InfoBarContainerIOS::SuspendInfobars() {
  SetViewAlphaWithAnimation(container_view_, 0);
}

void InfoBarContainerIOS::RestoreInfobars() {
  SetViewAlphaWithAnimation(container_view_, 1);
}
