// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/components/shortcut_viewer/views/ksv_search_box_view.h"

#include "ash/components/shortcut_viewer/vector_icons/vector_icons.h"
#include "ash/components/strings/grit/ash_components_strings.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/search_box/search_box_view_delegate.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/controls/textfield/textfield.h"

namespace keyboard_shortcut_viewer {

namespace {

constexpr SkColor kDefaultSearchBoxBackgroundColor =
    SkColorSetARGB(0x28, 0x80, 0x86, 0x8B);

constexpr int kIconSize = 20;

// Border corner radius of the search box.
constexpr int kBorderCornerRadius = 32;

}  // namespace

KSVSearchBoxView::KSVSearchBoxView(search_box::SearchBoxViewDelegate* delegate)
    : search_box::SearchBoxViewBase(delegate) {
  SetSearchBoxBackgroundCornerRadius(kBorderCornerRadius);
  SetSearchBoxBackgroundColor(kDefaultSearchBoxBackgroundColor);
  search_box()->SetBackgroundColor(SK_ColorTRANSPARENT);
  constexpr SkColor kSearchBoxTextColor =
      SkColorSetARGB(0xFF, 0x3C, 0x40, 0x43);
  search_box()->SetColor(kSearchBoxTextColor);
  search_box()->set_placeholder_text_color(kSearchBoxTextColor);
  search_box()->set_placeholder_text_draw_flags(gfx::Canvas::TEXT_ALIGN_CENTER);
  const base::string16 search_box_name(
      l10n_util::GetStringUTF16(IDS_KSV_SEARCH_BOX_ACCESSIBILITY_NAME));
  search_box()->set_placeholder_text(search_box_name);
  search_box()->SetAccessibleName(search_box_name);

  constexpr SkColor kSearchBarIconColor =
      SkColorSetARGB(0xFF, 0x3C, 0x40, 0x43);
  SetSearchIconImage(
      gfx::CreateVectorIcon(kKsvSearchBarIcon, kSearchBarIconColor));
}

gfx::Size KSVSearchBoxView::CalculatePreferredSize() const {
  return gfx::Size(740, 32);
}

void KSVSearchBoxView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kSearchBox;
  node_data->SetValue(accessible_value_);
}

void KSVSearchBoxView::OnKeyEvent(ui::KeyEvent* event) {
  const ui::KeyboardCode key = event->key_code();
  const bool is_escape_key = (key == ui::VKEY_ESCAPE);
  if (!is_escape_key && key != ui::VKEY_BROWSER_BACK)
    return;

  event->SetHandled();
  // |VKEY_BROWSER_BACK| will only clear all the text.
  ClearSearch();
  // |VKEY_ESCAPE| will clear text and exit search mode directly.
  if (is_escape_key)
    SetSearchBoxActive(false);
}

void KSVSearchBoxView::ButtonPressed(views::Button* sender,
                                     const ui::Event& event) {
  // Focus on the search box text field after clicking close button.
  if (close_button() && sender == close_button())
    search_box()->RequestFocus();
  SearchBoxViewBase::ButtonPressed(sender, event);
}

void KSVSearchBoxView::SetAccessibleValue(const base::string16& value) {
  accessible_value_ = value;
  NotifyAccessibilityEvent(ax::mojom::Event::kValueChanged, true);
}

void KSVSearchBoxView::UpdateBackgroundColor(SkColor color) {
  SetSearchBoxBackgroundColor(color);
}

void KSVSearchBoxView::UpdateSearchBoxBorder() {
  // TODO(wutao): Rename this function or create another function in base class.
  // It updates many things in addition to the border.
  if (!search_box()->HasFocus() && search_box()->text().empty())
    SetSearchBoxActive(false);

  constexpr int kBorderThichness = 2;
  constexpr SkColor kActiveBorderColor = SkColorSetARGB(0x7F, 0x1A, 0x73, 0xE8);
  constexpr SkColor kActiveFillColor = SkColorSetARGB(0xFF, 0xF1, 0xF3, 0xF4);

  if (search_box()->HasFocus() || is_search_box_active()) {
    SetBorder(views::CreateRoundedRectBorder(
        kBorderThichness, kBorderCornerRadius, kActiveBorderColor));
    SetSearchBoxBackgroundColor(kActiveFillColor);
    return;
  }
  SetBorder(views::CreateRoundedRectBorder(
      kBorderThichness, kBorderCornerRadius, SK_ColorTRANSPARENT));
  SetSearchBoxBackgroundColor(kDefaultSearchBoxBackgroundColor);
}

void KSVSearchBoxView::SetupCloseButton() {
  constexpr SkColor kCloseIconColor = SkColorSetARGB(0xFF, 0x80, 0x86, 0x8B);
  views::ImageButton* close = close_button();
  close->SetImage(views::ImageButton::STATE_NORMAL,
                  gfx::CreateVectorIcon(kKsvSearchCloseIcon, kCloseIconColor));
  close->SetSize(gfx::Size(kIconSize, kIconSize));
  close->SetImageAlignment(views::ImageButton::ALIGN_CENTER,
                           views::ImageButton::ALIGN_MIDDLE);
  const base::string16 close_button_label(
      l10n_util::GetStringUTF16(IDS_KSV_CLEAR_SEARCHBOX_ACCESSIBILITY_NAME));
  close->SetAccessibleName(close_button_label);
  close->SetTooltipText(close_button_label);
  close->SetVisible(false);
}

void KSVSearchBoxView::SetupBackButton() {
  constexpr SkColor kBackIconColor = SkColorSetARGB(0xFF, 0x42, 0x85, 0xF4);
  views::ImageButton* back = back_button();
  back->SetImage(views::ImageButton::STATE_NORMAL,
                 gfx::CreateVectorIcon(kKsvSearchBackIcon, kBackIconColor));
  back->SetSize(gfx::Size(kIconSize, kIconSize));
  back->SetImageAlignment(views::ImageButton::ALIGN_CENTER,
                          views::ImageButton::ALIGN_MIDDLE);
  const base::string16 back_button_label(
      l10n_util::GetStringUTF16(IDS_KSV_BACK_ACCESSIBILITY_NAME));
  back->SetAccessibleName(back_button_label);
  back->SetTooltipText(back_button_label);
  back->SetVisible(false);
}

}  // namespace keyboard_shortcut_viewer
