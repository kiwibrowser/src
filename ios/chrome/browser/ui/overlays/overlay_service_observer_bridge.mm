// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/overlay_service_observer_bridge.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

OverlayServiceObserverBridge::OverlayServiceObserverBridge(
    id<OverlayServiceObserving> observer)
    : observer_(observer) {}

OverlayServiceObserverBridge::~OverlayServiceObserverBridge() {}

void OverlayServiceObserverBridge::OverlayServiceWillShowOverlay(
    OverlayService* service,
    web::WebState* web_state,
    Browser* browser) {
  [observer_ overlayService:service
      willShowOverlayForWebState:web_state
                       inBrowser:browser];
}
