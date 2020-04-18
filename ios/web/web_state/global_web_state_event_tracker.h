// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEB_STATE_GLOBAL_WEB_STATE_EVENT_TRACKER_H_
#define IOS_WEB_WEB_STATE_GLOBAL_WEB_STATE_EVENT_TRACKER_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/scoped_observer.h"
#include "ios/web/public/web_state/global_web_state_observer.h"
#include "ios/web/public/web_state/web_state_observer.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace web {

// This singleton serves as the mechanism via which GlobalWebStateObservers get
// informed of relevant events from all WebState instances.
class GlobalWebStateEventTracker : public WebStateObserver {
 public:
  // Returns the instance of GlobalWebStateEventTracker.
  static GlobalWebStateEventTracker* GetInstance();

  // Adds/removes observers.
  void AddObserver(GlobalWebStateObserver* observer);
  void RemoveObserver(GlobalWebStateObserver* observer);

 private:
  friend struct base::DefaultSingletonTraits<GlobalWebStateEventTracker>;
  friend class WebStateEventForwarder;
  friend class WebStateImpl;

  // Should be called whenever a WebState instance is created.
  void OnWebStateCreated(WebState* web_state);

  // WebStateObserver implementation.
  void NavigationItemsPruned(WebState* web_state,
                             size_t pruned_item_count) override;
  void NavigationItemChanged(WebState* web_state) override;
  void NavigationItemCommitted(
      WebState* web_state,
      const LoadCommittedDetails& load_details) override;
  void DidStartNavigation(WebState* web_state,
                          NavigationContext* navigation_context) override;
  void DidStartLoading(WebState* web_state) override;
  void DidStopLoading(WebState* web_state) override;
  void PageLoaded(WebState* web_state,
                  PageLoadCompletionStatus load_completion_status) override;
  void RenderProcessGone(WebState* web_state) override;
  void WebStateDestroyed(WebState* web_state) override;

  GlobalWebStateEventTracker();
  ~GlobalWebStateEventTracker() override;

  // ScopedObserver used to track registration with WebState.
  ScopedObserver<WebState, WebStateObserver> scoped_observer_;

  // List of observers currently registered with the tracker.
  base::ObserverList<GlobalWebStateObserver, true> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(GlobalWebStateEventTracker);
};

}  // namespace web

#endif  // IOS_WEB_WEB_STATE_GLOBAL_WEB_STATE_EVENT_TRACKER_H_
