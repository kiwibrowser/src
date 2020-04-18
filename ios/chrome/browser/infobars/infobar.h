// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_H_

#include <memory>

#include "base/macros.h"
#include "components/infobars/core/infobar.h"
#import "ios/chrome/browser/infobars/infobar_controller_delegate.h"
#import "ios/chrome/browser/ui/infobars/infobar_view_sizing.h"

@class InfoBarController;
namespace infobars {
class InfoBarDelegate;
}

// The iOS version of infobars::InfoBar.
class InfoBarIOS : public infobars::InfoBar, public InfoBarControllerDelegate {
 public:
  InfoBarIOS(InfoBarController* controller,
             std::unique_ptr<infobars::InfoBarDelegate> delegate);
  ~InfoBarIOS() override;

  // Lays out the infobar view using data from the delegate to add to superview.
  void Layout(CGRect container_bounds);

  // Returns the infobar view holding contents of this infobar.
  UIView<InfoBarViewSizing>* view();

  // Remove the infobar view from infobar container view.
  void RemoveView();

 private:
  // infobars::InfoBar overrides:
  void PlatformSpecificOnHeightRecalculated() override;

  // InfoBarControllerDelegate overrides:
  void SetInfoBarTargetHeight(int height) override;
  void RemoveInfoBar() override;

  InfoBarController* controller_;
  DISALLOW_COPY_AND_ASSIGN(InfoBarIOS);
};

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_H_
