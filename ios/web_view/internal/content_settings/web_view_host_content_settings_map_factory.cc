// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/content_settings/web_view_host_content_settings_map_factory.h"

#include "base/memory/singleton.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "ios/web_view/internal/web_view_browser_state.h"

namespace ios_web_view {

// static
HostContentSettingsMap*
WebViewHostContentSettingsMapFactory::GetForBrowserState(
    ios_web_view::WebViewBrowserState* browser_state) {
  return static_cast<HostContentSettingsMap*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true).get());
}

// static
WebViewHostContentSettingsMapFactory*
WebViewHostContentSettingsMapFactory::GetInstance() {
  return base::Singleton<WebViewHostContentSettingsMapFactory>::get();
}

WebViewHostContentSettingsMapFactory::WebViewHostContentSettingsMapFactory()
    : RefcountedBrowserStateKeyedServiceFactory(
          "HostContentSettingsMap",
          BrowserStateDependencyManager::GetInstance()) {}

WebViewHostContentSettingsMapFactory::~WebViewHostContentSettingsMapFactory() {}

void WebViewHostContentSettingsMapFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  HostContentSettingsMap::RegisterProfilePrefs(registry);
}

scoped_refptr<RefcountedKeyedService>
WebViewHostContentSettingsMapFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  WebViewBrowserState* browser_state =
      WebViewBrowserState::FromBrowserState(context);
  return base::MakeRefCounted<HostContentSettingsMap>(
      browser_state->GetPrefs(), browser_state->IsOffTheRecord(),
      false /* guest_profile */, false /* store_last_modified */);
}

web::BrowserState* WebViewHostContentSettingsMapFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return context;
}

}  // namespace ios_web_view
