// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEB_STATE_GLOBAL_WEB_STATE_OBSERVER_H_
#define IOS_WEB_PUBLIC_WEB_STATE_GLOBAL_WEB_STATE_OBSERVER_H_

#include <stddef.h>

#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"

namespace web {

struct LoadCommittedDetails;
class NavigationContext;
class WebState;

// An observer API implemented by classes that are interested in various
// WebState-related events from all WebStates. GlobalWebStateObservers will
// receive callbacks for the duration of their lifetime.
// WARNING: This class exists only to mimic //chrome-specific flows that
// listen for notifications from all WebContents or NavigationController
// instances. Do not add new subclasses of this class for any other reason.
// DEPRECATED. Use TabHelper to observe WebState creation. Use WebStateObserver
// to observe WebState changes.
// TODO(crbug.com/782269): Remove this class.
class GlobalWebStateObserver {
 public:
  // DEPRECATED. Use WebStateObserver's |NavigationItemsPruned| instead.
  // Called when navigation items have been pruned in |web_state|.
  // TODO(crbug.com/782269): Remove this method.
  virtual void NavigationItemsPruned(WebState* web_state,
                                     size_t pruned_item_count) {}

  // Called when a navigation item has changed in |web_state|.
  // DEPRECATED. Use WebStateObserver's |TitleWasSet| to listen for title
  // changes and |DidFinishNavigation| for |window.location.replace|.
  // TODO(crbug.com/782269): Remove this method.
  virtual void NavigationItemChanged(WebState* web_state) {}

  // Called when a navigation item has been committed in |web_state|.
  // DEPRECATED. Use WebStateObserver's |DidFinishNavigation| instead.
  // TODO(crbug.com/782269): Remove this method.
  virtual void NavigationItemCommitted(
      WebState* web_state,
      const LoadCommittedDetails& load_details) {}

  // Called when |web_state| has started loading a page.
  // DEPRECATED. Use WebStateObserver's |DidStartLoading| instead.
  // TODO(crbug.com/782269): Remove this method.
  virtual void WebStateDidStartLoading(WebState* web_state) {}

  // Called when |web_state| has stopped loading a page.
  // DEPRECATED. Use WebStateObserver's |DidStopLoading| instead.
  // TODO(crbug.com/782269): Remove this method.
  virtual void WebStateDidStopLoading(WebState* web_state) {}

  // Called when a navigation started in |web_state| for the main frame.
  // DEPRECATED. Use WebStateObserver's |DidStartNavigation| instead.
  // TODO(crbug.com/782269): Remove this method.
  virtual void WebStateDidStartNavigation(
      WebState* web_state,
      NavigationContext* navigation_context) {}

  // Called when the current page is loaded in |web_state|.
  // DEPRECATED. Use WebStateObserver's |PageLoaded| instead.
  // TODO(crbug.com/782269): Remove this method.
  virtual void PageLoaded(WebState* web_state,
                          PageLoadCompletionStatus load_completion_status) {}

  // Called when the web process is terminated (usually by crashing, though
  // possibly by other means).
  // DEPRECATED. Use WebStateObserver's |RendererProcessGone| instead.
  // TODO(crbug.com/782269): Remove this method.
  virtual void RenderProcessGone(WebState* web_state) {}

  // Called when |web_state| is being destroyed.
  // DEPRECATED. Use WebStateObserver's |WebStateDestroyed| instead.
  // TODO(crbug.com/782269): Remove this method.
  virtual void WebStateDestroyed(WebState* web_state) {}

 protected:
  GlobalWebStateObserver();
  virtual ~GlobalWebStateObserver();

 private:
  DISALLOW_COPY_AND_ASSIGN(GlobalWebStateObserver);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_STATE_GLOBAL_WEB_STATE_OBSERVER_H_
