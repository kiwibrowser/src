// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_ACCOUNT_TRACKER_SERVICE_FACTORY_H_
#define CHROME_BROWSER_SIGNIN_ACCOUNT_TRACKER_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class AccountTrackerService;
class Profile;

// Singleton that owns all AccountTrackerServices and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated AccountTrackerService.
class AccountTrackerServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the instance of AccountTrackerService associated with this
  // profile (creating one if none exists). Returns NULL if this profile
  // cannot have a AccountTrackerService (for example, if |profile| is
  // incognito).
  static AccountTrackerService* GetForProfile(Profile* profile);

  // Returns an instance of the AccountTrackerServiceFactory singleton.
  static AccountTrackerServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AccountTrackerServiceFactory>;

  AccountTrackerServiceFactory();
  ~AccountTrackerServiceFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AccountTrackerServiceFactory);
};

#endif  // CHROME_BROWSER_SIGNIN_ACCOUNT_TRACKER_SERVICE_FACTORY_H_
