// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/ios_can_make_payment_query_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/payments/core/can_make_payment_query.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"

// static
IOSCanMakePaymentQueryFactory* IOSCanMakePaymentQueryFactory::GetInstance() {
  return base::Singleton<IOSCanMakePaymentQueryFactory>::get();
}

// static
payments::CanMakePaymentQuery*
IOSCanMakePaymentQueryFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<payments::CanMakePaymentQuery*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

IOSCanMakePaymentQueryFactory::IOSCanMakePaymentQueryFactory()
    : BrowserStateKeyedServiceFactory(
          "CanMakePaymentQuery",
          BrowserStateDependencyManager::GetInstance()) {}

IOSCanMakePaymentQueryFactory::~IOSCanMakePaymentQueryFactory() {}

std::unique_ptr<KeyedService>
IOSCanMakePaymentQueryFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return base::WrapUnique(new payments::CanMakePaymentQuery);
}
