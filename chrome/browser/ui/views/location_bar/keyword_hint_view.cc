// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/keyword_hint_view.h"

#include <stddef.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/search_engines/template_url_service.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_utils.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"

KeywordHintView::KeywordHintView(views::ButtonListener* listener,
                                 Profile* profile,
                                 OmniboxTint tint)
    : Button(listener),
      profile_(profile),
      leading_label_(nullptr),
      chip_container_(new views::View()),
      chip_label_(
          new views::Label(base::string16(), CONTEXT_OMNIBOX_DECORATION)),
      trailing_label_(nullptr) {
  const bool is_newer_material =
      ui::MaterialDesignController::IsNewerMaterialUi();
  SkColor text_color =
      is_newer_material
          ? GetOmniboxColor(OmniboxPart::LOCATION_BAR_TEXT_DEFAULT, tint)
          : GetOmniboxColor(OmniboxPart::LOCATION_BAR_TEXT_DIMMED, tint);
  SkColor background_color =
      GetOmniboxColor(OmniboxPart::LOCATION_BAR_BACKGROUND, tint);
  leading_label_ = CreateLabel(text_color, background_color);

  constexpr int kPaddingInsideBorder = 5;
  // Even though the border is 1 px thick visibly, it takes 1 DIP logically for
  // the non-rounded style.
  const int horizontal_padding = LocationBarView::IsRounded()
                                     ? GetCornerRadius()
                                     : kPaddingInsideBorder + 1;
  chip_label_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets(0, horizontal_padding)));
  chip_label_->SetEnabledColor(text_color);

  bool inverted = color_utils::IsDark(background_color);
  SkColor tab_bg_color =
      inverted ? SK_ColorWHITE : SkColorSetA(text_color, 0x13);
  SkColor tab_border_color = inverted ? SK_ColorWHITE : text_color;
  if (is_newer_material) {
    tab_bg_color = background_color;
    tab_border_color =
        GetOmniboxColor(OmniboxPart::LOCATION_BAR_BUBBLE_OUTLINE, tint);
  }
  chip_label_->SetBackgroundColor(tab_bg_color);

  chip_container_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets(GetLayoutConstant(LOCATION_BAR_BUBBLE_VERTICAL_PADDING), 0)));
  chip_container_->SetBackground(std::make_unique<BackgroundWith1PxBorder>(
      tab_bg_color, tab_border_color));
  chip_container_->AddChildView(chip_label_);
  chip_container_->SetLayoutManager(std::make_unique<views::FillLayout>());
  AddChildView(chip_container_);

  trailing_label_ = CreateLabel(text_color, background_color);

  SetFocusBehavior(FocusBehavior::NEVER);

  // Use leaf alert role so that name is spoken by screen reader, but redundant
  // child label text is not also spoken.
  GetViewAccessibility().OverrideRole(ax::mojom::Role::kGenericContainer);
  GetViewAccessibility().OverrideIsLeaf();
}

KeywordHintView::~KeywordHintView() {}

void KeywordHintView::SetKeyword(const base::string16& keyword,
                                 bool popup_open,
                                 OmniboxTint tint) {
  // In the newer MD style, the KeywordHintView chip background should match the
  // LocationBarView's background, which changes when the popup is open.
  if (ui::MaterialDesignController::IsNewerMaterialUi()) {
    OmniboxPart background_part = popup_open
                                      ? OmniboxPart::RESULTS_BACKGROUND
                                      : OmniboxPart::LOCATION_BAR_BACKGROUND;
    SkColor tab_bg_color = GetOmniboxColor(background_part, tint);
    chip_label_->SetBackgroundColor(tab_bg_color);
    chip_container_->background()->SetNativeControlColor(tab_bg_color);
  }

  // When the virtual keyboard is visible, we show a modified touch UI
  // containing only the chip and no surrounding labels.
  const bool was_touch_ui = leading_label_->text().empty();
  const bool is_touch_ui =
      LocationBarView::IsVirtualKeyboardVisible(GetWidget());
  if (is_touch_ui == was_touch_ui && keyword_ == keyword)
    return;

  keyword_ = keyword;
  if (keyword_.empty())
    return;
  DCHECK(profile_);
  TemplateURLService* url_service =
      TemplateURLServiceFactory::GetForProfile(profile_);
  if (!url_service)
    return;

  bool is_extension_keyword;
  base::string16 short_name(
      url_service->GetKeywordShortName(keyword, &is_extension_keyword));

  if (is_touch_ui) {
    int message_id = is_extension_keyword
                         ? IDS_OMNIBOX_EXTENSION_KEYWORD_HINT_TOUCH
                         : IDS_OMNIBOX_KEYWORD_HINT_TOUCH;
    base::string16 visible_text =
        l10n_util::GetStringFUTF16(message_id, short_name);
    chip_label_->SetText(visible_text);
    SetAccessibleName(visible_text);

    leading_label_->SetText(base::string16());
    trailing_label_->SetText(base::string16());
  } else {
    chip_label_->SetText(l10n_util::GetStringUTF16(IDS_APP_TAB_KEY));

    std::vector<size_t> content_param_offsets;
    int message_id = is_extension_keyword ? IDS_OMNIBOX_EXTENSION_KEYWORD_HINT
                                          : IDS_OMNIBOX_KEYWORD_HINT;
    const base::string16 keyword_hint = l10n_util::GetStringFUTF16(
        message_id, base::string16(), short_name, &content_param_offsets);
    DCHECK_EQ(2U, content_param_offsets.size());
    leading_label_->SetText(
        keyword_hint.substr(0, content_param_offsets.front()));
    trailing_label_->SetText(
        keyword_hint.substr(content_param_offsets.front()));

    const base::string16 tab_key_name =
        l10n_util::GetStringUTF16(IDS_OMNIBOX_KEYWORD_HINT_KEY_ACCNAME);
    SetAccessibleName(leading_label_->text() + tab_key_name +
                      trailing_label_->text());
  }

  // Fire an accessibility event, causing the hint to be spoken.
  NotifyAccessibilityEvent(ax::mojom::Event::kLiveRegionChanged, true);
}

