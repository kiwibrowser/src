// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_IDENTITY_MANAGER_FACTORY_H_
#define IOS_CHROME_BROWSER_SIGNIN_IDENTITY_MANAGER_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace identity {
class IdentityManager;
}

namespace ios {
class ChromeBrowserState;
}

// Singleton that owns all IdentityManager instances and associates them with
// BrowserStates.
class IdentityManagerFactory : public BrowserStateKeyedServiceFactory {
 public:
  static identity::IdentityManager* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  // Returns an instance of the IdentityManagerFactory singleton.
  static IdentityManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<IdentityManagerFactory>;

  IdentityManagerFactory();
  ~IdentityManagerFactory() override;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* browser_state) const override;

  DISALLOW_COPY_AND_ASSIGN(IdentityManagerFactory);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_IDENTITY_MANAGER_FACTORY_H_
