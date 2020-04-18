// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/save_card_bubble_views.h"

#include <stddef.h>
#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/autofill/dialog_view_ids.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/legal_message_line.h"
#include "components/autofill/core/browser/ui/save_card_bubble_controller.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/blue_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/window/dialog_client_view.h"

namespace autofill {

namespace {

// Dimensions of the Google Pay logo.
const int kGooglePayLogoWidth = 57;
const int kGooglePayLogoHeight = 16;

std::unique_ptr<views::StyledLabel> CreateLegalMessageLineLabel(
    const LegalMessageLine& line,
    views::StyledLabelListener* listener) {
  std::unique_ptr<views::StyledLabel> label(
      new views::StyledLabel(line.text(), listener));
  for (const LegalMessageLine::Link& link : line.links()) {
    label->AddStyleRange(link.range,
                         views::StyledLabel::RangeStyleInfo::CreateForLink());
  }
  return label;
}

}  // namespace

SaveCardBubbleViews::SaveCardBubbleViews(views::View* anchor_view,
                                         const gfx::Point& anchor_point,
                                         content::WebContents* web_contents,
                                         SaveCardBubbleController* controller)
    : LocationBarBubbleDelegateView(anchor_view, anchor_point, web_contents),
      controller_(controller) {
  DCHECK(controller);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::SAVE_CARD);
}

void SaveCardBubbleViews::Show(DisplayReason reason) {
  ShowForReason(reason);
  AssignIdsToDialogClientView();
}

void SaveCardBubbleViews::Hide() {
  // If |controller_| is null, WindowClosing() won't invoke OnBubbleClosed(), so
  // do that here. This will clear out |controller_|'s reference to |this|. Note
  // that WindowClosing() happens only after the _asynchronous_ Close() task
  // posted in CloseBubble() completes, but we need to fix references sooner.
  if (controller_)
    controller_->OnBubbleClosed();
  controller_ = nullptr;
  CloseBubble();
}

views::View* SaveCardBubbleViews::CreateExtraView() {
  if (GetCurrentFlowStep() != LOCAL_SAVE_ONLY_STEP)
    return nullptr;
  // Learn More link is only shown on local save bubble.
  DCHECK(!learn_more_link_);
  learn_more_link_ = new views::Link(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  learn_more_link_->SetUnderline(false);
  learn_more_link_->set_listener(this);
  learn_more_link_->set_id(DialogViewId::LEARN_MORE_LINK);
  return learn_more_link_;
}

views::View* SaveCardBubbleViews::CreateFootnoteView() {
  if (controller_->GetLegalMessageLines().empty())
    return nullptr;

  // Use BoxLayout to provide insets around the label.
  footnote_view_ = new View();
  footnote_view_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  footnote_view_->set_id(DialogViewId::FOOTNOTE_VIEW);

  // Add a StyledLabel for each line of the legal message.
  for (const LegalMessageLine& line : controller_->GetLegalMessageLines()) {
    footnote_view_->AddChildView(
        CreateLegalMessageLineLabel(line, this).release());
  }

  return footnote_view_;
}

bool SaveCardBubbleViews::Accept() {
  if (controller_)
    controller_->OnSaveButton();
  return true;
}

bool SaveCardBubbleViews::Cancel() {
  if (controller_)
    controller_->OnCancelButton();
  return true;
}

bool SaveCardBubbleViews::Close() {
  // If there is a cancel button (non-Material UI), Cancel is logged as a
  // different user action than closing, so override Close() to prevent the
  // superclass' implementation from calling Cancel().
  //
  // Clicking the top-right [X] close button and/or focusing then unfocusing the
  // bubble count as a close action only (without calling Cancel), which means
  // we can't tell the controller to permanently hide the bubble on close,
  // because the user simply dismissed/ignored the bubble; they might want to
  // access the bubble again from the location bar icon. Return true to indicate
  // that the bubble can be closed.
  return true;
}

int SaveCardBubbleViews::GetDialogButtons() const {
  // Material UI has no "No thanks" button in favor of an [X].
  return ui::MaterialDesignController::IsSecondaryUiMaterial()
             ? ui::DIALOG_BUTTON_OK
             : ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;
}

base::string16 SaveCardBubbleViews::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_OK
                                       ? IDS_AUTOFILL_SAVE_CARD_PROMPT_ACCEPT
                                       : IDS_NO_THANKS);
}

gfx::Size SaveCardBubbleViews::CalculatePreferredSize() const {
  const int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
                        DISTANCE_BUBBLE_PREFERRED_WIDTH) -
                    margins().width();
  return gfx::Size(width, GetHeightForWidth(width));
}

bool SaveCardBubbleViews::ShouldShowCloseButton() const {
  // The [X] is shown for Material UI.
  return ui::MaterialDesignController::IsSecondaryUiMaterial();
}

base::string16 SaveCardBubbleViews::GetWindowTitle() const {
  return controller_ ? controller_->GetWindowTitle() : base::string16();
}

