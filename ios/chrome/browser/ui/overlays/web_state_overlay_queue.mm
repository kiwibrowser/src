// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/web_state_overlay_queue.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/overlays/overlay_coordinator+internal.h"
#import "ios/chrome/browser/ui/overlays/overlay_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(WebStateOverlayQueue);

WebStateOverlayQueue::WebStateOverlayQueue(web::WebState* web_state)
    : web_state_(web_state), parent_coordinator_(nil) {
  DCHECK(web_state);
}

web::WebState* WebStateOverlayQueue::GetWebState() const {
  return web_state_;
}

void WebStateOverlayQueue::StartNextOverlay() {
  DCHECK(parent_coordinator_);
  DCHECK(HasQueuedOverlays());
  DCHECK(!IsShowingOverlay());
  [GetFirstOverlay() startOverlayingCoordinator:parent_coordinator_];
  OverlayWasStarted();
}

void WebStateOverlayQueue::AddWebStateOverlay(
    OverlayCoordinator* overlay_coordinator) {
  AddOverlay(overlay_coordinator);
}

void WebStateOverlayQueue::SetWebStateParentCoordinator(
    BrowserCoordinator* parent_coordinator) {
  parent_coordinator_ = parent_coordinator;
}
