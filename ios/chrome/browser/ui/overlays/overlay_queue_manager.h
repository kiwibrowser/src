// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_MANAGER_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_MANAGER_H_

#include <Foundation/Foundation.h>
#include <set>

#include "base/macros.h"
#include "base/observer_list.h"
#import "ios/chrome/browser/ui/browser_list/browser_user_data.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer.h"

class OverlayQueue;
class OverlayQueueManagerObserver;
class WebStateList;
namespace web {
class WebState;
}

// Object that manages the creation of OverlayQueues for a Browser and all its
// associated WebStates.
class OverlayQueueManager : public BrowserUserData<OverlayQueueManager>,
                            public WebStateListObserver {
 public:
  ~OverlayQueueManager() override;

  // The WebStateList for which OverlayQueues are being managed.
  WebStateList* web_state_list() { return web_state_list_; }

  // The OverlayQueues managed by this object.
  const std::set<OverlayQueue*>& queues() const { return queues_; }

  // Adds and removes observers.
  void AddObserver(OverlayQueueManagerObserver* observer);
  void RemoveObserver(OverlayQueueManagerObserver* observer);

  // Tells the OverlayQueueManager to disconnect itself as an observer before
  // deallocation.
  void Disconnect();

 private:
  friend class BrowserUserData<OverlayQueueManager>;

  // Private constructor used by factory method.
  explicit OverlayQueueManager(Browser* browser);

  // WebStateListObserver:
  void WebStateInsertedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index,
                          bool activating) override;
  void WebStateReplacedAt(WebStateList* web_state_list,
                          web::WebState* old_web_state,
                          web::WebState* new_web_state,
                          int index) override;
  void WebStateDetachedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index) override;

  // Lazily creates an OverlayQueue for |web_state| and adds it to |queues_|.
  void AddQueueForWebState(web::WebState* web_state);

  // Removes the queue that was created in response to WebStateListObserver
  // callbacks and notifies its observers.
  void RemoveQueueForWebState(web::WebState* web_state);

  // The observers for this manager.
  base::ObserverList<OverlayQueueManagerObserver> observers_;
  // The OverlayQueues created by this manager.
  std::set<OverlayQueue*> queues_;
  // The WebStateList for which queues are beign managed.
  WebStateList* web_state_list_;

  DISALLOW_COPY_AND_ASSIGN(OverlayQueueManager);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_QUEUE_MANAGER_H_
