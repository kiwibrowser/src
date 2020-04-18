// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/payments/payment_method_view_controller.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_forward.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/browser/ui/views/payments/payment_request_dialog_view.h"
#include "chrome/browser/ui/views/payments/payment_request_dialog_view_ids.h"
#include "chrome/browser/ui/views/payments/payment_request_row_view.h"
#include "chrome/browser/ui/views/payments/payment_request_views_util.h"
#include "components/payments/content/payment_request_state.h"
#include "components/payments/core/autofill_payment_instrument.h"
#include "components/payments/core/payment_instrument.h"
#include "components/payments/core/strings_util.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/vector_icons.h"

namespace payments {

namespace {

constexpr int kFirstTagValue =
    static_cast<int>(PaymentRequestCommonTags::PAYMENT_REQUEST_COMMON_TAG_MAX);

enum class PaymentMethodViewControllerTags : int {
  // The tag for the button that triggers the "add card" flow. Starts at
  // |kFirstTagValue| not to conflict with tags common to all views.
  ADD_CREDIT_CARD_BUTTON = kFirstTagValue,
  // This value is passed to inner views so they can use it as a starting tag.
  MAX_TAG,
};

class PaymentMethodListItem : public PaymentRequestItemList::Item {
 public:
  // Does not take ownership of |instrument|, which should not be null and
  // should outlive this object. |list| is the PaymentRequestItemList object
  // that will own this.
  PaymentMethodListItem(PaymentInstrument* instrument,
                        PaymentRequestSpec* spec,
                        PaymentRequestState* state,
                        PaymentRequestItemList* list,
                        PaymentRequestDialogView* dialog,
                        bool selected)
      : PaymentRequestItemList::Item(spec,
                                     state,
                                     list,
                                     selected,
                                     /*clickable=*/true,
                                     /*show_edit_button=*/instrument->type() ==
                                         PaymentInstrument::Type::AUTOFILL),
        instrument_(instrument),
        dialog_(dialog) {
    Init();
  }
  ~PaymentMethodListItem() override {}

 private:
  void ShowEditor() {
    switch (instrument_->type()) {
      case PaymentInstrument::Type::AUTOFILL:
        // Since we are a list item, we only care about the on_edited callback.
        dialog_->ShowCreditCardEditor(
            BackNavigationType::kPaymentSheet,
            static_cast<int>(PaymentMethodViewControllerTags::MAX_TAG),
            /*on_edited=*/
            base::BindOnce(&PaymentRequestState::SetSelectedInstrument,
                           base::Unretained(state()), instrument_),
            /*on_added=*/
            base::OnceCallback<void(const autofill::CreditCard&)>(),
            static_cast<AutofillPaymentInstrument*>(instrument_)
                ->credit_card());
        return;
      case PaymentInstrument::Type::NATIVE_MOBILE_APP:
      case PaymentInstrument::Type::SERVICE_WORKER_APP:
        // We cannot edit a native mobile app instrument and service worker
        // based payment instrument.
        return;
    }
    NOTREACHED();
  }

  // PaymentRequestItemList::Item:
  std::unique_ptr<views::View> CreateExtraView() override {
    std::unique_ptr<views::ImageView> icon_view = CreateInstrumentIconView(
        instrument_->icon_resource_id(), instrument_->icon_image_skia(),
        instrument_->GetLabel());
    return icon_view;
  }

  std::unique_ptr<views::View> CreateContentView(
      base::string16* accessible_content) override {
    DCHECK(accessible_content);
    auto card_info_container = std::make_unique<views::View>();
    card_info_container->set_can_process_events_within_subtree(false);

    auto box_layout = std::make_unique<views::BoxLayout>(
        views::BoxLayout::kVertical,
        gfx::Insets(kPaymentRequestRowVerticalInsets, 0));
    box_layout->set_cross_axis_alignment(
        views::BoxLayout::CROSS_AXIS_ALIGNMENT_START);
    card_info_container->SetLayoutManager(std::move(box_layout));

    base::string16 label = instrument_->GetLabel();
    if (!label.empty())
      card_info_container->AddChildView(new views::Label(label));
    base::string16 sublabel = instrument_->GetSublabel();
    if (!sublabel.empty())
      card_info_container->AddChildView(new views::Label(sublabel));
    base::string16 missing_info;
    if (!instrument_->IsCompleteForPayment()) {
      missing_info = instrument_->GetMissingInfoLabel();
      auto missing_info_label = std::make_unique<views::Label>(
          missing_info, CONTEXT_DEPRECATED_SMALL);
      missing_info_label->SetEnabledColor(
          missing_info_label->GetNativeTheme()->GetSystemColor(
              ui::NativeTheme::kColorId_LinkEnabled));
      card_info_container->AddChildView(missing_info_label.release());
    }

    *accessible_content = l10n_util::GetStringFUTF16(
        IDS_PAYMENTS_PROFILE_LABELS_ACCESSIBLE_FORMAT, label, sublabel,
        missing_info);

    return card_info_container;
  }

