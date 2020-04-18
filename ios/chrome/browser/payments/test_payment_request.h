// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PAYMENTS_TEST_PAYMENT_REQUEST_H_
#define IOS_CHROME_BROWSER_PAYMENTS_TEST_PAYMENT_REQUEST_H_

#include "base/macros.h"
#include "components/autofill/core/browser/address_normalization_manager.h"
#include "components/autofill/core/browser/test_address_normalizer.h"
#include "ios/chrome/browser/payments/payment_request.h"

namespace autofill {
class PersonalDataManager;
class RegionDataLoader;
}  // namespace autofill

namespace ios {
class ChromeBrowserState;
}  // namespace ios

namespace payments {
class PaymentShippingOption;
class PaymentsProfileComparator;
}  // namespace payments

namespace web {
class WebState;
}  // namespace web

class PrefService;

namespace payments {

// PaymentRequest for use in tests.
class TestPaymentRequest : public PaymentRequest {
 public:
  // |browser_state|, |web_state|, and |personal_data_manager| should not be
  // null and should outlive this object.
  TestPaymentRequest(const payments::WebPaymentRequest& web_payment_request,
                     ios::ChromeBrowserState* browser_state,
                     web::WebState* web_state,
                     autofill::PersonalDataManager* personal_data_manager,
                     id<PaymentRequestUIDelegate> payment_request_ui_delegate);

  TestPaymentRequest(const payments::WebPaymentRequest& web_payment_request,
                     ios::ChromeBrowserState* browser_state,
                     web::WebState* web_state,
                     autofill::PersonalDataManager* personal_data_manager);

  ~TestPaymentRequest() override {}

  void SetRegionDataLoader(autofill::RegionDataLoader* region_data_loader) {
    region_data_loader_ = region_data_loader;
  }

  void SetPrefService(PrefService* pref_service) {
    pref_service_ = pref_service;
  }

  void SetProfileComparator(PaymentsProfileComparator* profile_comparator) {
    profile_comparator_ = profile_comparator;
  }

  // Returns the payments::WebPaymentRequest instance that was used to build
  // this object.
  payments::WebPaymentRequest& web_payment_request() {
    return web_payment_request_;
  }

  // Removes all the shipping profiles.
  void ClearShippingProfiles();

  // Removes all the contact profiles.
  void ClearContactProfiles();

  // Removes all the payment methods.
  void ClearPaymentMethods();

  // Clears all url payment method identifiers, supported card networks,
  // basic card specified networks, and supported card types and then resets
  // them.
  void ResetParsedPaymentMethodData();

  // Sets the currently selected shipping option for this PaymentRequest flow.
  void set_selected_shipping_option(payments::PaymentShippingOption* option) {
    selected_shipping_option_ = option;
  }

  void set_is_incognito(bool is_incognito) { is_incognito_ = is_incognito; }

  // PaymentRequest
  autofill::AddressNormalizer* GetAddressNormalizer() override;
  autofill::AddressNormalizationManager* GetAddressNormalizationManager()
      override;
  autofill::RegionDataLoader* GetRegionDataLoader() override;
  PrefService* GetPrefService() override;
  PaymentsProfileComparator* profile_comparator() override;
  bool IsIncognito() const override;

 private:
  autofill::TestAddressNormalizer address_normalizer_;
  autofill::AddressNormalizationManager address_normalization_manager_;

  // Not owned and must outlive this object.
  autofill::RegionDataLoader* region_data_loader_;

  // Not owned and must outlive this object.
  PrefService* pref_service_;

  // Not owned and must outlive this object.
  PaymentsProfileComparator* profile_comparator_;

  // Whether the user is in incognito mode.
  bool is_incognito_;

  DISALLOW_COPY_AND_ASSIGN(TestPaymentRequest);
};

}  // namespace payments

#endif  // IOS_CHROME_BROWSER_PAYMENTS_TEST_PAYMENT_REQUEST_H_
