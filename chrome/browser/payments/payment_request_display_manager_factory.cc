// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/payments/payment_request_display_manager_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/payments/content/payment_request_display_manager.h"

namespace payments {

PaymentRequestDisplayManagerFactory*
PaymentRequestDisplayManagerFactory::GetInstance() {
  return base::Singleton<PaymentRequestDisplayManagerFactory>::get();
}

PaymentRequestDisplayManager*
PaymentRequestDisplayManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<PaymentRequestDisplayManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

PaymentRequestDisplayManagerFactory::PaymentRequestDisplayManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "PaymentRequestDisplayManager",
          BrowserContextDependencyManager::GetInstance()) {}

PaymentRequestDisplayManagerFactory::~PaymentRequestDisplayManagerFactory() {}

KeyedService* PaymentRequestDisplayManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PaymentRequestDisplayManager();
}

content::BrowserContext*
PaymentRequestDisplayManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Returns non-NULL even for Incognito contexts so that a separate
  // instance of a service is created for the Incognito context.
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace payments
