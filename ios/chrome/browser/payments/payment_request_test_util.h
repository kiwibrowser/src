// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PAYMENTS_PAYMENT_REQUEST_TEST_UTIL_H_
#define IOS_CHROME_BROWSER_PAYMENTS_PAYMENT_REQUEST_TEST_UTIL_H_

namespace payments {
class WebPaymentRequest;
}  // namespace payments

namespace payment_request_test_util {

// Returns an instance of web::PaymentRequest for testing purposes.
payments::WebPaymentRequest CreateTestWebPaymentRequest();

}  // namespace payment_request_test_util

#endif  // IOS_CHROME_BROWSER_PAYMENTS_PAYMENT_REQUEST_TEST_UTIL_H_
