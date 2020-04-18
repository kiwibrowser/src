// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/infobars/infobar_container_cocoa.h"

#import "chrome/browser/ui/cocoa/infobars/infobar_cocoa.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_controller.h"

InfoBarContainerCocoa::InfoBarContainerCocoa(
    InfoBarContainerController* controller)
    : infobars::InfoBarContainer(this),
      controller_(controller) {
}

InfoBarContainerCocoa::~InfoBarContainerCocoa() {
  RemoveAllInfoBarsForDestruction();
}

void InfoBarContainerCocoa::PlatformSpecificAddInfoBar(
    infobars::InfoBar* infobar,
    size_t position) {
  InfoBarCocoa* infobar_cocoa = static_cast<InfoBarCocoa*>(infobar);
  [controller_ addInfoBar:infobar_cocoa position:position];
}

void InfoBarContainerCocoa::PlatformSpecificRemoveInfoBar(
    infobars::InfoBar* infobar) {
  InfoBarCocoa* infobar_cocoa = static_cast<InfoBarCocoa*>(infobar);
  [controller_ removeInfoBar:infobar_cocoa];
}

void InfoBarContainerCocoa::InfoBarContainerStateChanged(bool is_animating) {
  [controller_ positionInfoBarsAndRedraw:is_animating];
}
