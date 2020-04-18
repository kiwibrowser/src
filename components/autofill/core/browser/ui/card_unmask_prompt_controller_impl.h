// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_UI_CARD_UNMASK_PROMPT_CONTROLLER_IMPL_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_UI_CARD_UNMASK_PROMPT_CONTROLLER_IMPL_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/card_unmask_delegate.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/ui/card_unmask_prompt_controller.h"

namespace autofill {

class CardUnmaskPromptView;

class CardUnmaskPromptControllerImpl : public CardUnmaskPromptController {
 public:
  CardUnmaskPromptControllerImpl(
      PrefService* pref_service,
      bool is_off_the_record);
  virtual ~CardUnmaskPromptControllerImpl();

  // Functions called by ChromeAutofillClient.
  void ShowPrompt(CardUnmaskPromptView* view,
                  const CreditCard& card,
                  AutofillClient::UnmaskCardReason reason,
                  base::WeakPtr<CardUnmaskDelegate> delegate);
  // The CVC the user entered went through validation.
  void OnVerificationResult(AutofillClient::PaymentsRpcResult result);

  // CardUnmaskPromptController implementation.
  void OnUnmaskDialogClosed() override;
  void OnUnmaskResponse(const base::string16& cvc,
                        const base::string16& exp_month,
                        const base::string16& exp_year,
                        bool should_store_pan) override;
  void NewCardLinkClicked() override;
  base::string16 GetWindowTitle() const override;
  base::string16 GetInstructionsMessage() const override;
  base::string16 GetOkButtonLabel() const override;
  int GetCvcImageRid() const override;
  bool ShouldRequestExpirationDate() const override;
  bool CanStoreLocally() const override;
  bool GetStoreLocallyStartState() const override;
  bool InputCvcIsValid(const base::string16& input_text) const override;
  bool InputExpirationIsValid(const base::string16& month,
                              const base::string16& year) const override;
  int GetExpectedCvcLength() const override;
  base::TimeDelta GetSuccessMessageDuration() const override;

 protected:
  // Exposed for testing.
  CardUnmaskPromptView* view() { return card_unmask_view_; }
  void SetCreditCardForTesting(CreditCard test_card) { card_ = test_card; }

 private:
  bool AllowsRetry(AutofillClient::PaymentsRpcResult result);
  void LogOnCloseEvents();
  AutofillMetrics::UnmaskPromptEvent GetCloseReasonEvent();

  PrefService* pref_service_;
  bool new_card_link_clicked_;
  bool is_off_the_record_;
  CreditCard card_;
  AutofillClient::UnmaskCardReason reason_;
  base::WeakPtr<CardUnmaskDelegate> delegate_;
  CardUnmaskPromptView* card_unmask_view_;

  AutofillClient::PaymentsRpcResult unmasking_result_;
  bool unmasking_initial_should_store_pan_;
  int unmasking_number_of_attempts_;
  base::Time shown_timestamp_;
  // Timestamp of the last time the user clicked the Verify button.
  base::Time verify_timestamp_;

  CardUnmaskDelegate::UnmaskResponse pending_response_;

  base::WeakPtrFactory<CardUnmaskPromptControllerImpl> weak_pointer_factory_;

  DISALLOW_COPY_AND_ASSIGN(CardUnmaskPromptControllerImpl);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_UI_CARD_UNMASK_PROMPT_CONTROLLER_IMPL_H_
