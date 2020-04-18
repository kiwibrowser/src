// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_SMS_SERVICE_FACTORY_H_
#define CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_SMS_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class SMSService;

class SMSServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Get the singleton instance of the factory.
  static SMSServiceFactory* GetInstance();

  // Get the SMSService for |profile|, creating one if needed.
  static SMSService* GetForProfile(Profile* profile);

 protected:
  // Overridden from BrowserContextKeyedServiceFactory.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

 private:
  friend struct base::DefaultSingletonTraits<SMSServiceFactory>;

  SMSServiceFactory();
  ~SMSServiceFactory() override;

  DISALLOW_COPY_AND_ASSIGN(SMSServiceFactory);
};

#endif  // CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_SMS_SERVICE_FACTORY_H_
