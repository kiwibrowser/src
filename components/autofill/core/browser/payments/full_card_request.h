// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_FULL_CARD_REQUEST_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_FULL_CARD_REQUEST_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/card_unmask_delegate.h"
#include "components/autofill/core/browser/payments/payments_client.h"

namespace autofill {

class CreditCard;
class PersonalDataManager;

namespace payments {

// Retrieves the full card details, including the pan and the cvc.
class FullCardRequest final : public CardUnmaskDelegate {
 public:
  // The interface for receiving the full card details.
  class ResultDelegate {
   public:
    virtual ~ResultDelegate() = default;
    virtual void OnFullCardRequestSucceeded(
        const payments::FullCardRequest& full_card_request,
        const CreditCard& card,
        const base::string16& cvc) = 0;
    virtual void OnFullCardRequestFailed() = 0;
  };

  // The delegate responsible for displaying the unmask prompt UI.
  class UIDelegate {
   public:
    virtual ~UIDelegate() = default;
    virtual void ShowUnmaskPrompt(
        const CreditCard& card,
        AutofillClient::UnmaskCardReason reason,
        base::WeakPtr<CardUnmaskDelegate> delegate) = 0;
    virtual void OnUnmaskVerificationResult(
        AutofillClient::PaymentsRpcResult result) = 0;
  };

  // The parameters should outlive the FullCardRequest.
  FullCardRequest(RiskDataLoader* risk_data_loader,
                  payments::PaymentsClient* payments_client,
                  PersonalDataManager* personal_data_manager);
  FullCardRequest(RiskDataLoader* risk_data_loader,
                  payments::PaymentsClient* payments_client,
                  PersonalDataManager* personal_data_manager,
                  base::TimeTicks form_parsed_timestamp);
  ~FullCardRequest();

  // Retrieves the pan and cvc for |card| and invokes
  // Delegate::OnFullCardRequestSucceeded() or
  // Delegate::OnFullCardRequestFailed(). Only one request should be active at a
  // time.
  //
  // If the card is local, has a non-empty GUID, and the user has updated its
  // expiration date, then this function will write the new information to
  // autofill table on disk.
  void GetFullCard(const CreditCard& card,
                   AutofillClient::UnmaskCardReason reason,
                   base::WeakPtr<ResultDelegate> result_delegate,
                   base::WeakPtr<UIDelegate> ui_delegate);

  // Returns true if there's a pending request to get the full card.
  bool IsGettingFullCard() const;

  // Called by the payments client when a card has been unmasked.
  void OnDidGetRealPan(AutofillClient::PaymentsRpcResult result,
                       const std::string& real_pan);

  base::TimeTicks form_parsed_timestamp() const {
    return form_parsed_timestamp_;
  }

 private:
  // CardUnmaskDelegate:
  void OnUnmaskResponse(const UnmaskResponse& response) override;
  void OnUnmaskPromptClosed() override;

  // Called by autofill client when the risk data has been loaded.
  void OnDidGetUnmaskRiskData(const std::string& risk_data);

  // Makes final preparations for the unmask request and calls
  // PaymentsClient::UnmaskCard().
  void SendUnmaskCardRequest();

  // Resets the state of the request.
  void Reset();

  // Used to fetch risk data for this request.
  RiskDataLoader* const risk_data_loader_;

  // Responsible for unmasking a masked server card.
  payments::PaymentsClient* const payments_client_;

  // Responsible for updating the server card on disk after it's been unmasked.
  PersonalDataManager* const personal_data_manager_;

  // Receiver of the full PAN and CVC.
  base::WeakPtr<ResultDelegate> result_delegate_;

  // Delegate responsible for displaying the unmask prompt UI.
  base::WeakPtr<UIDelegate> ui_delegate_;

  // The pending request to get a card's full PAN and CVC.
  std::unique_ptr<payments::PaymentsClient::UnmaskRequestDetails> request_;

  // Whether the card unmask request should be sent to the payment server.
  bool should_unmask_card_;

  // The timestamp when the full PAN was requested from a server. For
  // histograms.
  base::Time real_pan_request_timestamp_;

  // The timestamp when the form is parsed. For histograms.
  base::TimeTicks form_parsed_timestamp_;

  // Enables destroying FullCardRequest while CVC prompt is showing or a server
  // communication is pending.
  base::WeakPtrFactory<FullCardRequest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FullCardRequest);
};

}  // namespace payments
}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_FULL_CARD_REQUEST_H_
