// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_FOOTNOTE_DELEGATE_H_
#define CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_FOOTNOTE_DELEGATE_H_

class DesktopIOSPromotionFootnoteDelegate {
 public:
  virtual ~DesktopIOSPromotionFootnoteDelegate() {}
  virtual void OnIOSPromotionFootnoteLinkClicked() = 0;
};

#endif  // CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_FOOTNOTE_DELEGATE_H_
