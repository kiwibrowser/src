// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/password_generation_popup_view_views.h"

#include "base/macros.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/autofill/password_generation_popup_controller.h"
#include "chrome/browser/ui/autofill/popup_constants.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/browser/ui/views_mode_controller.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace autofill {

namespace {

// The amount of whitespace that is present when there is no padding. Used
// to get the proper spacing in the help section.
const int kHelpVerticalOffset = 5;

// Wrapper around just the text portions of the generation UI (password and
// prompting text).
class PasswordTextBox : public views::View {
 public:
  PasswordTextBox() {}
  ~PasswordTextBox() override {}

  // |suggestion_text| prompts the user to select the password,
  // |generated_password| is the generated password.
  void Init(const base::string16& suggestion_text,
            const base::string16& generated_password) {
    auto box_layout = std::make_unique<views::BoxLayout>(
        views::BoxLayout::kVertical, gfx::Insets(12, 0), 5);
    box_layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
    SetLayoutManager(std::move(box_layout));

    views::Label* suggestion_label = new views::Label(
        suggestion_text, CONTEXT_DEPRECATED_SMALL, STYLE_EMPHASIZED);
    suggestion_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    suggestion_label->SetEnabledColor(
        PasswordGenerationPopupView::kPasswordTextColor);
    AddChildView(suggestion_label);

    views::Label* password_label =
        new views::Label(generated_password, CONTEXT_DEPRECATED_SMALL);
    password_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    password_label->SetEnabledColor(
        PasswordGenerationPopupView::kPasswordTextColor);
    AddChildView(password_label);
  }

  // views::View:
  bool CanProcessEventsWithinSubtree() const override {
    // Send events to the parent view for handling.
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordTextBox);
};

}  // namespace

// Class that shows the generated password and associated UI (currently a key
// image and some explanatory text).
class PasswordGenerationPopupViewViews::PasswordBox : public views::View {
 public:
  PasswordBox() {}
  ~PasswordBox() override {}

  // |password| is the generated password, |suggestion| is the text prompting
  // the user to select the password.
  void Init(const base::string16& password, const base::string16& suggestion) {
    auto box_layout = std::make_unique<views::BoxLayout>(
        views::BoxLayout::kHorizontal,
        gfx::Insets(0, PasswordGenerationPopupController::kHorizontalPadding),
        PasswordGenerationPopupController::kHorizontalPadding);
    box_layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
    SetLayoutManager(std::move(box_layout));

    views::ImageView* key_image = new views::ImageView();
    key_image->SetImage(
        gfx::CreateVectorIcon(kKeyIcon, 16, gfx::kChromeIconGrey));
    AddChildView(key_image);

    PasswordTextBox* password_text_box = new PasswordTextBox();
    password_text_box->Init(suggestion, password);
    AddChildView(password_text_box);
  }

  // views::View:
  bool CanProcessEventsWithinSubtree() const override {
    // Send events to the parent view for handling.
    return false;
  }

 private:

  DISALLOW_COPY_AND_ASSIGN(PasswordBox);
};

PasswordGenerationPopupViewViews::PasswordGenerationPopupViewViews(
    PasswordGenerationPopupController* controller,
    views::Widget* parent_widget)
    : AutofillPopupBaseView(controller, parent_widget),
      password_view_(NULL),
      controller_(controller) {
  if (controller_->display_password())
    CreatePasswordView();

  help_label_ = new views::StyledLabel(controller_->HelpText(), this);

  // With MD, the line spacing comes from the TextContext instead and this is
  // unnecessary.
  if (!ui::MaterialDesignController::IsSecondaryUiMaterial())
    help_label_->SetLineHeight(20);

  help_label_->SetTextContext(CONTEXT_DEPRECATED_SMALL);
  help_label_->SetDefaultTextStyle(STYLE_HINT);

  views::StyledLabel::RangeStyleInfo link_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink();
  link_style.disable_line_wrapping = false;
  help_label_->AddStyleRange(controller_->HelpTextLinkRange(), link_style);

  help_label_->SetBackground(
      views::CreateSolidBackground(kExplanatoryTextBackgroundColor));
  help_label_->SetBorder(views::CreateEmptyBorder(
      PasswordGenerationPopupController::kHelpVerticalPadding -
          kHelpVerticalOffset,
      PasswordGenerationPopupController::kHorizontalPadding,
      PasswordGenerationPopupController::kHelpVerticalPadding -
          kHelpVerticalOffset,
      PasswordGenerationPopupController::kHorizontalPadding));
  AddChildView(help_label_);

  SetBackground(views::CreateThemedSolidBackground(
      this, ui::NativeTheme::kColorId_ResultsTableNormalBackground));
}

