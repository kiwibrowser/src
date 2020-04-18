// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_FOOTNOTE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_FOOTNOTE_VIEW_H_

#include "base/macros.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/controls/styled_label_listener.h"

class DesktopIOSPromotionController;
class DesktopIOSPromotionFootnoteDelegate;
class Profile;

// Desktop iOS promotion Footnote displayed at the bottom of bookmark bubble.
class DesktopIOSPromotionFootnoteView : public views::StyledLabel,
                                        public views::StyledLabelListener {
 public:
  DesktopIOSPromotionFootnoteView(
      Profile* profile,
      DesktopIOSPromotionFootnoteDelegate* delegate);
  ~DesktopIOSPromotionFootnoteView() override;

 private:
  // views::StyledLabelListener:
  void StyledLabelLinkClicked(views::StyledLabel* label,
                              const gfx::Range& range,
                              int event_flags) override;

  std::unique_ptr<DesktopIOSPromotionController> promotion_controller_;
  // Delegate, to handle clicks on the sign in link.
  DesktopIOSPromotionFootnoteDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(DesktopIOSPromotionFootnoteView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_FOOTNOTE_VIEW_H_
