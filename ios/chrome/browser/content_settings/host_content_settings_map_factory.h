// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_CONTENT_SETTINGS_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_
#define IOS_CHROME_BROWSER_CONTENT_SETTINGS_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/ios/refcounted_browser_state_keyed_service_factory.h"

class HostContentSettingsMap;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace ios {

class ChromeBrowserState;

// Singleton that owns all HostContentSettingsMaps and associates them with
// ios::ChromeBrowserState.
class HostContentSettingsMapFactory
    : public RefcountedBrowserStateKeyedServiceFactory {
 public:
  static HostContentSettingsMap* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static HostContentSettingsMapFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<HostContentSettingsMapFactory>;

  HostContentSettingsMapFactory();
  ~HostContentSettingsMapFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  scoped_refptr<RefcountedKeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(HostContentSettingsMapFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_CONTENT_SETTINGS_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_