PasswordGenerationPopupViewViews::~PasswordGenerationPopupViewViews() {}

void PasswordGenerationPopupViewViews::CreatePasswordView() {
  if (password_view_)
    return;

  password_view_ = new PasswordBox();
  password_view_->Init(controller_->password(), controller_->SuggestedText());
  password_view_->SetPosition(gfx::Point());
  password_view_->SizeToPreferredSize();
  AddChildView(password_view_);
}

gfx::Size PasswordGenerationPopupViewViews::GetPreferredSizeOfPasswordView() {
  int width = controller_->GetMinimumWidth();
  if (password_view_)
    width = std::max(width, password_view_->GetMinimumSize().width());
  int height = help_label_->GetHeightForWidth(width);
  if (controller_->display_password()) {
    // Add divider height as well.
    height +=
        PasswordGenerationPopupController::kPopupPasswordSectionHeight + 1;
  }
  return gfx::Size(width, height);
}

void PasswordGenerationPopupViewViews::Show() {
  DoShow();
}

void PasswordGenerationPopupViewViews::Hide() {
  // The controller is no longer valid after it hides us.
  controller_ = NULL;

  DoHide();
}

void PasswordGenerationPopupViewViews::UpdateBoundsAndRedrawPopup() {
  DoUpdateBoundsAndRedrawPopup();
}

void PasswordGenerationPopupViewViews::PasswordSelectionUpdated() {
  if (!password_view_)
    return;

  if (controller_->password_selected())
    NotifyAccessibilityEvent(ax::mojom::Event::kSelection, true);

  password_view_->SetBackground(views::CreateThemedSolidBackground(
      password_view_,
      controller_->password_selected()
          ? ui::NativeTheme::kColorId_ResultsTableHoveredBackground
          : ui::NativeTheme::kColorId_ResultsTableNormalBackground));
}

void PasswordGenerationPopupViewViews::Layout() {
  // Need to leave room for the border.
  int y = 0;
  int popup_width = bounds().width();
  if (controller_->display_password()) {
    // Currently the UI can change from not offering a password to offering
    // a password (e.g. the user is editing a generated password and deletes
    // it), but it can't change the other way around.
    CreatePasswordView();
    password_view_->SetBounds(
        0, 0, popup_width,
        PasswordGenerationPopupController::kPopupPasswordSectionHeight);
    divider_bounds_ =
        gfx::Rect(0, password_view_->bounds().bottom(), popup_width, 1);
    y = divider_bounds_.bottom();
  }

  help_label_->SetBounds(0, y, popup_width,
                         help_label_->GetHeightForWidth(popup_width));
}

void PasswordGenerationPopupViewViews::OnPaint(gfx::Canvas* canvas) {
  if (!controller_)
    return;

  // Draw border and background.
  views::View::OnPaint(canvas);

  // Divider line needs to be drawn after OnPaint() otherwise the background
  // will overwrite the divider.
  if (password_view_)
    canvas->FillRect(divider_bounds_, kDividerColor);
}

void PasswordGenerationPopupViewViews::StyledLabelLinkClicked(
    views::StyledLabel* label,
    const gfx::Range& range,
    int event_flags) {
  controller_->OnSavedPasswordsLinkClicked();
}

bool PasswordGenerationPopupViewViews::IsPointInPasswordBounds(
    const gfx::Point& point) {
  if (password_view_)
    return password_view_->bounds().Contains(point);
  return false;
}

PasswordGenerationPopupView* PasswordGenerationPopupView::Create(
    PasswordGenerationPopupController* controller) {
#if defined(OS_MACOSX)
  if (views_mode_controller::IsViewsBrowserCocoa())
    return CreateCocoa(controller);
#endif
  views::Widget* observing_widget =
      views::Widget::GetTopLevelWidgetForNativeView(
          controller->container_view());

  // If the top level widget can't be found, cancel the popup since we can't
  // fully set it up.
  if (!observing_widget)
    return NULL;

  return new PasswordGenerationPopupViewViews(controller, observing_widget);
}

void PasswordGenerationPopupViewViews::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->SetName(controller_->SuggestedText());
  node_data->role = ax::mojom::Role::kMenuItem;
}

}  // namespace autofill
