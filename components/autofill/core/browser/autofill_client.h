// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/i18n/rtl.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "components/autofill/core/browser/risk_data_loader.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

class PrefService;

namespace content {
class RenderFrameHost;
}

namespace gfx {
class RectF;
}

namespace identity {
class IdentityManager;
}

namespace syncer {
class SyncService;
}

namespace ukm {
class UkmRecorder;
}

namespace autofill {

class AddressNormalizer;
class AutofillPopupDelegate;
class AutofillWebDataService;
class CardUnmaskDelegate;
class CreditCard;
class FormStructure;
class PersonalDataManager;
struct Suggestion;

// A client interface that needs to be supplied to the Autofill component by the
// embedder.
//
// Each client instance is associated with a given context within which an
// AutofillManager is used (e.g. a single tab), so when we say "for the client"
// below, we mean "in the execution context the client is associated with" (e.g.
// for the tab the AutofillManager is attached to).
class AutofillClient : public RiskDataLoader {
 public:
  enum PaymentsRpcResult {
    // Empty result. Used for initializing variables and should generally
    // not be returned nor passed as arguments unless explicitly allowed by
    // the API.
    NONE,

    // Request succeeded.
    SUCCESS,

    // Request failed; try again.
    TRY_AGAIN_FAILURE,

    // Request failed; don't try again.
    PERMANENT_FAILURE,

    // Unable to connect to Payments servers. Prompt user to check internet
    // connection.
    NETWORK_ERROR,
  };

  enum UnmaskCardReason {
    // The card is being unmasked for PaymentRequest.
    UNMASK_FOR_PAYMENT_REQUEST,

    // The card is being unmasked for Autofill.
    UNMASK_FOR_AUTOFILL,
  };

  typedef base::Callback<void(const CreditCard&)> CreditCardScanCallback;

  ~AutofillClient() override {}

  // Gets the PersonalDataManager instance associated with the client.
  virtual PersonalDataManager* GetPersonalDataManager() = 0;

  // Gets the AutofillWebDataService instance associated with the client.
  virtual scoped_refptr<AutofillWebDataService> GetDatabase() = 0;

  // Gets the preferences associated with the client.
  virtual PrefService* GetPrefs() = 0;

  // Gets the sync service associated with the client.
  virtual syncer::SyncService* GetSyncService() = 0;

  // Gets the IdentityManager associated with the client.
  virtual identity::IdentityManager* GetIdentityManager() = 0;

  // Gets the UKM service associated with this client (for metrics).
  virtual ukm::UkmRecorder* GetUkmRecorder() = 0;

  // Gets an AddressNormalizer instance (can be null).
  virtual AddressNormalizer* GetAddressNormalizer() = 0;

  // Causes the Autofill settings UI to be shown.
  virtual void ShowAutofillSettings() = 0;

  // A user has attempted to use a masked card. Prompt them for further
  // information to proceed.
  virtual void ShowUnmaskPrompt(const CreditCard& card,
                                UnmaskCardReason reason,
                                base::WeakPtr<CardUnmaskDelegate> delegate) = 0;
  virtual void OnUnmaskVerificationResult(PaymentsRpcResult result) = 0;

  // Runs |callback| if the |card| should be imported as personal data.
  // |metric_logger| can be used to log user actions.
  virtual void ConfirmSaveCreditCardLocally(const CreditCard& card,
                                            const base::Closure& callback) = 0;

  // Runs |callback| if the |card| should be uploaded to Payments. Displays the
  // contents of |legal_message| to the user.
  virtual void ConfirmSaveCreditCardToCloud(
      const CreditCard& card,
      std::unique_ptr<base::DictionaryValue> legal_message,
      const base::Closure& callback) = 0;

  // Will show an infobar to get user consent for Credit Card assistive filling.
  // Will run |callback| on success.
  virtual void ConfirmCreditCardFillAssist(const CreditCard& card,
                                           const base::Closure& callback) = 0;

  // Returns true if both the platform and the device support scanning credit
  // cards. Should be called before ScanCreditCard().
  virtual bool HasCreditCardScanFeature() = 0;

  // Shows the user interface for scanning a credit card. Invokes the |callback|
  // when a credit card is scanned successfully. Should be called only if
  // HasCreditCardScanFeature() returns true.
  virtual void ScanCreditCard(const CreditCardScanCallback& callback) = 0;

  // Shows an Autofill popup with the given |values|, |labels|, |icons|, and
  // |identifiers| for the element at |element_bounds|. |delegate| will be
  // notified of popup events.
  virtual void ShowAutofillPopup(
      const gfx::RectF& element_bounds,
      base::i18n::TextDirection text_direction,
      const std::vector<Suggestion>& suggestions,
      base::WeakPtr<AutofillPopupDelegate> delegate) = 0;

  // Update the data list values shown by the Autofill popup, if visible.
  virtual void UpdateAutofillPopupDataListValues(
      const std::vector<base::string16>& values,
      const std::vector<base::string16>& labels) = 0;

  // Hide the Autofill popup if one is currently showing.
  virtual void HideAutofillPopup() = 0;

  // Whether the Autocomplete feature of Autofill should be enabled.
  virtual bool IsAutocompleteEnabled() = 0;

  // Pass the form structures to the password manager to choose correct username
  // and to the password generation manager to detect account creation forms.
  virtual void PropagateAutofillPredictions(
      content::RenderFrameHost* rfh,
      const std::vector<autofill::FormStructure*>& forms) = 0;

  // Inform the client that the field has been filled.
  virtual void DidFillOrPreviewField(
      const base::string16& autofilled_value,
      const base::string16& profile_full_name) = 0;

  // Inform the client that the user interacted with a non-secure credit card
  // field.
  virtual void DidInteractWithNonsecureCreditCardInput() = 0;

  // If the context is secure.
  virtual bool IsContextSecure() = 0;

  // Whether it is appropriate to show a signin promo for this user.
  virtual bool ShouldShowSigninPromo() = 0;

  // Whether Autofill is currently supported by the client. If false, all
  // features of Autofill are disabled, including Autocomplete.
  virtual bool IsAutofillSupported() = 0;

  // Whether server side cards are supported by the client. If false, only
  // local cards will be shown.
  virtual bool AreServerCardsSupported() = 0;

  // Handles simple actions for the autofill popups.
  virtual void ExecuteCommand(int id) = 0;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_
