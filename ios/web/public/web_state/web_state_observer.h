// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEB_STATE_WEB_STATE_OBSERVER_H_
#define IOS_WEB_PUBLIC_WEB_STATE_WEB_STATE_OBSERVER_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "base/macros.h"

namespace web {

struct FaviconURL;
struct FormActivityParams;
class NavigationContext;
struct LoadCommittedDetails;
class WebState;

enum class PageLoadCompletionStatus : bool { SUCCESS = 0, FAILURE = 1 };

// An observer API implemented by classes which are interested in various page
// load events from WebState.
class WebStateObserver {
 public:
  virtual ~WebStateObserver();

  // These methods are invoked every time the WebState changes visibility.
  virtual void WasShown(WebState* web_state) {}
  virtual void WasHidden(WebState* web_state) {}

  // This method is invoked when committed navigation items have been pruned.
  virtual void NavigationItemsPruned(WebState* web_state,
                                     size_t pruned_item_count) {}

  // This method is invoked either when NavigationItem's titile did change or
  // when window.location.replace JavaScript API was called.
  // DEPRECATED. Use |TitleWasSet| to listen for title changes and
  // |DidFinishNavigation| for |window.location.replace|.
  // TODO(crbug.com/720786): Remove this method.
  virtual void NavigationItemChanged(WebState* web_state) {}

  // This method is invoked when a new non-pending navigation item is created.
  // This corresponds to one NavigationManager item being created
  // (in the case of new navigations) or renavigated to (for back/forward
  // navigations).
  // DEPRECATED. Use |DidFinishNavigation| to listen for
  // "navigation item committed" signals.
  // TODO(crbug.com/720786): Remove this method.
  virtual void NavigationItemCommitted(
      WebState* web_state,
      const LoadCommittedDetails& load_details) {}

  // Called when a navigation started in the WebState for the main frame.
  // |navigation_context| is unique to a specific navigation. The same
  // NavigationContext will be provided on subsequent call to
  // DidFinishNavigation() when related to this navigation. Observers should
  // clear any references to |navigation_context| in DidFinishNavigation(), just
  // before it is destroyed.
  //
  // This is also fired by same-document navigations, such as fragment
  // navigations or pushState/replaceState, which will not result in a document
  // change. To filter these out, use NavigationContext::IsSameDocument().
  //
  // More than one navigation can be ongoing in the same frame at the same
  // time. Each will get its own NavigationContext.
  //
  // There is no guarantee that DidFinishNavigation() will be called for any
  // particular navigation before DidStartNavigation is called on the next.
  virtual void DidStartNavigation(WebState* web_state,
                                  NavigationContext* navigation_context) {}

  // Called when a navigation finished in the WebState for the main frame. This
  // happens when a navigation is committed, aborted or replaced by a new one.
  // To know if the navigation has resulted in an error page, use
  // NavigationContext::GetError().
  //
  // If this is called because the navigation committed, then the document load
  // will still be ongoing in the WebState returned by |navigation_context|.
  // Use the document loads events such as DidStopLoading
  // and related methods to listen for continued events from this
  // WebState.
  //
  // This is also fired by same-document navigations, such as fragment
  // navigations or pushState/replaceState, which will not result in a document
  // change. To filter these out, use NavigationContext::IsSameDocument().
  //
  // |navigation_context| will be destroyed at the end of this call, so do not
  // keep a reference to it afterward.
  virtual void DidFinishNavigation(WebState* web_state,
                                   NavigationContext* navigation_context) {}

  // Called when the current WebState has started or stopped loading. This is
  // not correlated with the document load phase of the main frame, but rather
  // represents the load of the web page as a whole. Clients should present
  // network activity indicator UI to the user when DidStartLoading is called
  // and UI when DidStopLoading is called. DidStartLoading is a different event
  // than DidStartNavigation and clients shold not assume that these two
  // callbacks always called in pair or in a specific order (same true for
  // DidFinishNavigation/DidFinishLoading). "Navigation" is about fetching the
  // new document content and committing it as a new document, and "Loading"
  // continues well after that. "Loading" callbacks are not called for fragment
  // change navigations, but called for other same-document navigations
  // (crbug.com/767092).
  virtual void DidStartLoading(WebState* web_state) {}
  virtual void DidStopLoading(WebState* web_state) {}

  // Called when the current page has finished the loading of the main frame
  // document (including same-document navigations). DidStopLoading relates to
  // the general loading state of the WebState, but PageLoaded is correlated
  // with the main frame document load phase. Unlike DidStopLoading, this
  // callback is not called when the load is aborted (WebState::Stop is called
  // or the load is rejected via WebStatePolicyDecider (both ShouldAllowRequest
  // or ShouldAllowResponse). If PageLoaded is called it is always called after
  // DidFinishNavigation.
  virtual void PageLoaded(WebState* web_state,
                          PageLoadCompletionStatus load_completion_status) {}

  // Notifies the observer that the page has made some progress loading.
  // |progress| is a value between 0.0 (nothing loaded) to 1.0 (page fully
  // loaded).
  virtual void LoadProgressChanged(WebState* web_state, double progress) {}

  // Called when the canGoBack / canGoForward state of the window was changed.
  virtual void DidChangeBackForwardState(WebState* web_state) {}

  // Called when the title of the WebState is set.
  virtual void TitleWasSet(WebState* web_state) {}

  // Called when the visible security state of the page changes.
  virtual void DidChangeVisibleSecurityState(WebState* web_state) {}

  // Called when a JavaScript dialog or window open request was suppressed.
  // NOTE: Called only if WebState::SetShouldSuppressDialogs() was called with
  // false.
  virtual void DidSuppressDialog(WebState* web_state) {}

  // Called on form submission in the main frame or in a same-origin iframe.
  // |user_initiated| is true if the user interacted with the page.
  // |is_main_frame| is true if the submitted form is in the main frame.
  // TODO(crbug.com/823285): move this handler to components/autofill.
  virtual void DocumentSubmitted(WebState* web_state,
                                 const std::string& form_name,
                                 bool user_initiated,
                                 bool is_main_frame) {}

  // Called when the user is typing on a form field in the main frame or in a
  // same-origin iframe. |params.input_missing| is indicating if there is any
  // error when parsing the form field information.
  // TODO(crbug.com/823285): move this handler to components/autofill.
  virtual void FormActivityRegistered(WebState* web_state,
                                      const FormActivityParams& params) {}

  // Invoked when new favicon URL candidates are received.
  virtual void FaviconUrlUpdated(WebState* web_state,
                                 const std::vector<FaviconURL>& candidates) {}

  // Called when the web process is terminated (usually by crashing, though
  // possibly by other means).
  virtual void RenderProcessGone(WebState* web_state) {}

  // Invoked when the WebState is being destroyed. Gives subclasses a chance
  // to cleanup.
  virtual void WebStateDestroyed(WebState* web_state) {}

 protected:
  WebStateObserver();

 private:
  DISALLOW_COPY_AND_ASSIGN(WebStateObserver);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_STATE_WEB_STATE_OBSERVER_H_
