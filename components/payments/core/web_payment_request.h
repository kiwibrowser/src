// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_WEB_PAYMENT_REQUEST_H_
#define COMPONENTS_PAYMENTS_CORE_WEB_PAYMENT_REQUEST_H_

#include <string>
#include <vector>

#include "components/payments/core/payment_address.h"
#include "components/payments/core/payment_details.h"
#include "components/payments/core/payment_method_data.h"
#include "components/payments/core/payment_options.h"

// C++ bindings for the PaymentRequest API PaymentRequest. Conforms to the
// following spec:
// https://w3c.github.io/payment-request/#paymentrequest-interface

namespace base {
class DictionaryValue;
}

namespace payments {

// All of the information provided by a page making a request for payment.
class WebPaymentRequest {
 public:
  WebPaymentRequest();
  ~WebPaymentRequest();

  bool operator==(const WebPaymentRequest& other) const;
  bool operator!=(const WebPaymentRequest& other) const;
  WebPaymentRequest(const WebPaymentRequest& other);
  WebPaymentRequest& operator=(const WebPaymentRequest& other);

  // Populates the properties of this WebPaymentRequest from |value|. Returns
  // true if the required values are present.
  bool FromDictionaryValue(const base::DictionaryValue& value);

  // The unique ID for this WebPaymentRequest. If it is not provided during
  // construction, one is generated.
  std::string payment_request_id;

  // Properties set in order to communicate user choices back to the page.
  mojom::PaymentAddressPtr shipping_address;
  std::string shipping_option;

  // Properties set via the constructor for communicating from the page to the
  // browser UI.
  std::vector<PaymentMethodData> method_data;
  PaymentDetails details;
  PaymentOptions options;
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_WEB_PAYMENT_REQUEST_H_
