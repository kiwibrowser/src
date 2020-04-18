// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/desktop_ios_promotion/desktop_ios_promotion_footnote_view.h"

#include <stddef.h>

#include <memory>

#include "base/strings/string16.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_controller.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_footnote_delegate.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_util.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/font.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/fill_layout.h"

DesktopIOSPromotionFootnoteView::DesktopIOSPromotionFootnoteView(
    Profile* profile,
    DesktopIOSPromotionFootnoteDelegate* delegate)
    : StyledLabel(base::string16(), this),
      promotion_controller_(std::make_unique<DesktopIOSPromotionController>(
          profile,
          desktop_ios_promotion::PromotionEntryPoint::BOOKMARKS_FOOTNOTE)),
      delegate_(delegate) {
  size_t offset = 0;
  base::string16 link_text =
      l10n_util::GetStringUTF16(IDS_FOOTNOTE_DESKTOP_TO_IOS_PROMO_LINK);
  base::string16 promo_text = l10n_util::GetStringFUTF16(
      IDS_BOOKMARK_FOOTNOTE_DESKTOP_TO_IOS_PROMO_MESSAGE, link_text, &offset);
  SetText(promo_text);

  AddStyleRange(gfx::Range(offset, offset + link_text.length()),
                views::StyledLabel::RangeStyleInfo::CreateForLink());

  views::StyledLabel::RangeStyleInfo promo_style;
  promo_style.text_style = STYLE_SECONDARY;
  gfx::Range before_link_range(0, offset);
  if (!before_link_range.is_empty())
    AddStyleRange(before_link_range, promo_style);
  gfx::Range after_link_range(offset + link_text.length(), promo_text.length());
  if (!after_link_range.is_empty())
    AddStyleRange(after_link_range, promo_style);
  promotion_controller_->OnPromotionShown();
}

DesktopIOSPromotionFootnoteView::~DesktopIOSPromotionFootnoteView() {}

void DesktopIOSPromotionFootnoteView::StyledLabelLinkClicked(
    views::StyledLabel* label,
    const gfx::Range& range,
    int event_flags) {
  promotion_controller_->OnLearnMoreLinkClicked();
  delegate_->OnIOSPromotionFootnoteLinkClicked();
}
