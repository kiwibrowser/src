// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_VIEW_H_
#define CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_VIEW_H_

// Interface for The Desktop iOS promotion view.
class DesktopIOSPromotionView {
 public:
  virtual void UpdateRecoveryPhoneLabel() = 0;

 protected:
  virtual ~DesktopIOSPromotionView() = default;
};

#endif  // CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_VIEW_H_
