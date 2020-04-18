// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/route_message_observer.h"

#include "chrome/browser/media/router/media_router.h"

namespace media_router {

RouteMessageObserver::RouteMessageObserver(MediaRouter* router,
                                           const MediaRoute::Id& route_id)
    : router_(router), route_id_(route_id) {
  DCHECK(router_);
  DCHECK(!route_id_.empty());
  router_->RegisterRouteMessageObserver(this);
}

RouteMessageObserver::~RouteMessageObserver() {
  router_->UnregisterRouteMessageObserver(this);
}

}  // namespace media_router
