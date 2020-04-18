// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/ios_payment_instrument.h"

#include "base/strings/utf_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace payments {

// URL payment method identifiers for iOS payment apps.
const char kBobpayPaymentMethodIdentifier[] =
    "https://emerald-eon.appspot.com/bobpay";
const char kAlicepayPaymentMethodIdentifier[] =
    "https://emerald-eon.appspot.com/alicepay";

// Scheme names for iOS payment apps.
const char kBobpaySchemeName[] = "bobpay://";

const std::map<std::string, std::string>& GetMethodNameToSchemeName() {
  static const std::map<std::string, std::string> kMethodToScheme =
      std::map<std::string, std::string>{
          {kBobpayPaymentMethodIdentifier, kBobpaySchemeName},
          {kAlicepayPaymentMethodIdentifier, kBobpaySchemeName}};
  return kMethodToScheme;
}

IOSPaymentInstrument::IOSPaymentInstrument(
    const std::string& method_name,
    const GURL& universal_link,
    const std::string& app_name,
    UIImage* icon_image,
    id<PaymentRequestUIDelegate> payment_request_ui_delegate)
    : PaymentInstrument(-1 /* resource id not used */,
                        PaymentInstrument::Type::NATIVE_MOBILE_APP),
      method_name_(method_name),
      universal_link_(universal_link),
      app_name_(app_name),
      icon_image_(icon_image),
      payment_request_ui_delegate_(payment_request_ui_delegate) {}
IOSPaymentInstrument::~IOSPaymentInstrument() {}

void IOSPaymentInstrument::InvokePaymentApp(
    PaymentInstrument::Delegate* delegate) {
  DCHECK(delegate);
  [payment_request_ui_delegate_ paymentInstrument:this
                       launchAppWithUniversalLink:universal_link_
                               instrumentDelegate:delegate];
}

bool IOSPaymentInstrument::IsCompleteForPayment() const {
  // As long as the native app is installed on the user's device it is
  // always complete for payment.
  return true;
}

bool IOSPaymentInstrument::IsExactlyMatchingMerchantRequest() const {
  // TODO(crbug.com/602666): Determine if the native payment app supports
  // 'basic-card' if the merchant only accepts payment through credit cards.
  return true;
}

base::string16 IOSPaymentInstrument::GetMissingInfoLabel() const {
  // This will always be an empty string because a native app cannot
  // have incomplete information that can then be edited by the user.
  return base::string16();
}

bool IOSPaymentInstrument::IsValidForCanMakePayment() const {
  // Same as IsCompleteForPayment, as long as the native app is installed
  // and found on the user's device then it is valid for payment.
  return true;
}

void IOSPaymentInstrument::RecordUse() {
  // TODO(crbug.com/60266): Record the use of the native payment app.
}

base::string16 IOSPaymentInstrument::GetLabel() const {
  return base::ASCIIToUTF16(app_name_);
}

base::string16 IOSPaymentInstrument::GetSublabel() const {
  // Return host of |method_name_| e.g., paypal.com.
  return base::ASCIIToUTF16(GURL(method_name_).host());
}

bool IOSPaymentInstrument::IsValidForModifier(
    const std::vector<std::string>& methods,
    bool supported_networks_specified,
    const std::set<std::string>& supported_networks,
    bool supported_types_specified,
    const std::set<autofill::CreditCard::CardType>& supported_types) const {
  // This instrument only matches url-based payment method identifiers that
  // are equal to the instrument's method name.
  if (std::find(methods.begin(), methods.end(), method_name_) == methods.end())
    return false;

  // TODO(crbug.com/602666): Determine if the native payment app supports
  // 'basic-card' if 'basic-card' is the specified modifier.
  return true;
}

}  // namespace payments
