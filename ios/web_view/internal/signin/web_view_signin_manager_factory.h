// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_SIGNIN_MANAGER_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_SIGNIN_MANAGER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class SigninManager;

namespace ios_web_view {

class WebViewBrowserState;

// Singleton that owns all SigninManagers and associates them with browser
// states.
class WebViewSigninManagerFactory : public BrowserStateKeyedServiceFactory {
 public:
  static SigninManager* GetForBrowserState(
      ios_web_view::WebViewBrowserState* browser_state);
  static SigninManager* GetForBrowserStateIfExists(
      ios_web_view::WebViewBrowserState* browser_state);

  // Returns an instance of the SigninManagerFactory singleton.
  static WebViewSigninManagerFactory* GetInstance();

  // Implementation of BrowserStateKeyedServiceFactory (public so tests
  // can call it).
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

 private:
  friend struct base::DefaultSingletonTraits<WebViewSigninManagerFactory>;

  WebViewSigninManagerFactory();
  ~WebViewSigninManagerFactory() override = default;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  void BrowserStateShutdown(web::BrowserState* context) override;
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_SIGNIN_MANAGER_FACTORY_H_
