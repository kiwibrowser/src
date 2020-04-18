// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SCHEDULER_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SCHEDULER_H_

#include <Foundation/Foundation.h>
#include <list>
#include <memory>

#include "base/observer_list.h"
#import "ios/chrome/browser/ui/browser_list/browser_user_data.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue_manager_observer.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue_observer.h"
#include "ios/web/public/web_state/web_state_observer.h"

@class OverlayCoordinator;
class OverlaySchedulerObserver;

namespace web {
class WebState;
}

// An object that manages scheduling when overlays from various OverlayQueues
// can be started.  If an OverlayQueue requires a WebState's content area to be
// visible, this class will update the WebStateLists's active WebState.
class OverlayScheduler : public BrowserUserData<OverlayScheduler>,
                         public OverlayQueueObserver,
                         public OverlayQueueManagerObserver,
                         public web::WebStateObserver {
 public:
  ~OverlayScheduler() override;

  // Adds and removes OverlaySchedulerObservers.
  void AddObserver(OverlaySchedulerObserver* observer);
  void RemoveObserver(OverlaySchedulerObserver* observer);

  // Getter and setter for the OverlayQueueManager.  Setting the manager to a
  // new value will add the OverlayScheduler as an observer to the manager and
  // all of its associated OverlayQueues.
  OverlayQueueManager* queue_manager() { return queue_manager_; }
  void SetQueueManager(OverlayQueueManager* queue_manager);

  // Whether the scheduler is paused.  When the scheduler is paused, no queued
  // overlays will be started.  When unpaused, it will attempt to start the next
  // queued overlay.
  void SetPaused(bool paused);
  bool paused() { return paused_; }

  // Whether a scheduled overlay is currently being shown.
  bool IsShowingOverlay() const;

  // Replaces the currently visible overlay with |overlay_coordinator|.
  void ReplaceVisibleOverlay(OverlayCoordinator* overlay_coordinator);

  // Cancels all scheduled overlays and empties overlay queues.
  void CancelOverlays();

  // Tells the OverlayScheduler to disconnect itself as an observer before
  // deallocation.
  void Disconnect();

 private:
  friend class BrowserUserData<OverlayScheduler>;

  // Private constructor used by factory method.
  explicit OverlayScheduler(Browser* browser);

  // OverlayQueueManagerObserver:
  void OverlayQueueManagerDidAddQueue(OverlayQueueManager* manager,
                                      OverlayQueue* queue) override;
  void OverlayQueueManagerWillRemoveQueue(OverlayQueueManager* manager,
                                          OverlayQueue* queue) override;

  // OverlayQueueObserver:
  void OverlayQueueDidAddOverlay(OverlayQueue* queue) override;
  void OverlayQueueWillReplaceVisibleOverlay(OverlayQueue* queue) override;
  void OverlayQueueDidStopVisibleOverlay(OverlayQueue* queue) override;
  void OverlayQueueDidCancelOverlays(OverlayQueue* queue) override;

  // WebStateObserver:
  void WasShown(web::WebState* web_state) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  // Attempts to show the next queued overlay.
  void TryToStartNextOverlay();
  // Notifies the scheduler that |web_state|'s content area was shown.
  void OnWebStateShown(web::WebState* web_state);
  // Cancels outstanding overlays for |queue| and removes this as an observer.
  void StopObservingQueue(OverlayQueue* queue);
  // Stops observing |web_state|.
  void StopObservingWebState(web::WebState* web_state);

  // The WebStateList of the Browser whose overlays are being scheduled.
  WebStateList* web_state_list_;
  // The OverlaySchedulerObservers.
  base::ObserverList<OverlaySchedulerObserver> observers_;
  // The OverlayQueueManager responsible for creating queues for the Browser
  // associated with this OverlayScheduler.
  OverlayQueueManager* queue_manager_ = nullptr;
  // The OverlayQueues that have queued overlays.  After an overlay is stopped,
  // the first queue is popped and the first ovelay in the next queue is
  // started.
  std::list<OverlayQueue*> overlay_queues_;
  // Whether the scheduler is paused.
  bool paused_ = false;
  // Whether the |overlay_queues_|'s frontmost queue's WebState is being
  // observed.  When a WebState-specific overlay is scheduled to be shown, the
  // scheduler will wait until its WebState's WasShown() is received before
  // starting that overlay.
  bool observing_front_web_state_ = false;

  DISALLOW_COPY_AND_ASSIGN(OverlayScheduler);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SCHEDULER_H_
