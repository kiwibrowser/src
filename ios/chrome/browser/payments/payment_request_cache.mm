// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/payments/payment_request_cache.h"

#include <utility>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace payments {

PaymentRequestCache::PaymentRequestCache() {}

PaymentRequestCache::~PaymentRequestCache() {}

payments::PaymentRequest* PaymentRequestCache::AddPaymentRequest(
    web::WebState* web_state,
    std::unique_ptr<payments::PaymentRequest> payment_request) {
  PaymentRequestCache::PaymentRequestSet& payment_requests =
      GetPaymentRequests(web_state);
  const auto result = payment_requests.insert(std::move(payment_request));
  return result.first->get();
}

PaymentRequestCache::PaymentRequestSet& PaymentRequestCache::GetPaymentRequests(
    web::WebState* web_state) {
  const auto result = payment_requests_.insert(
      std::make_pair(web_state, PaymentRequestCache::PaymentRequestSet()));
  return result.first->second;
}

void PaymentRequestCache::ClearPaymentRequests(web::WebState* web_state) {
  payment_requests_.erase(web_state);
}

}  // namespace payments
