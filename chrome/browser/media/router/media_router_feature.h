// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FEATURE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FEATURE_H_

#include "base/feature_list.h"

class PrefRegistrySimple;
class PrefService;

namespace content {
class BrowserContext;
}

namespace media_router {

// Returns true if Media Router is enabled for |context|.
bool MediaRouterEnabled(content::BrowserContext* context);

#if !defined(OS_ANDROID)

namespace prefs {
// Pref name for the enterprise policy for allowing Cast devices on all IPs.
constexpr char kMediaRouterCastAllowAllIPs[] =
    "media_router.cast_allow_all_ips";
}  // namespace prefs

// Registers |kMediaRouterCastAllowAllIPs| with local state pref |registry|.
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

// If enabled, allows Media Router to connect to Cast devices on all IP
// addresses, not just RFC1918/RFC4913 private addresses. Workaround for
// https://crbug.com/813974.
extern const base::Feature kCastAllowAllIPsFeature;

// Returns true if CastMediaSinkService can connect to Cast devices on
// all IPs, as determined by local state |pref_service| / feature flag.
bool GetCastAllowAllIPsPref(PrefService* pref_service);

extern const base::Feature kEnableDialSinkQuery;
extern const base::Feature kEnableCastDiscovery;
extern const base::Feature kCastMediaRouteProvider;

// Returns true if browser side DIAL Media Route Provider is enabled.
bool DialMediaRouteProviderEnabled();

// Returns true if browser side Cast discovery is enabled.
bool CastDiscoveryEnabled();

// Returns true if browser side Cast Media Route Provider and sink query are
// enabled.
bool CastMediaRouteProviderEnabled();

// Returns true if the presentation receiver window for local media casting is
// available on the current platform.
// TODO(crbug.com/802332): Remove this when mac_views_browser=1 by default.
bool PresentationReceiverWindowEnabled();

#endif  // !defined(OS_ANDROID)

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FEATURE_H_
