// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/ios_payment_request_cache_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/payments/payment_request_cache.h"
#include "ios/web/public/browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace payments {

// static
payments::PaymentRequestCache*
IOSPaymentRequestCacheFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<payments::PaymentRequestCache*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
IOSPaymentRequestCacheFactory* IOSPaymentRequestCacheFactory::GetInstance() {
  return base::Singleton<IOSPaymentRequestCacheFactory>::get();
}

IOSPaymentRequestCacheFactory::IOSPaymentRequestCacheFactory()
    : BrowserStateKeyedServiceFactory(
          "PaymentRequestCache",
          BrowserStateDependencyManager::GetInstance()) {}

IOSPaymentRequestCacheFactory::~IOSPaymentRequestCacheFactory() {}

std::unique_ptr<KeyedService>
IOSPaymentRequestCacheFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return base::WrapUnique(new payments::PaymentRequestCache);
}

}  // namespace payments
