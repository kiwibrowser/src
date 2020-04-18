// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_LANGUAGE_WEB_VIEW_LANGUAGE_MODEL_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_LANGUAGE_WEB_VIEW_LANGUAGE_MODEL_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace language {
class LanguageModel;
}  // namespace language

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace ios_web_view {

class WebViewBrowserState;

class WebViewLanguageModelFactory : public BrowserStateKeyedServiceFactory {
 public:
  static language::LanguageModel* GetForBrowserState(
      WebViewBrowserState* browser_state);
  static WebViewLanguageModelFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<WebViewLanguageModelFactory>;

  WebViewLanguageModelFactory();
  ~WebViewLanguageModelFactory() override = default;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* const registry) override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* state) const override;

  DISALLOW_COPY_AND_ASSIGN(WebViewLanguageModelFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_LANGUAGE_WEB_VIEW_LANGUAGE_MODEL_FACTORY_H_
