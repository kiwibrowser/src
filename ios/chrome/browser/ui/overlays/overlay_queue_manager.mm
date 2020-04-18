// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/overlay_queue_manager.h"

#import "ios/chrome/browser/ui/browser_list/browser_list.h"
#import "ios/chrome/browser/ui/overlays/browser_overlay_queue.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue_manager_observer.h"
#import "ios/chrome/browser/ui/overlays/web_state_overlay_queue.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_BROWSER_USER_DATA_KEY(OverlayQueueManager);

OverlayQueueManager::OverlayQueueManager(Browser* browser)
    : web_state_list_(&browser->web_state_list()) {
  // Create the BrowserOverlayQueue and add it to |queues_|.
  BrowserOverlayQueue::CreateForBrowser(browser);
  queues_.insert(BrowserOverlayQueue::FromBrowser(browser));
  // Add queues for each WebState in |web_state_list_|.
  for (int i = 0; i < web_state_list_->count(); ++i) {
    AddQueueForWebState(web_state_list_->GetWebStateAt(i));
  }
  // Add as an observer for |web_state_list_|.  This allows the manager to
  // create OverlayQueues for newly added WebStates.
  web_state_list_->AddObserver(this);
}

OverlayQueueManager::~OverlayQueueManager() {
  // Disconnect() removes the manager as an observer for |web_state_list_|, nils
  // out |web_state_list_|, and removes the OverlayQueues from |queues_|.  It is
  // expected to be called before deallocation.
  DCHECK(queues_.empty());
  DCHECK(!web_state_list_);
}

#pragma mark - Public

void OverlayQueueManager::AddObserver(OverlayQueueManagerObserver* observer) {
  observers_.AddObserver(observer);
}

void OverlayQueueManager::RemoveObserver(
    OverlayQueueManagerObserver* observer) {
  observers_.RemoveObserver(observer);
}

void OverlayQueueManager::Disconnect() {
  for (auto& observer : observers_) {
    for (OverlayQueue* queue : queues_) {
      observer.OverlayQueueManagerWillRemoveQueue(this, queue);
    }
  }
  queues_.clear();
  web_state_list_->RemoveObserver(this);
  web_state_list_ = nullptr;
}

#pragma mark - WebStateListObserver

void OverlayQueueManager::WebStateInsertedAt(WebStateList* web_state_list,
                                             web::WebState* web_state,
                                             int index,
                                             bool activating) {
  AddQueueForWebState(web_state);
}

void OverlayQueueManager::WebStateReplacedAt(WebStateList* web_state_list,
                                             web::WebState* old_web_state,
                                             web::WebState* new_web_state,
                                             int index) {
  RemoveQueueForWebState(old_web_state);
  AddQueueForWebState(new_web_state);
}

void OverlayQueueManager::WebStateDetachedAt(WebStateList* web_state_list,
                                             web::WebState* web_state,
                                             int index) {
  RemoveQueueForWebState(web_state);
}

#pragma mark -

void OverlayQueueManager::AddQueueForWebState(web::WebState* web_state) {
  WebStateOverlayQueue::CreateForWebState(web_state);
  OverlayQueue* queue = WebStateOverlayQueue::FromWebState(web_state);
  if (queues_.insert(queue).second) {
    for (auto& observer : observers_) {
      observer.OverlayQueueManagerDidAddQueue(this, queue);
    }
  }
}

void OverlayQueueManager::RemoveQueueForWebState(web::WebState* web_state) {
  OverlayQueue* queue = WebStateOverlayQueue::FromWebState(web_state);
  auto queue_iter = queues_.find(queue);
  DCHECK(queue_iter != queues_.end());
  for (auto& observer : observers_) {
    observer.OverlayQueueManagerWillRemoveQueue(this, queue);
  }
  queues_.erase(queue_iter);
}
