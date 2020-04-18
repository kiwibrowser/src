// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_OBSERVER_H_

#include "base/macros.h"

class Browser;
class OverlayService;
namespace web {
class WebState;
}

// Interface for listening to OverlayService events.
class OverlayServiceObserver {
 public:
  OverlayServiceObserver() = default;
  virtual ~OverlayServiceObserver() = default;

  // Tells the observer that |service| is about to display an overlay over
  // |browser|.  If |web_state| is non-null, then the overlay is meant to be
  // displayed over its content area.
  virtual void OverlayServiceWillShowOverlay(OverlayService* service,
                                             web::WebState* web_state,
                                             Browser* browser) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlayServiceObserver);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_OBSERVER_H_
