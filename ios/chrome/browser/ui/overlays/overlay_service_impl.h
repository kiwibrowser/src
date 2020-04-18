// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_IMPL_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_IMPL_H_

#include "base/observer_list.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_observer.h"
#import "ios/chrome/browser/ui/overlays/overlay_scheduler_observer.h"
#import "ios/chrome/browser/ui/overlays/overlay_service.h"

class BrowserList;

// Concrete subclass of OverlayService.
class OverlayServiceImpl : public BrowserListObserver,
                           public OverlaySchedulerObserver,
                           public OverlayService {
 public:
  // Constructor for an OverlayService that schedules overlays for the Browsers
  // in |browser_list|.
  OverlayServiceImpl(BrowserList* browser_list);
  ~OverlayServiceImpl() override;

 private:
  // The OverlayServiceObservers.
  base::ObserverList<OverlayServiceObserver> observers_;
  // The BrowserList passed on implementation.
  BrowserList* browser_list_;

  // BrowserListObserver:
  void OnBrowserCreated(BrowserList* browser_list, Browser* browser) override;
  void OnBrowserRemoved(BrowserList* browser_list, Browser* browser) override;

  // KeyedService:
  void Shutdown() override;

  // OverlaySchedulerObserver:
  void OverlaySchedulerWillShowOverlay(OverlayScheduler* scheduler,
                                       web::WebState* web_state) override;

  // OverlayService:
  void AddObserver(OverlayServiceObserver* observer) override;
  void RemoveObserver(OverlayServiceObserver* observer) override;
  void PauseServiceForBrowser(Browser* browser) override;
  void ResumeServiceForBrowser(Browser* browser) override;
  bool IsPausedForBrowser(Browser* browser) const override;
  bool IsBrowserShowingOverlay(Browser* browser) const override;
  void ReplaceVisibleOverlay(OverlayCoordinator* overlay_coordinator,
                             Browser* browser) override;
  void CancelOverlays() override;
  void ShowOverlayForBrowser(OverlayCoordinator* overlay_coordinator,
                             BrowserCoordinator* parent_coordiantor,
                             Browser* browser) override;
  void CancelAllOverlaysForBrowser(Browser* browser) override;
  void ShowOverlayForWebState(OverlayCoordinator* overlay_coordinator,
                              web::WebState* web_state) override;
  void SetWebStateParentCoordinator(BrowserCoordinator* parent_coordinator,
                                    web::WebState* web_state) override;
  void CancelAllOverlaysForWebState(web::WebState* web_state) override;

  // Sets up or tears down the OverlayQueueManager and OverlayScheduler for
  // |browser|.
  void StartServiceForBrowser(Browser* browser);
  void StopServiceForBrowser(Browser* browser);

  DISALLOW_COPY_AND_ASSIGN(OverlayServiceImpl);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SERVICE_IMPL_H_
