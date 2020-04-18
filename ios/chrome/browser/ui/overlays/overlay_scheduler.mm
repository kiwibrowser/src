// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/overlay_scheduler.h"

#include <list>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue_manager.h"
#import "ios/chrome/browser/ui/overlays/overlay_scheduler_observer.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_BROWSER_USER_DATA_KEY(OverlayScheduler);

OverlayScheduler::OverlayScheduler(Browser* browser)
    : web_state_list_(&browser->web_state_list()) {
  // Create the OverlayQueueManager and add the scheduler as an observer to the
  // manager and all its queues.
  OverlayQueueManager::CreateForBrowser(browser);
  queue_manager_ = OverlayQueueManager::FromBrowser(browser);
  queue_manager_->AddObserver(this);
  for (OverlayQueue* queue : queue_manager_->queues()) {
    queue->AddObserver(this);
  }
}

OverlayScheduler::~OverlayScheduler() {
  // Disconnect() removes the scheduler as an observer and nils out
  // |queue_manager_|.  It is expected to be called before deallocation.
  DCHECK(!queue_manager_);
}

#pragma mark - Public

void OverlayScheduler::AddObserver(OverlaySchedulerObserver* observer) {
  observers_.AddObserver(observer);
}

void OverlayScheduler::RemoveObserver(OverlaySchedulerObserver* observer) {
  observers_.RemoveObserver(observer);
}

void OverlayScheduler::SetQueueManager(OverlayQueueManager* queue_manager) {
  if (queue_manager_) {
    queue_manager_->RemoveObserver(this);
    for (OverlayQueue* queue : queue_manager_->queues()) {
      StopObservingQueue(queue);
    }
  }
  queue_manager_ = queue_manager;
  if (queue_manager_) {
    queue_manager_->AddObserver(this);
    for (OverlayQueue* queue : queue_manager_->queues()) {
      queue->AddObserver(this);
    }
  }
}

bool OverlayScheduler::IsShowingOverlay() const {
  return !overlay_queues_.empty() &&
         overlay_queues_.front()->IsShowingOverlay();
}

void OverlayScheduler::SetPaused(bool paused) {
  if (paused_ == paused)
    return;
  paused_ = paused;
  if (!paused_)
    TryToStartNextOverlay();
}

void OverlayScheduler::ReplaceVisibleOverlay(
    OverlayCoordinator* overlay_coordinator) {
  DCHECK(overlay_coordinator);
  DCHECK(IsShowingOverlay());
  overlay_queues_.front()->ReplaceVisibleOverlay(overlay_coordinator);
}

void OverlayScheduler::CancelOverlays() {
  // |overlay_queues_| will be updated in OverlayQueueDidCancelOverlays(), so a
  // while loop is used to avoid using invalidated iterators.
  while (!overlay_queues_.empty()) {
    overlay_queues_.front()->CancelOverlays();
  }
}

void OverlayScheduler::Disconnect() {
  queue_manager_->RemoveObserver(this);
  for (OverlayQueue* queue : queue_manager_->queues()) {
    StopObservingQueue(queue);
  }
  queue_manager_->Disconnect();
  queue_manager_ = nullptr;
}

#pragma mark - OverlayQueueManagerObserver

void OverlayScheduler::OverlayQueueManagerDidAddQueue(
    OverlayQueueManager* manager,
    OverlayQueue* queue) {
  queue->AddObserver(this);
}

void OverlayScheduler::OverlayQueueManagerWillRemoveQueue(
    OverlayQueueManager* manager,
    OverlayQueue* queue) {
  StopObservingQueue(queue);
}

#pragma mark - OverlayQueueObserver

void OverlayScheduler::OverlayQueueDidAddOverlay(OverlayQueue* queue) {
  DCHECK(queue);
  overlay_queues_.push_back(queue);
  TryToStartNextOverlay();
}

void OverlayScheduler::OverlayQueueWillReplaceVisibleOverlay(
    OverlayQueue* queue) {
  DCHECK(queue);
  DCHECK_EQ(overlay_queues_.front(), queue);
  DCHECK(queue->IsShowingOverlay());
  // An OverlayQueue's visible overlay can only be replaced if it's the first
  // queue in the scheduler and is already showing an overlay.  The queue is
  // added here to schedule its replacement overlay can be displayed when its
  // currently-visible overlay is stopped.
  overlay_queues_.push_front(queue);
}

void OverlayScheduler::OverlayQueueDidStopVisibleOverlay(OverlayQueue* queue) {
  DCHECK(!overlay_queues_.empty());
  DCHECK_EQ(overlay_queues_.front(), queue);
  // Only the first queue in the scheduler can start overlays, so it is expected
  // that this function is only called for that queue.
  overlay_queues_.pop_front();
  TryToStartNextOverlay();
}

void OverlayScheduler::OverlayQueueDidCancelOverlays(OverlayQueue* queue) {
  DCHECK(queue);
  // Remove all scheduled instances of |queue| from the |overlay_queues_|.
  auto i = overlay_queues_.begin();
  while (i != overlay_queues_.end()) {
    if (*i == queue)
      overlay_queues_.erase(i);
  }
  // If |queue| is currently showing an overlay, prepend it to
  // |overlay_queues_|.  It will be removed when the cancelled overlay is
  // stopped.
  if (queue->IsShowingOverlay())
    overlay_queues_.push_front(queue);
}

#pragma mark - WebStateObserver

void OverlayScheduler::WasShown(web::WebState* web_state) {
  StopObservingWebState(web_state);
}

void OverlayScheduler::WebStateDestroyed(web::WebState* web_state) {
  StopObservingWebState(web_state);
}

#pragma mark -

void OverlayScheduler::TryToStartNextOverlay() {
  // Overlays cannot be started if:
  // - the service is paused,
  // - there are no overlays to display,
  // - an overlay is already being displayed,
  // - an overlay is already scheduled to be started once a specific WebState
  //   has been shown.
  if (paused_ || overlay_queues_.empty() || IsShowingOverlay() ||
      observing_front_web_state_) {
    return;
  }
  OverlayQueue* queue = overlay_queues_.front();
  web::WebState* web_state = queue->GetWebState();
  // Create a WebStateVisibilityObserver if |web_state| needs to be activated.
  if (web_state && (web_state_list_->GetActiveWebState() != web_state ||
                    !web_state->IsVisible())) {
    web_state->AddObserver(this);
    observing_front_web_state_ = true;
  }
  // Notify the observers that an overlay will be shown for |web_state|.  If
  // |web_state| isn't nil, this callback is expected to update the active
  // WebState and show its content area.
  for (auto& observer : observers_) {
    observer.OverlaySchedulerWillShowOverlay(this, web_state);
  }
  // If |web_state| was nil or already activated, then start |queue|'s next
  // overlay.  Otherwise, it will be started in OnWebStateShown().  The check
  // for IsShowingOverlay() is to prevent calling StartNextOverlay() twice in
  // the event that WasShown() was called directly from the observer callbacks
  // above.
  if (!observing_front_web_state_ && !IsShowingOverlay()) {
    queue->StartNextOverlay();
  }
}

void OverlayScheduler::StopObservingQueue(OverlayQueue* queue) {
  queue->RemoveObserver(this);
  queue->CancelOverlays();
}

void OverlayScheduler::StopObservingWebState(web::WebState* web_state) {
  DCHECK(observing_front_web_state_);
  DCHECK(web_state);
  DCHECK_EQ(overlay_queues_.front()->GetWebState(), web_state);
  web_state->RemoveObserver(this);
  observing_front_web_state_ = false;
  TryToStartNextOverlay();
}
