// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_LANGUAGE_LANGUAGE_MODEL_FACTORY_H
#define IOS_CHROME_BROWSER_LANGUAGE_LANGUAGE_MODEL_FACTORY_H

#include <memory>

#include "base/memory/singleton.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace ios {
class ChromeBrowserState;
}

namespace language {
class LanguageModel;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

// Manages the language model for each profile. The particular language model
// provided depends on feature flags.
class LanguageModelFactory : public BrowserStateKeyedServiceFactory {
 public:
  static LanguageModelFactory* GetInstance();
  static language::LanguageModel* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

 private:
  friend struct base::DefaultSingletonTraits<LanguageModelFactory>;

  LanguageModelFactory();
  ~LanguageModelFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* state) const override;
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(LanguageModelFactory);
};

#endif  // IOS_CHROME_BROWSER_LANGUAGE_LANGUAGE_MODEL_FACTORY_H
