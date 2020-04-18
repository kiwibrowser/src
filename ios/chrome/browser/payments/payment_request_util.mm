// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/payments/payment_request_util.h"

#include "base/json/json_reader.h"
#include "base/strings/string16.h"
#include "base/strings/string_split.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/phone_number_i18n.h"
#include "components/autofill/core/browser/validation.h"
#include "components/payments/core/payment_address.h"
#include "components/payments/core/payment_instrument.h"
#include "components/payments/core/payment_request_data_util.h"
#include "components/payments/core/strings_util.h"
#include "components/payments/core/web_payment_request.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/payments/payment_request.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace payment_request_util {

namespace {

// These are defined as part of the spec at:
// https://w3c.github.io/payment-request/#paymentresponse-interface
static const char kPaymentResponseDetails[] = "details";
static const char kPaymentResponseId[] = "requestId";
static const char kPaymentResponseMethodName[] = "methodName";
static const char kPaymentResponsePayerEmail[] = "payerEmail";
static const char kPaymentResponsePayerName[] = "payerName";
static const char kPaymentResponsePayerPhone[] = "payerPhone";
static const char kPaymentResponseShippingAddress[] = "shippingAddress";
static const char kPaymentResponseShippingOption[] = "shippingOption";

}  // namespace

std::unique_ptr<base::DictionaryValue> PaymentResponseToDictionaryValue(
    const payments::PaymentResponse& response) {
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetString(kPaymentResponseId, response.payment_request_id);
  result->SetString(kPaymentResponseMethodName, response.method_name);
  // |details| is a json-serialized string. Parse it to a base::Value so that
  // when |result| is converted to a JSON string, the "details" property won't
  // get json-escaped.
  std::unique_ptr<base::Value> details_value =
      base::JSONReader().ReadToValue(response.details);
  result->Set(kPaymentResponseDetails, details_value
                                           ? std::move(details_value)
                                           : std::make_unique<base::Value>());
  result->Set(kPaymentResponseShippingAddress,
              response.shipping_address
                  ? payments::PaymentAddressToDictionaryValue(
                        *response.shipping_address)
                  : std::make_unique<base::Value>());
  result->SetString(kPaymentResponseShippingOption, response.shipping_option);
  result->SetString(kPaymentResponsePayerName, response.payer_name);
  result->SetString(kPaymentResponsePayerEmail, response.payer_email);
  result->SetString(kPaymentResponsePayerPhone, response.payer_phone);
  return result;
}

NSString* GetNameLabelFromAutofillProfile(
    const autofill::AutofillProfile& profile) {
  base::string16 label =
      profile.GetInfo(autofill::AutofillType(autofill::NAME_FULL),
                      GetApplicationContext()->GetApplicationLocale());
  return !label.empty() ? base::SysUTF16ToNSString(label) : nil;
}

NSString* GetShippingAddressLabelFromAutofillProfile(
    const autofill::AutofillProfile& profile) {
  base::string16 label = payments::GetShippingAddressLabelFormAutofillProfile(
      profile, GetApplicationContext()->GetApplicationLocale());
  return !label.empty() ? base::SysUTF16ToNSString(label) : nil;
}

NSString* GetBillingAddressLabelFromAutofillProfile(
    const autofill::AutofillProfile& profile) {
  base::string16 label = payments::GetBillingAddressLabelFromAutofillProfile(
      profile, GetApplicationContext()->GetApplicationLocale());
  return !label.empty() ? base::SysUTF16ToNSString(label) : nil;
}

NSString* GetPhoneNumberLabelFromAutofillProfile(
    const autofill::AutofillProfile& profile) {
  base::string16 label = autofill::i18n::GetFormattedPhoneNumberForDisplay(
      profile, GetApplicationContext()->GetApplicationLocale());
  return !label.empty() ? base::SysUTF16ToNSString(label) : nil;
}

