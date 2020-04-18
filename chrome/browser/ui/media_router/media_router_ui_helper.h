// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_ROUTER_UI_HELPER_H_
#define CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_ROUTER_UI_HELPER_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "chrome/browser/media/router/media_router.h"
#include "chrome/browser/ui/media_router/media_cast_mode.h"
#include "chrome/common/media_router/media_source.h"
#include "url/origin.h"

namespace extensions {
class ExtensionRegistry;
}

class GURL;

namespace media_router {

// Returns the extension name for |url|, so that it can be displayed for
// extension-initiated presentations.
std::string GetExtensionName(const GURL& url,
                             extensions::ExtensionRegistry* registry);

std::string GetHostFromURL(const GURL& gurl);

// Returns the duration to wait for route creation result before we time out.
base::TimeDelta GetRouteRequestTimeout(MediaCastMode cast_mode);

// Contains common parameters for route requests to MediaRouter.
struct RouteParameters {
 public:
  RouteParameters();
  RouteParameters(RouteParameters&& other);
  ~RouteParameters();

  RouteParameters& operator=(RouteParameters&& other);

  MediaSource::Id source_id;
  url::Origin origin;
  std::vector<MediaRouteResponseCallback> route_response_callbacks;
  base::TimeDelta timeout;
  bool incognito;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_ROUTER_UI_HELPER_H_