  void SelectedStateChanged() override {
    if (selected()) {
      state()->SetSelectedInstrument(instrument_);
      dialog_->GoBack();
    }
  }

  base::string16 GetNameForDataType() override {
    return l10n_util::GetStringUTF16(IDS_PAYMENTS_METHOD_OF_PAYMENT_LABEL);
  }

  bool CanBeSelected() override {
    // If an instrument can't be selected, PerformSelectionFallback is called,
    // where the instrument can be made complete.
    return instrument_->IsCompleteForPayment();
  }

  void PerformSelectionFallback() override { ShowEditor(); }

  void EditButtonPressed() override { ShowEditor(); }

  PaymentInstrument* instrument_;
  PaymentRequestDialogView* dialog_;

  DISALLOW_COPY_AND_ASSIGN(PaymentMethodListItem);
};

std::unique_ptr<views::View> CreateHeaderView(const base::string16& text) {
  auto label = std::make_unique<views::Label>(text);
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetBorder(views::CreateEmptyBorder(
      kPaymentRequestRowVerticalInsets, kPaymentRequestRowHorizontalInsets, 0,
      kPaymentRequestRowHorizontalInsets));
  return label;
}

}  // namespace

PaymentMethodViewController::PaymentMethodViewController(
    PaymentRequestSpec* spec,
    PaymentRequestState* state,
    PaymentRequestDialogView* dialog)
    : PaymentRequestSheetController(spec, state, dialog),
      payment_method_list_(dialog) {
  const std::vector<std::unique_ptr<PaymentInstrument>>& available_instruments =
      state->available_instruments();
  for (const auto& instrument : available_instruments) {
    auto item = std::make_unique<PaymentMethodListItem>(
        instrument.get(), spec, state, &payment_method_list_, dialog,
        instrument.get() == state->selected_instrument());
    payment_method_list_.AddItem(std::move(item));
  }
}

PaymentMethodViewController::~PaymentMethodViewController() {}

base::string16 PaymentMethodViewController::GetSheetTitle() {
  return l10n_util::GetStringUTF16(
      IDS_PAYMENT_REQUEST_PAYMENT_METHOD_SECTION_NAME);
}

void PaymentMethodViewController::FillContentView(views::View* content_view) {
  auto layout = std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical);
  layout->set_main_axis_alignment(views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_STRETCH);
  content_view->SetLayoutManager(std::move(layout));

  base::string16 sub_header =
      GetCardTypesAreAcceptedText(spec()->supported_card_types_set());
  if (!sub_header.empty())
    content_view->AddChildView(CreateHeaderView(sub_header).release());

  std::unique_ptr<views::View> list_view =
      payment_method_list_.CreateListView();
  list_view->set_id(
      static_cast<int>(DialogViewID::PAYMENT_METHOD_SHEET_LIST_VIEW));
  content_view->AddChildView(list_view.release());
}

std::unique_ptr<views::View>
PaymentMethodViewController::CreateExtraFooterView() {
  if (!spec()->supports_basic_card())
    return nullptr;

  auto extra_view = std::make_unique<views::View>();

  extra_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, gfx::Insets(),
      kPaymentRequestButtonSpacing));

  views::LabelButton* button = views::MdTextButton::CreateSecondaryUiButton(
      this, l10n_util::GetStringUTF16(IDS_PAYMENTS_ADD_CARD));
  button->set_tag(static_cast<int>(
      PaymentMethodViewControllerTags::ADD_CREDIT_CARD_BUTTON));
  button->set_id(
      static_cast<int>(DialogViewID::PAYMENT_METHOD_ADD_CARD_BUTTON));
  button->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  extra_view->AddChildView(button);

  return extra_view;
}

void PaymentMethodViewController::ButtonPressed(views::Button* sender,
                                                const ui::Event& event) {
  switch (sender->tag()) {
    case static_cast<int>(
        PaymentMethodViewControllerTags::ADD_CREDIT_CARD_BUTTON):
      // Only provide the |on_added| callback, in response to this button.
      dialog()->ShowCreditCardEditor(
          BackNavigationType::kPaymentSheet,
          static_cast<int>(PaymentMethodViewControllerTags::MAX_TAG),
          /*on_edited=*/base::OnceClosure(),
          /*on_added=*/
          base::BindOnce(&PaymentRequestState::AddAutofillPaymentInstrument,
                         base::Unretained(state()), /*selected=*/true),
          /*credit_card=*/nullptr);
      break;
    default:
      PaymentRequestSheetController::ButtonPressed(sender, event);
      break;
  }
}

}  // namespace payments
