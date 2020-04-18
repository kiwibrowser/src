// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_CONTENT_SETTINGS_WEB_VIEW_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_
#define IOS_WEB_VIEW_INTERNAL_CONTENT_SETTINGS_WEB_VIEW_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/ios/refcounted_browser_state_keyed_service_factory.h"

class HostContentSettingsMap;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace ios_web_view {

class WebViewBrowserState;

// Singleton that owns all HostContentSettingsMaps and associates them with a
// browser state.
class WebViewHostContentSettingsMapFactory
    : public RefcountedBrowserStateKeyedServiceFactory {
 public:
  static HostContentSettingsMap* GetForBrowserState(
      ios_web_view::WebViewBrowserState* browser_state);
  static WebViewHostContentSettingsMapFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      WebViewHostContentSettingsMapFactory>;

  WebViewHostContentSettingsMapFactory();
  ~WebViewHostContentSettingsMapFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  scoped_refptr<RefcountedKeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(WebViewHostContentSettingsMapFactory);
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_CONTENT_SETTINGS_WEB_VIEW_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_
