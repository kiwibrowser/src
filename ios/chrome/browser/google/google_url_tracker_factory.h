// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_GOOGLE_GOOGLE_URL_TRACKER_FACTORY_H_
#define IOS_CHROME_BROWSER_GOOGLE_GOOGLE_URL_TRACKER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace bas

class GoogleURLTracker;

namespace ios {

class ChromeBrowserState;

// Singleton that owns all GoogleURLTrackers and associates them with
// ios::ChromeBrowserState.
class GoogleURLTrackerFactory : public BrowserStateKeyedServiceFactory {
 public:
  static GoogleURLTracker* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static GoogleURLTrackerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<GoogleURLTrackerFactory>;

  GoogleURLTrackerFactory();
  ~GoogleURLTrackerFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
  bool ServiceIsCreatedWithBrowserState() const override;
  bool ServiceIsNULLWhileTesting() const override;
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(GoogleURLTrackerFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_GOOGLE_GOOGLE_URL_TRACKER_FACTORY_H_
