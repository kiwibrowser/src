// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_AUTH_SYNC_OBSERVER_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_AUTH_SYNC_OBSERVER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace chromeos {

class AuthSyncObserver;

// Singleton that owns all AuthSyncObserver and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated AuthSyncObserver.
class AuthSyncObserverFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the instance of AuthSyncObserver associated with this
  // |profile| (creates one if none exists).
  static AuthSyncObserver* GetForProfile(Profile* profile);

  // Returns an instance of the AuthSyncObserverFactory singleton.
  static AuthSyncObserverFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AuthSyncObserverFactory>;

  AuthSyncObserverFactory();
  ~AuthSyncObserverFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AuthSyncObserverFactory);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_AUTH_SYNC_OBSERVER_FACTORY_H_
