// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTAINER_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTAINER_COCOA_H_

#include <stddef.h>

#include "base/macros.h"
#include "components/infobars/core/infobar_container.h"

@class InfoBarContainerController;

// The cocoa specific implementation of InfoBarContainer. This mostly serves as
// a bridge for InfoBarContainerController.
class InfoBarContainerCocoa : public infobars::InfoBarContainer,
                              public infobars::InfoBarContainer::Delegate {
 public:
  explicit InfoBarContainerCocoa(InfoBarContainerController* controller);
  ~InfoBarContainerCocoa() override;

 private:
  // InfoBarContainer:
  void PlatformSpecificAddInfoBar(infobars::InfoBar* infobar,
                                  size_t position) override;
  void PlatformSpecificRemoveInfoBar(infobars::InfoBar* infobar) override;

  // InfoBarContainer::Delegate:
  void InfoBarContainerStateChanged(bool is_animating) override;

  InfoBarContainerController* controller_;  // weak, owns us.

  DISALLOW_COPY_AND_ASSIGN(InfoBarContainerCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTAINER_COCOA_H_
