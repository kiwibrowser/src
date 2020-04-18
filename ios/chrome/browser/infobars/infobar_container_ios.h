// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_IOS_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_IOS_H_

#include "components/infobars/core/infobar_container.h"

#include <stddef.h>

#include "base/macros.h"

@class InfoBarContainerView;

// IOS infobar container specialization, managing infobars visibility so
// that only the front most one is visible at any time.
class InfoBarContainerIOS : public infobars::InfoBarContainer {
 public:
  explicit InfoBarContainerIOS(infobars::InfoBarContainer::Delegate* delegate);
  ~InfoBarContainerIOS() override;

  // Returns the UIView container.
  InfoBarContainerView* view();

  // Hides the current infobar keeping the current state. If a new infobar is
  // added, it will be hidden as well.
  void SuspendInfobars();

  // Restores the normal behavior of the infobars.
  void RestoreInfobars();

 protected:
  void PlatformSpecificAddInfoBar(infobars::InfoBar* infobar,
                                  size_t position) override;
  void PlatformSpecificRemoveInfoBar(infobars::InfoBar* infobar) override;
  void PlatformSpecificInfoBarStateChanged(bool is_animating) override;

 private:
  InfoBarContainerView* container_view_;
  InfoBarContainer::Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(InfoBarContainerIOS);
};

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_IOS_H_
