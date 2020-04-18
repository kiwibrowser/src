// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_CONTENT_SETTINGS_COOKIE_SETTINGS_FACTORY_H_
#define IOS_CHROME_BROWSER_CONTENT_SETTINGS_COOKIE_SETTINGS_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/ios/refcounted_browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace content_settings {
class CookieSettings;
}

namespace ios {

class ChromeBrowserState;

// Singleton that owns all CookieSettings and associates them with
// ios::ChromeBrowserState.
class CookieSettingsFactory : public RefcountedBrowserStateKeyedServiceFactory {
 public:
  static scoped_refptr<content_settings::CookieSettings> GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static CookieSettingsFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<CookieSettingsFactory>;

  CookieSettingsFactory();
  ~CookieSettingsFactory() override;

  // RefcountedBrowserStateKeyedServiceFactory implementation.
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
  scoped_refptr<RefcountedKeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(CookieSettingsFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_CONTENT_SETTINGS_COOKIE_SETTINGS_FACTORY_H_
