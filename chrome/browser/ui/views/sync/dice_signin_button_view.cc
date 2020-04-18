// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/sync/dice_signin_button_view.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"

namespace {

constexpr int kDropDownArrowIconSize = 12;
constexpr int kDividerVerticalPadding = 10;
constexpr int kButtonCornerRadius = 2;
constexpr SkColor kDividerColor = SK_ColorWHITE;

base::string16 GetButtonTitleForAccount(const AccountInfo& account) {
  if (!account.given_name.empty()) {
    return l10n_util::GetStringFUTF16(
        IDS_PROFILES_DICE_SIGNIN_FIRST_ACCOUNT_BUTTON,
        base::UTF8ToUTF16(account.given_name));
  }
  if (!account.full_name.empty()) {
    return l10n_util::GetStringFUTF16(
        IDS_PROFILES_DICE_SIGNIN_FIRST_ACCOUNT_BUTTON,
        base::UTF8ToUTF16(account.full_name));
  }
  return l10n_util::GetStringUTF16(
      IDS_PROFILES_DICE_SIGNIN_FIRST_ACCOUNT_BUTTON_NO_NAME);
}

// Sizes |image| to 40x40, adds a white background in case it is transparent and
// shapes it circular.
gfx::ImageSkia PrepareAvatarImage(const gfx::Image& image) {
  // Add white background.
  SkBitmap mask;
  mask.allocN32Pixels(image.Width(), image.Height());
  mask.eraseARGB(255, 255, 255, 0);
  SkBitmap opaque_bitmap = SkBitmapOperations::CreateButtonBackground(
      SK_ColorWHITE, image.AsBitmap(), mask);
  // Size and shape.
  return profiles::GetSizedAvatarIcon(
             gfx::Image::CreateFrom1xBitmap(opaque_bitmap), true, 40, 40,
             profiles::SHAPE_CIRCLE)
      .AsImageSkia();
}

}  // namespace

DiceSigninButtonView::DiceSigninButtonView(
    views::ButtonListener* button_listener,
    bool prominent)
    : account_(base::nullopt) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  // Regular MD text button when there is no account.
  views::MdTextButton* button = views::MdTextButton::Create(
      button_listener,
      l10n_util::GetStringUTF16(IDS_PROFILES_DICE_SIGNIN_BUTTON),
      views::style::CONTEXT_BUTTON);
  button->SetProminent(prominent);
  AddChildView(button);
  signin_button_ = button;
}

DiceSigninButtonView::DiceSigninButtonView(
    const AccountInfo& account,
    const gfx::Image& account_icon,
    views::ButtonListener* button_listener,
    bool show_drop_down_arrow)
    : account_(account) {
  views::GridLayout* grid_layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));
  views::ColumnSet* columns = grid_layout->AddColumnSet(0);
  grid_layout->StartRow(0, 0);

  // Add a stretching column for the account button.
  columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                     views::GridLayout::USE_PREF, 0, 0);
  auto icon_view = std::make_unique<views::ImageView>();
  icon_view->SetImage(PrepareAvatarImage(account_icon));
  HoverButton* hover_button = new HoverButton(
      button_listener, std::move(icon_view), GetButtonTitleForAccount(account),
      base::ASCIIToUTF16(account_->email));
  hover_button->SetStyle(HoverButton::STYLE_PROMINENT);
  signin_button_ = hover_button;
  grid_layout->AddView(signin_button_);

  if (!show_drop_down_arrow)
    return;

  // Add a non-stretching column for the the drop down arrow.
  columns->AddColumn(views::GridLayout::TRAILING, views::GridLayout::FILL, 0,
                     views::GridLayout::USE_PREF, 0, 0);
  arrow_ = new HoverButton(
      button_listener,
      gfx::CreateVectorIcon(kSigninButtonDropDownArrowIcon,
                            kDropDownArrowIconSize, SK_ColorWHITE),
      base::string16());
  arrow_->SetTooltipText(l10n_util::GetStringUTF16(
      IDS_PROFILES_DICE_SIGNIN_WITH_ANOTHER_ACCOUNT_BUTTON));
  grid_layout->AddView(arrow_);

  // Make the background of the entire view match the prominent
  // |signin_button_|.
  SetBackground(
      views::CreateSolidBackground(signin_button_->background()->get_color()));
  // Make the entire view (including the arrow) highlighted when the
  // |signin_button_| is hovered.
  hover_button->SetHighlightingView(this);
}

DiceSigninButtonView::~DiceSigninButtonView() = default;

void DiceSigninButtonView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);
  if (arrow_) {
    // Draw divider between |signin_button_| and |arrow_|.
    gfx::Rect bounds = arrow_->GetMirroredBounds();
    bounds.Inset(gfx::Insets(kDividerVerticalPadding, 0));
    if (base::i18n::IsRTL())
      bounds.Inset(bounds.width() - 1, 0, 0, 0);
    canvas->DrawLine(bounds.origin(), bounds.bottom_left(), kDividerColor);
  }
}

void DiceSigninButtonView::Layout() {
  views::View::Layout();
  // Clip the view to make the corners rounded and to remove the border
  // background in case a parent view wants to set a transparent border.
  gfx::Path path;
  path.addRoundRect(gfx::RectToSkRect(GetContentsBounds()), kButtonCornerRadius,
                    kButtonCornerRadius);
  set_clip_path(path);
}
