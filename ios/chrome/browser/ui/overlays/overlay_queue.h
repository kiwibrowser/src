// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_H_

#import <Foundation/Foundation.h>

#include <vector>

#include "base/observer_list.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue_observer.h"

@class OverlayCoordinator;
namespace web {
class WebState;
}

// Class used to enqueue OverlayCoordinators.  It communicates changes in the
// queue to registered OverlayQueueObservers.
class OverlayQueue {
 public:
  virtual ~OverlayQueue();

  // Adds and removes OverlayQueueObservers.
  void AddObserver(OverlayQueueObserver* observer);
  void RemoveObserver(OverlayQueueObserver* observer);

  // Starts the next overlay in the queue.  If GetWebState() returns non-null,
  // it is expected that its content area is visible before this is called.
  virtual void StartNextOverlay() = 0;
  // Tells the OverlayQueue that |overlay_coordinator| was stopped.
  virtual void OverlayWasStopped(OverlayCoordinator* overlay_coordinator);
  // Removes the currently displayed overlay and adds |overlay_coordinator| to
  // the front of the queue to be displayed immediately.
  virtual void ReplaceVisibleOverlay(OverlayCoordinator* overlay_coordinator);
  // Returns whether there are any queued overlays.
  bool HasQueuedOverlays() const;
  // Returns whether an overlay is curently started.
  bool IsShowingOverlay() const;
  // Cancels all queued overlays for this queue.  If one is being displayed, it
  // will also be stopped
  void CancelOverlays();

  // Some OverlayQueues require that a particular WebState's content area is
  // visible before its queued BrowserCoordinators can be started.  If this
  // queue's overlays require showing a WebState, this function will return that
  // WebState.
  virtual web::WebState* GetWebState() const;

 protected:
  // Adds |overlay_coordinator| to the queue and schedules its presentation.
  void AddOverlay(OverlayCoordinator* overlay_coordinator);
  // Returns the number of overlays in the queue.
  NSUInteger GetCount() const;
  // Returns the first BrowserCoordinator in the queue.
  OverlayCoordinator* GetFirstOverlay();
  // Called when the first overlay in the queue is started.
  void OverlayWasStarted();
  // Default constructor.
  OverlayQueue();

 private:
  // The observers for this queue.
  base::ObserverList<OverlayQueueObserver> observers_;
  // The queue of overlays that were added for this WebState.
  __strong NSMutableArray<OverlayCoordinator*>* overlays_;
  // Whether an overlay is currently started.  If this is true, the first
  // BrowserCoordinator in |overlays_| has been started.
  bool showing_overlay_;

  DISALLOW_COPY_AND_ASSIGN(OverlayQueue);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_H_
