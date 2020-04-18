// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/desktop_ios_promotion/desktop_ios_promotion_bubble_view.h"

#include <memory>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_bubble_controller.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/border.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

DesktopIOSPromotionBubbleView::DesktopIOSPromotionBubbleView(
    Profile* profile,
    desktop_ios_promotion::PromotionEntryPoint entry_point)
    : promotion_text_label_(
          new views::Label(desktop_ios_promotion::GetPromoText(entry_point),
                           CONTEXT_BODY_TEXT_LARGE,
                           STYLE_SECONDARY)),
      promotion_controller_(
          std::make_unique<DesktopIOSPromotionBubbleController>(profile,
                                                                this,
                                                                entry_point)) {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  SetBorder(
      views::CreateEmptyBorder(0,
                               ChromeLayoutProvider::Get()
                                       ->GetInsetsMetric(views::INSETS_DIALOG)
                                       .left() +
                                   GetWindowIcon().width(),
                               0, 0));

  promotion_text_label_->SetMultiLine(true);
  promotion_text_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(promotion_text_label_);

  promotion_controller_->OnPromotionShown();
}

DesktopIOSPromotionBubbleView::~DesktopIOSPromotionBubbleView() = default;

void DesktopIOSPromotionBubbleView::UpdateRecoveryPhoneLabel() {
  std::string number = promotion_controller_->GetUsersRecoveryPhoneNumber();
  if (!number.empty()) {
    promotion_text_label_->SetText(desktop_ios_promotion::GetPromoText(
        promotion_controller_->entry_point(), number));
    Layout();
    views::Widget* widget = GetWidget();
    gfx::Rect old_bounds = widget->GetWindowBoundsInScreen();
    old_bounds.set_height(
        widget->GetRootView()->GetHeightForWidth(old_bounds.width()));
    widget->SetBounds(old_bounds);
  }
}

bool DesktopIOSPromotionBubbleView::Accept() {
  promotion_controller_->OnSendSMSClicked();
  return true;
}

bool DesktopIOSPromotionBubbleView::Cancel() {
  promotion_controller_->OnNoThanksClicked();
  return true;
}

base::string16 DesktopIOSPromotionBubbleView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_OK
                                       ? IDS_DESKTOP_TO_IOS_PROMO_SEND_TO_PHONE
                                       : IDS_DESKTOP_TO_IOS_PROMO_NO_THANKS);
}

gfx::ImageSkia DesktopIOSPromotionBubbleView::GetWindowIcon() {
  return desktop_ios_promotion::GetPromoImage(GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_TextfieldDefaultColor));
}
