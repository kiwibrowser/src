// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_OBSERVER_BRIDGE_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_OBSERVER_BRIDGE_H_

#include "ios/chrome/browser/ui/overlays/overlay_service_observer.h"

#import <Foundation/Foundation.h>

// Objective-C interface for observing an OverlayService.  To use, wrap in an
// OverlayServiceObserverBridge.
@protocol OverlayServiceObserving<NSObject>

// Tells the observer that |service| is about to display an overlay over
// |browser|.  If |web_state| is non-null, then the overlay is meant to be
// displayed over its content area.
- (void)overlayService:(OverlayService*)overlayService
    willShowOverlayForWebState:(web::WebState*)webState
                     inBrowser:(Browser*)browser;

@end

// Observer that bridges OverlayService events to an Objective-C observer that
// implements the OverlayServiceObserving protocol (the observer is *not*
// owned).
class OverlayServiceObserverBridge final : public OverlayServiceObserver {
 public:
  // Constructor for a bridge that forwards events to |observer|.
  explicit OverlayServiceObserverBridge(id<OverlayServiceObserving> observer);
  ~OverlayServiceObserverBridge() override;

 private:
  // OverlayServiceObserver:
  void OverlayServiceWillShowOverlay(OverlayService* service,
                                     web::WebState* web_state,
                                     Browser* browser) override;

  // The observer passed on initialization.
  __weak id<OverlayServiceObserving> observer_;

  DISALLOW_COPY_AND_ASSIGN(OverlayServiceObserverBridge);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_OBSERVER_BRIDGE_H_
