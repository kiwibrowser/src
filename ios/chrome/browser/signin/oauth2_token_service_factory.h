// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_OAUTH2_TOKEN_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SIGNIN_OAUTH2_TOKEN_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace ios {
class ChromeBrowserState;
}

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class ProfileOAuth2TokenService;

// Singleton that owns all ProfileOAuth2TokenServices and associates them with
// ios::ChromeBrowserState.
class OAuth2TokenServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the instance of ProfileOAuth2TokenService associated with this
  // browser state (creating one if none exists). Returns nulltpr if this
  // browser state cannot have a ProfileOAuth2TokenService (for example, if
  // |browser_state| is incognito).
  static ProfileOAuth2TokenService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  // Returns an instance of the OAuth2TokenServiceFactory singleton.
  static OAuth2TokenServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<OAuth2TokenServiceFactory>;

  OAuth2TokenServiceFactory();
  ~OAuth2TokenServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(OAuth2TokenServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_OAUTH2_TOKEN_SERVICE_FACTORY_H_
