// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_DELEGATE_IOS_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_DELEGATE_IOS_H_

#import <Foundation/Foundation.h>

#include "components/infobars/core/infobar_container.h"
#include "third_party/skia/include/core/SkColor.h"

@protocol InfobarContainerStateDelegate;

// iOS implementation of InfoBarContainer::Delegate. Most of the method
// implementations are no-ops, since the iOS UI should not be driven or managed
// by the cross-platform infobar code.
class InfoBarContainerDelegateIOS
    : public infobars::InfoBarContainer::Delegate {
 public:
  explicit InfoBarContainerDelegateIOS(
      id<InfobarContainerStateDelegate> delegate)
      : delegate_(delegate) {}

  ~InfoBarContainerDelegateIOS() override {}

  // Informs |delegate_| that the container state has changed.
  void InfoBarContainerStateChanged(bool is_animating) override;

 private:
  __weak id<InfobarContainerStateDelegate> delegate_;
};

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_DELEGATE_IOS_H_
