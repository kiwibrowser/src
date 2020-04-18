// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

namespace web {
class WebState;
}

class Browser;
@class BrowserCoordinator;
@class OverlayCoordinator;
class OverlayServiceObserver;

// OverlayService allows for the easy presentation and dismissal of overlay
// cordinators.  Overlays are modal and displayed in the order in which this
// service receives them.  If an overlay is added to this service while one is
// already presented, it will be queued until that current overlay is stopped.
// This serivce is run on the UI thread.
class OverlayService : public KeyedService {
 public:
  OverlayService() = default;
  ~OverlayService() override = default;

  // Adds and removes observers to the OverlayService.
  virtual void AddObserver(OverlayServiceObserver* observer) = 0;
  virtual void RemoveObserver(OverlayServiceObserver* observer) = 0;

  // The OverlayService can be paused for a particular Browser.
  virtual void PauseServiceForBrowser(Browser* browser) = 0;
  virtual void ResumeServiceForBrowser(Browser* browser) = 0;
  virtual bool IsPausedForBrowser(Browser* browser) const = 0;

  // Whether an overlay is currently displayed for |browser|.  This will return
  // true for both WebState-specific or Browser-level overlays.
  virtual bool IsBrowserShowingOverlay(Browser* browser) const = 0;

  // Replaces |browser|'s currently-visible overlay with |overlay_coordinator|.
  // The replacement overlay will use the visible overlay's parent as its own.
  // This will replace the current overlay regardless of if it's WebState-
  // specific.
  virtual void ReplaceVisibleOverlay(OverlayCoordinator* overlay_coordinator,
                                     Browser* browser) = 0;

  // Cancels all scheduled overlays added to this service.  If an overlay is
  // currently visible, it will be stopped.
  virtual void CancelOverlays() = 0;

  // Browser-level overlays:

  // Shows |overlay_coordinator| over |parent_coordinator|'s UI in |browser|.
  virtual void ShowOverlayForBrowser(OverlayCoordinator* overlay_coordinator,
                                     BrowserCoordinator* parent_coordiantor,
                                     Browser* browser) = 0;

  // Cancells all scheduled overlays for |browser|.  If an overlay is already
  // being shown for |browser|, it will be stopped.
  virtual void CancelAllOverlaysForBrowser(Browser* browser) = 0;

  // WebState-specific overlays:

  // Switches the active WebState to |web_state| and displays
  // |overlay_coordinator|'s UI over it.  If an overlay is already started,
  // the active WebState will switch and |overlay_coordinator| will be started
  // when that one is dismissed.
  virtual void ShowOverlayForWebState(OverlayCoordinator* overlay_coordinator,
                                      web::WebState* web_state) = 0;

  // Cancels all scheduled overlays for |web_state|.  If an overlay is already
  // being shown for |web_state|, it will be stopped.
  virtual void CancelAllOverlaysForWebState(web::WebState* web_state) = 0;

  // Overlays added via ShowOverlayForWebState() can only be started when their
  // associated WebState's content area is visible.  When a coordinator showing
  // a WebState's content is started, it can use this function to notify the
  // OverlayService to use itself as the parent for that WebState's overlays.
  virtual void SetWebStateParentCoordinator(
      BrowserCoordinator* parent_coordinator,
      web::WebState* web_state) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlayService);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_H_