gfx::ImageSkia SaveCardBubbleViews::GetWindowIcon() {
  return gfx::ImageSkiaOperations::CreateTiledImage(
      gfx::CreateVectorIcon(kGooglePayLogoWithVerticalSeparatorIcon,
                            gfx::kPlaceholderColor),
      /*x=*/0, /*y=*/0, kGooglePayLogoWidth, kGooglePayLogoHeight);
}

bool SaveCardBubbleViews::ShouldShowWindowIcon() const {
  // We show the window icon (the Google Pay logo) in non-local save scenarios.
  return GetCurrentFlowStep() != LOCAL_SAVE_ONLY_STEP;
}

void SaveCardBubbleViews::WindowClosing() {
  if (controller_) {
    controller_->OnBubbleClosed();
    controller_ = nullptr;
  }
}

void SaveCardBubbleViews::LinkClicked(views::Link* source, int event_flags) {
  DCHECK_EQ(source, learn_more_link_);
  if (controller_)
    controller_->OnLearnMoreClicked();
}

void SaveCardBubbleViews::StyledLabelLinkClicked(views::StyledLabel* label,
                                                 const gfx::Range& range,
                                                 int event_flags) {
  if (!controller_)
    return;

  // Index of |label| within its parent's view hierarchy is the same as the
  // legal message line index. DCHECK this assumption to guard against future
  // layout changes.
  DCHECK_EQ(static_cast<size_t>(label->parent()->child_count()),
            controller_->GetLegalMessageLines().size());

  const auto& links =
      controller_->GetLegalMessageLines()[label->parent()->GetIndexOf(label)]
          .links();
  for (const LegalMessageLine::Link& link : links) {
    if (link.range == range) {
      controller_->OnLegalMessageLinkClicked(link.url);
      return;
    }
  }

  // |range| was not found.
  NOTREACHED();
}

views::View* SaveCardBubbleViews::GetFootnoteViewForTesting() {
  return footnote_view_;
}

SaveCardBubbleViews::~SaveCardBubbleViews() {}

SaveCardBubbleViews::CurrentFlowStep SaveCardBubbleViews::GetCurrentFlowStep()
    const {
  // Local save if no legal messages, upload save otherwise.
  return controller_->GetLegalMessageLines().empty() ? LOCAL_SAVE_ONLY_STEP
                                                     : UPLOAD_SAVE_ONLY_STEP;
}

std::unique_ptr<views::View> SaveCardBubbleViews::CreateMainContentView() {
  std::unique_ptr<views::View> view = std::make_unique<views::View>();
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(),
      provider->GetDistanceMetric(views::DISTANCE_UNRELATED_CONTROL_VERTICAL)));
  view->set_id(GetCurrentFlowStep() == LOCAL_SAVE_ONLY_STEP
                   ? DialogViewId::MAIN_CONTENT_VIEW_LOCAL
                   : DialogViewId::MAIN_CONTENT_VIEW_UPLOAD);

  // If applicable, add the upload explanation label.  Appears above the card
  // info.
  base::string16 explanation = controller_->GetExplanatoryMessage();
  if (!explanation.empty()) {
    auto* explanation_label = new views::Label(explanation);
    explanation_label->SetMultiLine(true);
    explanation_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    view->AddChildView(explanation_label);
  }

  // Add the card type icon, last four digits and expiration date.
  auto* description_view = new views::View();
  description_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, gfx::Insets(),
      provider->GetDistanceMetric(views::DISTANCE_RELATED_BUTTON_HORIZONTAL)));
  view->AddChildView(description_view);

  const CreditCard& card = controller_->GetCard();
  auto* card_type_icon = new views::ImageView();
  card_type_icon->SetImage(
      ui::ResourceBundle::GetSharedInstance()
          .GetImageNamed(CreditCard::IconResourceId(card.network()))
          .AsImageSkia());
  card_type_icon->SetTooltipText(card.NetworkForDisplay());
  card_type_icon->SetBorder(
      views::CreateSolidBorder(1, SkColorSetA(SK_ColorBLACK, 10)));
  description_view->AddChildView(card_type_icon);

  description_view->AddChildView(
      new views::Label(card.NetworkAndLastFourDigits()));
  description_view->AddChildView(
      new views::Label(card.AbbreviatedExpirationDateForDisplay()));

  return view;
}

void SaveCardBubbleViews::AssignIdsToDialogClientView() {
  auto* ok_button = GetDialogClientView()->ok_button();
  if (ok_button)
    ok_button->set_id(DialogViewId::OK_BUTTON);
  auto* cancel_button = GetDialogClientView()->cancel_button();
  if (cancel_button)
    cancel_button->set_id(DialogViewId::CANCEL_BUTTON);
}

void SaveCardBubbleViews::Init() {
  SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  AddChildView(CreateMainContentView().release());
}

}  // namespace autofill
