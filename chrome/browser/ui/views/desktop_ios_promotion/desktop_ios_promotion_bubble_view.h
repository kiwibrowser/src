// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_util.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"

class DesktopIOSPromotionBubbleController;
class Profile;

// The DesktopIOSPromotionBubbleView is the main view for the desktop-to-ios
// promotion view. It proxies DialogClientView functionality to provide
// replacement button labels, actions and icon when activated.
class DesktopIOSPromotionBubbleView : public DesktopIOSPromotionView,
                                      public views::View {
 public:
  DesktopIOSPromotionBubbleView(
      Profile* profile,
      desktop_ios_promotion::PromotionEntryPoint entry_point);
  ~DesktopIOSPromotionBubbleView() override;

  // DesktopIOSPromotionView:
  void UpdateRecoveryPhoneLabel() override;

  bool Accept();
  bool Cancel();
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const;
  gfx::ImageSkia GetWindowIcon();

 private:
  // The text that will appear on the promotion body.
  views::Label* promotion_text_label_;
  std::unique_ptr<DesktopIOSPromotionBubbleController> promotion_controller_;

  DISALLOW_COPY_AND_ASSIGN(DesktopIOSPromotionBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_BUBBLE_VIEW_H_
