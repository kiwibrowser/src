// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SUGGESTIONS_SUGGESTIONS_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SUGGESTIONS_SUGGESTIONS_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios {
class ChromeBrowserState;
}

namespace suggestions {

class SuggestionsService;

// Singleton that owns all SuggestionsServices and associates them with
// web::BrowserState.
class SuggestionsServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static SuggestionsServiceFactory* GetInstance();

  // Returns the SuggestionsService for |browser_state|.
  static SuggestionsService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

 private:
  friend struct base::DefaultSingletonTraits<SuggestionsServiceFactory>;

  SuggestionsServiceFactory();
  ~SuggestionsServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(SuggestionsServiceFactory);
};

}  // namespace suggestions

#endif  // IOS_CHROME_BROWSER_SUGGESTIONS_SUGGESTIONS_SERVICE_FACTORY_H_
