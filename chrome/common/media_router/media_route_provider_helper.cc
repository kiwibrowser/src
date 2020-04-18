// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/media_route_provider_helper.h"

#include "base/logging.h"

namespace media_router {

const char* ProviderIdToString(MediaRouteProviderId provider_id) {
  switch (provider_id) {
    case EXTENSION:
      return "EXTENSION";
    case WIRED_DISPLAY:
      return "WIRED_DISPLAY";
    case CAST:
      return "CAST";
    case DIAL:
      return "DIAL";
    case UNKNOWN:
      return "UNKNOWN";
  }

  NOTREACHED() << "Unknown provider_id " << static_cast<int>(provider_id);
  return "Unknown provider_id";
}

}  // namespace media_router
