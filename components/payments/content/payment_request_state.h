// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_STATE_H_
#define COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_STATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/autofill/core/browser/address_normalizer.h"
#include "components/payments/content/payment_request_spec.h"
#include "components/payments/content/payment_response_helper.h"
#include "components/payments/content/service_worker_payment_app_factory.h"
#include "components/payments/core/payments_profile_comparator.h"
#include "content/public/browser/payment_app_provider.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/platform/modules/payments/payment_request.mojom.h"

namespace autofill {
class AutofillProfile;
class CreditCard;
class PersonalDataManager;
class RegionDataLoader;
}  // namespace autofill

namespace payments {

class ContentPaymentRequestDelegate;
class JourneyLogger;
class PaymentInstrument;
class ServiceWorkerPaymentInstrument;

// Keeps track of the information currently selected by the user and whether the
// user is ready to pay. Uses information from the PaymentRequestSpec, which is
// what the merchant has specified, as input into the "is ready to pay"
// computation.
class PaymentRequestState : public PaymentResponseHelper::Delegate,
                            public PaymentRequestSpec::Observer {
 public:
  // Any class call add itself as Observer via AddObserver() and receive
  // notification about the state changing.
  class Observer {
   public:
    // Called when finished getting all available payment instruments.
    virtual void OnGetAllPaymentInstrumentsFinished() = 0;

    // Called when the information (payment method, address/contact info,
    // shipping option) changes.
    virtual void OnSelectedInformationChanged() = 0;

   protected:
    virtual ~Observer() {}
  };

  class Delegate {
   public:
    // Called when the PaymentResponse is available.
    virtual void OnPaymentResponseAvailable(
        mojom::PaymentResponsePtr response) = 0;

    // Called when the shipping option has changed to |shipping_option_id|.
    virtual void OnShippingOptionIdSelected(std::string shipping_option_id) = 0;

    // Called when the shipping address has changed to |address|.
    virtual void OnShippingAddressSelected(
        mojom::PaymentAddressPtr address) = 0;

   protected:
    virtual ~Delegate() {}
  };

  using StatusCallback = base::OnceCallback<void(bool)>;

  PaymentRequestState(content::WebContents* web_contents,
                      const GURL& top_level_origin,
                      const GURL& frame_origin,
                      PaymentRequestSpec* spec,
                      Delegate* delegate,
                      const std::string& app_locale,
                      autofill::PersonalDataManager* personal_data_manager,
                      ContentPaymentRequestDelegate* payment_request_delegate,
                      JourneyLogger* journey_logger);
  ~PaymentRequestState() override;

  // PaymentResponseHelper::Delegate
  void OnPaymentResponseReady(
      mojom::PaymentResponsePtr payment_response) override;

  // PaymentRequestSpec::Observer
  void OnStartUpdating(PaymentRequestSpec::UpdateReason reason) override {}
  void OnSpecUpdated() override;

  // Checks whether the user has at least one instrument that satisfies the
  // specified supported payment methods asynchronously.
  void CanMakePayment(StatusCallback callback);

  // Checks if the payment methods that the merchant website have
  // requested are supported asynchronously. For example, may return true for
  // "basic-card", but false for "https://bobpay.com".
  void AreRequestedMethodsSupported(StatusCallback callback);

  // Returns authenticated user email, or empty string.
  std::string GetAuthenticatedEmail() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Initiates the generation of the PaymentResponse. Callers should check
  // |is_ready_to_pay|, which is inexpensive.
  void GeneratePaymentResponse();

  // Record the use of the data models that were used in the Payment Request.
  void RecordUseStats();

  // Gets the Autofill Profile representing the shipping address or contact
  // information currently selected for this PaymentRequest flow. Can return
  // null.
  autofill::AutofillProfile* selected_shipping_profile() const {
    return selected_shipping_profile_;
  }
  // If |spec()->selected_shipping_option_error()| is not empty, this contains
  // the profile for which the error is about.
  autofill::AutofillProfile* selected_shipping_option_error_profile() const {
    return selected_shipping_option_error_profile_;
  }
  autofill::AutofillProfile* selected_contact_profile() const {
    return selected_contact_profile_;
  }
  // Returns the currently selected instrument for this PaymentRequest flow.
  // It's not guaranteed to be complete. Returns nullptr if there is no selected
  // instrument.
  PaymentInstrument* selected_instrument() const {
    return selected_instrument_;
  }

  // Returns the appropriate Autofill Profiles for this user. The profiles
  // returned are owned by the PaymentRequestState.
  const std::vector<autofill::AutofillProfile*>& shipping_profiles() {
    return shipping_profiles_;
  }
  const std::vector<autofill::AutofillProfile*>& contact_profiles() {
    return contact_profiles_;
  }
  const std::vector<std::unique_ptr<PaymentInstrument>>&
  available_instruments() {
    return available_instruments_;
  }

  // Creates and adds an AutofillPaymentInstrument, which makes a copy of
  // |card|. |selected| indicates if the newly-created instrument should be
  // selected, after which observers will be notified.
  void AddAutofillPaymentInstrument(bool selected,
                                    const autofill::CreditCard& card);

  // Creates and adds an AutofillProfile as a shipping profile, which makes a
  // copy of |profile|. |selected| indicates if the newly-created shipping
  // profile should be selected, after which observers will be notified.
  void AddAutofillShippingProfile(bool selected,
                                  const autofill::AutofillProfile& profile);

  // Creates and adds an AutofillProfile as a contact profile, which makes a
  // copy of |profile|. |selected| indicates if the newly-created shipping
  // profile should be selected, after which observers will be notified.
  void AddAutofillContactProfile(bool selected,
                                 const autofill::AutofillProfile& profile);

  // Setters to change the selected information. Will have the side effect of
  // recomputing "is ready to pay" and notify observers.
  void SetSelectedShippingOption(const std::string& shipping_option_id);
  void SetSelectedShippingProfile(autofill::AutofillProfile* profile);
  void SetSelectedContactProfile(autofill::AutofillProfile* profile);
  void SetSelectedInstrument(PaymentInstrument* instrument);

  bool is_ready_to_pay() { return is_ready_to_pay_; }

  // Checks whether getting all available instruments is finished.
  bool is_get_all_instruments_finished() {
    return get_all_instruments_finished_;
  }

  const std::string& GetApplicationLocale();
  autofill::PersonalDataManager* GetPersonalDataManager();
  autofill::RegionDataLoader* GetRegionDataLoader();

  Delegate* delegate() { return delegate_; }

  PaymentsProfileComparator* profile_comparator() {
    return &profile_comparator_;
  }

  // Returns true if the payment app has been invoked and the payment response
  // generation has begun. False otherwise.
  bool IsPaymentAppInvoked() const;

  autofill::AddressNormalizer* GetAddressNormalizer();

 private:
  // Fetches the Autofill Profiles for this user from the PersonalDataManager,
  // and stores copies of them, owned by this PaymentRequestState, in
  // profile_cache_.
  void PopulateProfileCache();

  // Sets the initial selections for instruments and profiles, and notifies
  // observers.
  void SetDefaultProfileSelections();

  // Uses the user-selected information as well as the merchant spec to update
  // |is_ready_to_pay_| with the current state, by validating that all the
  // required information is available. Will notify observers.
  void UpdateIsReadyToPayAndNotifyObservers();

  // Notifies all observers that getting all payment instruments is finished.
  void NotifyOnGetAllPaymentInstrumentsFinished();

  // Notifies all observers that selected information has changed.
  void NotifyOnSelectedInformationChanged();

  // Returns whether the selected data satisfies the PaymentDetails requirements
  // (payment methods).
  bool ArePaymentDetailsSatisfied();

  // Returns whether the selected data satisfies the PaymentOptions requirements
  // (contact info, shipping address).
  bool ArePaymentOptionsSatisfied();

  // The PaymentAppProvider::GetAllPaymentAppsCallback.
  void GetAllPaymentAppsCallback(
      content::WebContents* web_contents,
      const GURL& top_level_origin,
      const GURL& frame_origin,
      content::PaymentAppProvider::PaymentApps apps,
      ServiceWorkerPaymentAppFactory::InstallablePaymentApps installable_apps);

  // The ServiceWorkerPaymentInstrument::ValidateCanMakePaymentCallback.
  void OnSWPaymentInstrumentValidated(
      ServiceWorkerPaymentInstrument* instrument,
      bool result);
  void FinishedGetAllSWPaymentInstruments();

  // Checks whether the user has at least one instrument that satisfies the
  // specified supported payment methods and call the |callback| to return the
  // result.
  void CheckCanMakePayment(StatusCallback callback);

  // Checks if the payment methods that the merchant website have
  // requested are supported and call the |callback| to return the result.
  void CheckRequestedMethodsSupported(StatusCallback callback);

  void OnAddressNormalized(bool success,
                           const autofill::AutofillProfile& normalized_profile);

  bool is_ready_to_pay_;

  bool get_all_instruments_finished_;

  // Whether the data is currently being validated by the merchant.
  bool is_waiting_for_merchant_validation_;

  const std::string app_locale_;

  // Not owned. Never null. Will outlive this object.
  PaymentRequestSpec* spec_;
  Delegate* delegate_;
  autofill::PersonalDataManager* personal_data_manager_;
  JourneyLogger* journey_logger_;

  StatusCallback can_make_payment_callback_;
  StatusCallback are_requested_methods_supported_callback_;

  autofill::AutofillProfile* selected_shipping_profile_;
  autofill::AutofillProfile* selected_shipping_option_error_profile_;
  autofill::AutofillProfile* selected_contact_profile_;
  PaymentInstrument* selected_instrument_;

  // Number of pending service worker payment instruments waiting for
  // validation.
  int number_of_pending_sw_payment_instruments_;

  // Profiles may change due to (e.g.) sync events, so profiles are cached after
  // loading and owned here. They are populated once only, and ordered by
  // frecency.
  std::vector<std::unique_ptr<autofill::AutofillProfile>> profile_cache_;
  std::vector<autofill::AutofillProfile*> shipping_profiles_;
  std::vector<autofill::AutofillProfile*> contact_profiles_;

  // Credit cards are directly owned by the instruments in this list.
  std::vector<std::unique_ptr<PaymentInstrument>> available_instruments_;

  ContentPaymentRequestDelegate* payment_request_delegate_;

  std::unique_ptr<PaymentResponseHelper> response_helper_;

  PaymentsProfileComparator profile_comparator_;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<PaymentRequestState> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PaymentRequestState);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_STATE_H_
