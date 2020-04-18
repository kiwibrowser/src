// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_NOTIFICATIONS_FACTORY_H_
#define CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_NOTIFICATIONS_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace cloud_print {

class PrivetNotificationServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of PrivetNotificationServiceFactory.
  static PrivetNotificationServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PrivetNotificationServiceFactory>;

  PrivetNotificationServiceFactory();
  ~PrivetNotificationServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace cloud_print

#endif  // CHROME_BROWSER_PRINTING_CLOUD_PRINT_PRIVET_NOTIFICATIONS_FACTORY_H_
