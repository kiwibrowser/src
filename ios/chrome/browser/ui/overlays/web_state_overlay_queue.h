// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_WEB_STATE_OVERLAY_QUEUE_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_WEB_STATE_OVERLAY_QUEUE_H_

#import "ios/chrome/browser/ui/overlays/overlay_queue.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@class BrowserCoordinator;

// An implementation of OverlayQueue that stores BrowserCoordinators for a
// specific WebState.
class WebStateOverlayQueue
    : public OverlayQueue,
      public web::WebStateUserData<WebStateOverlayQueue> {
 public:
  // OverlayQueue:
  web::WebState* GetWebState() const override;
  void StartNextOverlay() override;

  // Adds an overlay coordinator that will be displayed over GetWebState()'s
  // content area.
  void AddWebStateOverlay(OverlayCoordinator* overlay_coordinator);
  // Sets the parent coordinator to use for queued BrowserCoordinators.
  // |parent| is expected to be displaying GetWebState()'s content area before
  // calling this function.
  void SetWebStateParentCoordinator(BrowserCoordinator* parent_coordinator);

 private:
  friend class web::WebStateUserData<WebStateOverlayQueue>;

  // Private constructor.
  explicit WebStateOverlayQueue(web::WebState* web_state);

  // The WebState with which this presenter is associated.
  web::WebState* web_state_;
  // The parent coordinator to use for overlays.
  __weak BrowserCoordinator* parent_coordinator_;

  DISALLOW_COPY_AND_ASSIGN(WebStateOverlayQueue);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_WEB_STATE_OVERLAY_QUEUE_H_
