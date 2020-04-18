// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/assistant/ui/dialog_plate/dialog_plate.h"

#include <memory>

#include "ash/assistant/assistant_controller.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

namespace {

// Appearance.
constexpr SkColor kBorderColor = SkColorSetA(SK_ColorBLACK, 0x1F);
constexpr int kBorderSizeDip = 1;
constexpr int kPaddingHorizontalDip = 14;
constexpr int kPaddingVerticalDip = 8;
constexpr int kPreferredHeightDip = 48;
constexpr int kSpacingDip = 8;

// Typography.
constexpr SkColor kTextColorHint = SkColorSetA(SK_ColorBLACK, 0x42);
constexpr SkColor kTextColorPrimary = SkColorSetA(SK_ColorBLACK, 0xDE);

// TODO(b/77638210): Replace with localized resource strings.
constexpr char kHint[] = "Type a message";

}  // namespace

// DialogPlate -----------------------------------------------------------------

DialogPlate::DialogPlate(AssistantController* assistant_controller)
    : assistant_controller_(assistant_controller),
      textfield_(new views::Textfield()),
      action_view_(new ActionView(assistant_controller, this)) {
  InitLayout();

  // The Assistant controller indirectly owns the view hierarchy to which
  // DialogPlate belongs so is guaranteed to outlive it.
  assistant_controller_->AddInteractionModelObserver(this);
}

DialogPlate::~DialogPlate() {
  assistant_controller_->RemoveInteractionModelObserver(this);
}

gfx::Size DialogPlate::CalculatePreferredSize() const {
  return gfx::Size(INT_MAX, GetHeightForWidth(INT_MAX));
}

int DialogPlate::GetHeightForWidth(int width) const {
  return kPreferredHeightDip;
}

void DialogPlate::InitLayout() {
  SetBackground(views::CreateSolidBackground(SK_ColorWHITE));
  SetBorder(
      views::CreateSolidSidedBorder(kBorderSizeDip, 0, 0, 0, kBorderColor));

  views::BoxLayout* layout =
      SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          gfx::Insets(kPaddingVerticalDip, kPaddingHorizontalDip),
          kSpacingDip));

  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::CROSS_AXIS_ALIGNMENT_CENTER);

  gfx::FontList font_list =
      views::Textfield::GetDefaultFontList().DeriveWithSizeDelta(4);

  // Textfield.
  textfield_->SetBackgroundColor(SK_ColorTRANSPARENT);
  textfield_->SetBorder(views::NullBorder());
  textfield_->set_controller(this);
  textfield_->SetFontList(font_list);
  textfield_->set_placeholder_font_list(font_list);
  textfield_->set_placeholder_text(base::UTF8ToUTF16(kHint));
  textfield_->set_placeholder_text_color(kTextColorHint);
  textfield_->SetTextColor(kTextColorPrimary);
  AddChildView(textfield_);

  layout->SetFlexForView(textfield_, 1);

  // Action.
  AddChildView(action_view_);
}

void DialogPlate::OnInteractionStateChanged(
    InteractionState interaction_state) {
  // When the Assistant interaction becomes inactive we need to clear the
  // dialog plate so that text does not persist across Assistant entries.
  if (interaction_state == InteractionState::kInactive)
    textfield_->SetText(base::string16());
}

void DialogPlate::OnActionPressed() {
  const base::StringPiece16& trimmed_text =
      base::TrimWhitespace(textfield_->text(), base::TrimPositions::TRIM_ALL);

  assistant_controller_->OnDialogPlateActionPressed(
      base::UTF16ToUTF8(trimmed_text));

  textfield_->SetText(base::string16());
}

void DialogPlate::ContentsChanged(views::Textfield* textfield,
                                  const base::string16& new_contents) {
  assistant_controller_->OnDialogPlateContentsChanged(
      base::UTF16ToUTF8(new_contents));
}

bool DialogPlate::HandleKeyEvent(views::Textfield* textfield,
                                 const ui::KeyEvent& key_event) {
  if (key_event.key_code() != ui::KeyboardCode::VKEY_RETURN)
    return false;

  if (key_event.type() != ui::EventType::ET_KEY_PRESSED)
    return false;

  const base::string16& text = textfield_->text();

  // We filter out committing an empty string here but do allow committing a
  // whitespace only string. If the user commits a whitespace only string, we
  // want to be able to show a helpful message. This is taken care of in
  // AssistantController's handling of the commit event.
  if (text.empty())
    return false;

  const base::StringPiece16& trimmed_text =
      base::TrimWhitespace(text, base::TrimPositions::TRIM_ALL);

  assistant_controller_->OnDialogPlateContentsCommitted(
      base::UTF16ToUTF8(trimmed_text));

  textfield_->SetText(base::string16());

  return true;
}

void DialogPlate::RequestFocus() {
  textfield_->RequestFocus();
}

}  // namespace ash