gfx::Insets KeywordHintView::GetInsets() const {
  if (!LocationBarView::IsRounded())
    return gfx::Insets(0,
                       GetLayoutConstant(LOCATION_BAR_ICON_INTERIOR_PADDING));

  // The location bar and keyword hint view chip have rounded ends. Ensure the
  // chip label's corner with the furthest extent from its midpoint is still at
  // least kMinDistanceFromBorder DIPs away from the location bar rounded end.
  constexpr float kMinDistanceFromBorder = 6.f;
  const float radius = GetLayoutConstant(LOCATION_BAR_HEIGHT) / 2.f;
  const float hypotenuse = radius - kMinDistanceFromBorder;
  const float chip_midpoint = chip_container_->height() / 2.f;
  const float extent = std::max(chip_midpoint - chip_label_->y(),
                                chip_label_->bounds().bottom() - chip_midpoint);
  DCHECK_GE(hypotenuse, extent)
      << "LOCATION_BAR_HEIGHT must be tall enough to contain the chip.";
  const float subsumed_width =
      std::sqrt(hypotenuse * hypotenuse - extent * extent);
  const int horizontal_margin = gfx::ToCeiledInt(radius - subsumed_width);
  // This ensures the end of the KeywordHintView doesn't touch the edge of the
  // omnibox, but the padding should be symmetrical, so use it on both sides,
  // collapsing into the horizontal padding used by the previous View.
  const int left_margin =
      horizontal_margin - GetLayoutConstant(LOCATION_BAR_ICON_INTERIOR_PADDING);
  return gfx::Insets(0, std::max(0, left_margin), 0, horizontal_margin);
}

gfx::Size KeywordHintView::GetMinimumSize() const {
  // Height will be ignored by the LocationBarView.
  gfx::Size chip_size = chip_container_->GetPreferredSize();
  chip_size.Enlarge(GetInsets().width(), GetInsets().height());
  return chip_size;
}

const char* KeywordHintView::GetClassName() const {
  return "KeywordHintView";
}

void KeywordHintView::Layout() {
  int chip_width = chip_container_->GetPreferredSize().width();
  bool show_labels = width() - GetInsets().width() > chip_width;
  gfx::Size leading_size(leading_label_->GetPreferredSize());
  leading_label_->SetBounds(GetInsets().left(), 0,
                            show_labels ? leading_size.width() : 0, height());
  const int chip_height = LocationBarView::IsRounded()
                              ? GetLayoutConstant(LOCATION_BAR_ICON_SIZE) +
                                    chip_container_->GetInsets().height()
                              : height();
  const int chip_vertical_padding = std::max(0, height() - chip_height) / 2;
  chip_container_->SetBounds(leading_label_->bounds().right(),
                             chip_vertical_padding, chip_width, chip_height);
  gfx::Size trailing_size(trailing_label_->GetPreferredSize());
  trailing_label_->SetBounds(chip_container_->bounds().right(), 0,
                             show_labels ? trailing_size.width() : 0, height());
}

gfx::Size KeywordHintView::CalculatePreferredSize() const {
  // Height will be ignored by the LocationBarView.
  return gfx::Size(leading_label_->GetPreferredSize().width() +
                       chip_container_->GetPreferredSize().width() +
                       trailing_label_->GetPreferredSize().width() +
                       GetInsets().width(),
                   0);
}

void KeywordHintView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  if (LocationBarView::IsRounded()) {
    const int chip_corner_radius = GetCornerRadius();
    chip_label_->SetBorder(views::CreateEmptyBorder(
        gfx::Insets(GetInsets().top(), chip_corner_radius, GetInsets().bottom(),
                    chip_corner_radius)));
  }
  views::Button::OnBoundsChanged(previous_bounds);
}

views::Label* KeywordHintView::CreateLabel(SkColor text_color,
                                           SkColor background_color) {
  views::Label* label =
      new views::Label(base::string16(), LocationBarView::IsRounded()
                                             ? CONTEXT_OMNIBOX_DECORATION
                                             : CONTEXT_OMNIBOX_PRIMARY);
  label->SetEnabledColor(text_color);
  label->SetBackgroundColor(background_color);
  AddChildView(label);
  return label;
}

int KeywordHintView::GetCornerRadius() const {
  if (!LocationBarView::IsRounded())
    return GetLayoutConstant(LOCATION_BAR_BUBBLE_CORNER_RADIUS);
  return chip_container_->height() / 2;
}
