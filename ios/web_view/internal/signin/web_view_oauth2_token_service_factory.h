// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_OAUTH2_TOKEN_SERVICE_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_OAUTH2_TOKEN_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class ProfileOAuth2TokenService;

namespace ios_web_view {
class WebViewBrowserState;

// Singleton that owns all ProfileOAuth2TokenServices and associates them with
// a browser state.
class WebViewOAuth2TokenServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the instance of ProfileOAuth2TokenService associated with this
  // browser state (creating one if none exists). Returns nulltpr if this
  // browser state cannot have a ProfileOAuth2TokenService (for example, if
  // |browser_state| is incognito).
  static ProfileOAuth2TokenService* GetForBrowserState(
      ios_web_view::WebViewBrowserState* browser_state);

  // Returns an instance of the OAuth2TokenServiceFactory singleton.
  static WebViewOAuth2TokenServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<WebViewOAuth2TokenServiceFactory>;

  WebViewOAuth2TokenServiceFactory();
  ~WebViewOAuth2TokenServiceFactory() override = default;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(WebViewOAuth2TokenServiceFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_OAUTH2_TOKEN_SERVICE_FACTORY_H_