NSString* GetEmailLabelFromAutofillProfile(
    const autofill::AutofillProfile& profile) {
  base::string16 label =
      profile.GetInfo(autofill::AutofillType(autofill::EMAIL_ADDRESS),
                      GetApplicationContext()->GetApplicationLocale());
  return !label.empty() ? base::SysUTF16ToNSString(label) : nil;
}

NSString* GetAddressNotificationLabelFromAutofillProfile(
    const payments::PaymentRequest& payment_request,
    const autofill::AutofillProfile& profile) {
  base::string16 label =
      payment_request.profile_comparator()->GetStringForMissingShippingFields(
          profile);
  return !label.empty() ? base::SysUTF16ToNSString(label) : nil;
}

NSString* GetPaymentMethodNotificationLabelFromPaymentMethod(
    const payments::PaymentInstrument& payment_method,
    const std::vector<autofill::AutofillProfile*>& billing_profiles) {
  base::string16 label = payment_method.GetMissingInfoLabel();
  return !label.empty() ? base::SysUTF16ToNSString(label) : nil;
}

NSString* GetShippingSectionTitle(payments::PaymentShippingType shipping_type) {
  switch (shipping_type) {
    case payments::PaymentShippingType::SHIPPING:
      return l10n_util::GetNSString(IDS_PAYMENTS_SHIPPING_SUMMARY_LABEL);
    case payments::PaymentShippingType::DELIVERY:
      return l10n_util::GetNSString(IDS_PAYMENTS_DELIVERY_SUMMARY_LABEL);
    case payments::PaymentShippingType::PICKUP:
      return l10n_util::GetNSString(IDS_PAYMENTS_PICKUP_SUMMARY_LABEL);
    default:
      NOTREACHED();
      return nil;
  }
}

NSString* GetShippingAddressSelectorErrorMessage(
    const payments::PaymentRequest& payment_request) {
  if (!payment_request.payment_details().error.empty())
    return base::SysUTF8ToNSString(payment_request.payment_details().error);

  switch (payment_request.shipping_type()) {
    case payments::PaymentShippingType::SHIPPING:
      return l10n_util::GetNSString(IDS_PAYMENTS_UNSUPPORTED_SHIPPING_ADDRESS);
    case payments::PaymentShippingType::DELIVERY:
      return l10n_util::GetNSString(IDS_PAYMENTS_UNSUPPORTED_DELIVERY_ADDRESS);
    case payments::PaymentShippingType::PICKUP:
      return l10n_util::GetNSString(IDS_PAYMENTS_UNSUPPORTED_PICKUP_ADDRESS);
    default:
      NOTREACHED();
      return nil;
  }
}

NSString* GetShippingOptionSelectorErrorMessage(
    const payments::PaymentRequest& payment_request) {
  if (!payment_request.payment_details().error.empty())
    return base::SysUTF8ToNSString(payment_request.payment_details().error);

  switch (payment_request.shipping_type()) {
    case payments::PaymentShippingType::SHIPPING:
      return l10n_util::GetNSString(IDS_PAYMENTS_UNSUPPORTED_SHIPPING_OPTION);
    case payments::PaymentShippingType::DELIVERY:
      return l10n_util::GetNSString(IDS_PAYMENTS_UNSUPPORTED_DELIVERY_OPTION);
    case payments::PaymentShippingType::PICKUP:
      return l10n_util::GetNSString(IDS_PAYMENTS_UNSUPPORTED_PICKUP_OPTION);
    default:
      NOTREACHED();
      return nil;
  }
}

NSString* GetContactNotificationLabelFromAutofillProfile(
    const payments::PaymentRequest& payment_request,
    const autofill::AutofillProfile& profile) {
  const base::string16 notification =
      payment_request.profile_comparator()->GetStringForMissingContactFields(
          profile);
  return !notification.empty() ? base::SysUTF16ToNSString(notification) : nil;
}

}  // namespace payment_request_util
