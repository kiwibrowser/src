// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/omnibox/omnibox_tab_switch_button.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"
#include "chrome/browser/ui/views/omnibox/omnibox_popup_contents_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/paint_vector_icon.h"

bool OmniboxTabSwitchButton::calculated_widths_ = false;
size_t OmniboxTabSwitchButton::icon_only_width_;
size_t OmniboxTabSwitchButton::short_text_width_;
size_t OmniboxTabSwitchButton::full_text_width_;

OmniboxTabSwitchButton::OmniboxTabSwitchButton(OmniboxPopupContentsView* model,
                                               OmniboxResultView* result_view,
                                               int text_height)
    : MdTextButton(result_view, views::style::CONTEXT_BUTTON_MD),
      text_height_(text_height),
      model_(model),
      result_view_(result_view) {
  // TODO(krb): SetTooltipText(text);
  SetBgColorOverride(GetBackgroundColor());
  SetImage(STATE_NORMAL,
           gfx::CreateVectorIcon(omnibox::kSwitchIcon,
                                 GetLayoutConstant(LOCATION_BAR_ICON_SIZE),
                                 SK_ColorBLACK));
  if (!calculated_widths_) {
    icon_only_width_ = MdTextButton::CalculatePreferredSize().width();
    SetText(l10n_util::GetStringUTF16(IDS_OMNIBOX_TAB_SUGGEST_SHORT_HINT));
    short_text_width_ = MdTextButton::CalculatePreferredSize().width();
    SetText(l10n_util::GetStringUTF16(IDS_OMNIBOX_TAB_SUGGEST_HINT));
    full_text_width_ = MdTextButton::CalculatePreferredSize().width();
    calculated_widths_ = true;
  } else {
    SetText(l10n_util::GetStringUTF16(IDS_OMNIBOX_TAB_SUGGEST_HINT));
  }
  visible_ = true;
  set_corner_radius(CalculatePreferredSize().height() / 2.f);
}

void OmniboxTabSwitchButton::ProvideWidthHint(size_t parent_width) {
  visible_ = true;
  if (full_text_width_ < parent_width / 5) {
    SetText(l10n_util::GetStringUTF16(IDS_OMNIBOX_TAB_SUGGEST_HINT));
  } else if (short_text_width_ < parent_width / 5) {
    SetText(l10n_util::GetStringUTF16(IDS_OMNIBOX_TAB_SUGGEST_SHORT_HINT));
  } else if (icon_only_width_ < parent_width / 5) {
    SetText(base::ASCIIToUTF16(""));
  } else {
    visible_ = false;
  }
}

gfx::Size OmniboxTabSwitchButton::CalculatePreferredSize() const {
  if (!visible_)
    return gfx::Size();
  gfx::Size size = MdTextButton::CalculatePreferredSize();
  size.set_height(text_height_ + 2 * kVerticalPadding);
  return size;
}

void OmniboxTabSwitchButton::StateChanged(ButtonState old_state) {
  if (state() == STATE_NORMAL) {
    // If used to be pressed, transfer ownership.
    if (old_state == STATE_PRESSED) {
      SetBgColorOverride(GetBackgroundColor());
      SetMouseHandler(parent());
      if (model_->IsButtonSelected())
        model_->UnselectButton();
    // Otherwise was hovered. Update color if not selected.
    } else if (!model_->IsButtonSelected()) {
      SetBgColorOverride(GetBackgroundColor());
    }
  }
  if (state() == STATE_HOVERED) {
    if (!model_->IsButtonSelected() && old_state == STATE_NORMAL) {
      SetBgColorOverride(GetBackgroundColor());
    }
  }
  if (state() == STATE_PRESSED)
    SetPressed();
  MdTextButton::StateChanged(old_state);
}

void OmniboxTabSwitchButton::UpdateBackground() {
  if (model_->IsButtonSelected())
    SetPressed();
  else
    SetBgColorOverride(GetBackgroundColor());
}

SkColor OmniboxTabSwitchButton::GetBackgroundColor() const {
  return GetOmniboxColor(OmniboxPart::RESULTS_BACKGROUND,
                         result_view_->GetTint(),
                         state() == STATE_HOVERED ? OmniboxPartState::HOVERED
                                                  : OmniboxPartState::NORMAL);
}

void OmniboxTabSwitchButton::SetPressed() {
  SetBgColorOverride(color_utils::AlphaBlend(
      GetOmniboxColor(OmniboxPart::RESULTS_BACKGROUND, result_view_->GetTint(),
                      OmniboxPartState::SELECTED),
      SK_ColorBLACK, 0.8 * 255));
}
