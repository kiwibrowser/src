// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_FETCHER_SERVICE_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_FETCHER_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class AccountFetcherService;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios_web_view {

class WebViewBrowserState;

class WebViewAccountFetcherServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  static AccountFetcherService* GetForBrowserState(
      ios_web_view::WebViewBrowserState* browser_state);
  static WebViewAccountFetcherServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      WebViewAccountFetcherServiceFactory>;

  WebViewAccountFetcherServiceFactory();
  ~WebViewAccountFetcherServiceFactory() override = default;

  // BrowserStateKeyedServiceFactory implementation
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebViewAccountFetcherServiceFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_SIGNIN_WEB_VIEW_ACCOUNT_FETCHER_SERVICE_FACTORY_H_
