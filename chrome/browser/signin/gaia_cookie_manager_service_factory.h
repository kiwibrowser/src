// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_GAIA_COOKIE_MANAGER_SERVICE_FACTORY_H_
#define CHROME_BROWSER_SIGNIN_GAIA_COOKIE_MANAGER_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class GaiaCookieManagerService;
class Profile;

// Singleton that owns the GaiaCookieManagerService(s) and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up.
class GaiaCookieManagerServiceFactory :
    public BrowserContextKeyedServiceFactory {
 public:
  // Returns the instance of GaiaCookieManagerService associated with this
  // profile (creating one if none exists). Returns NULL if this profile cannot
  // have an GaiaCookieManagerService (for example, if |profile| is incognito).
  static GaiaCookieManagerService* GetForProfile(Profile* profile);

  // Returns an instance of the factory singleton.
  static GaiaCookieManagerServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<GaiaCookieManagerServiceFactory>;

  GaiaCookieManagerServiceFactory();
  ~GaiaCookieManagerServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

#endif  // CHROME_BROWSER_SIGNIN_GAIA_COOKIE_MANAGER_SERVICE_FACTORY_H_
