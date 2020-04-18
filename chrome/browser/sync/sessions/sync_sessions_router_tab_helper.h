// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_SESSIONS_SYNC_SESSIONS_ROUTER_TAB_HELPER_H_
#define CHROME_BROWSER_SYNC_SESSIONS_SYNC_SESSIONS_ROUTER_TAB_HELPER_H_

#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace sync_sessions {

class SyncSessionsWebContentsRouter;

// TabHelper class that forwards tab-level WebContentsObserver events to a
// (per-profile) sessions router. The router is responsible for forwarding
// these events to sessions sync. This class also tracks the source tab id
// of its corresponding tab, if available.
// A TabHelper is a WebContentsObserver tied to the top level WebContents for a
// browser tab.
// https://chromium.googlesource.com/chromium/src/+/master/docs/tab_helpers.md
class SyncSessionsRouterTabHelper
    : public content::WebContentsUserData<SyncSessionsRouterTabHelper>,
      public content::WebContentsObserver {
 public:
  ~SyncSessionsRouterTabHelper() override;

  static void CreateForWebContents(
      content::WebContents* web_contents,
      SyncSessionsWebContentsRouter* session_router);

  // WebContentsObserver implementation.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void WebContentsDestroyed() override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidOpenRequestedURL(content::WebContents* new_contents,
                           content::RenderFrameHost* source_render_frame_host,
                           const GURL& url,
                           const content::Referrer& referrer,
                           WindowOpenDisposition disposition,
                           ui::PageTransition transition,
                           bool started_from_context_menu,
                           bool renderer_initiated) override;

  // Sets the source tab id for the given child WebContents to the id of the
  // WebContents that owns this helper.
  void SetSourceTabIdForChild(content::WebContents* child_contents);

  // Get the tab id of the tab responsible for creating the tab this helper
  // corresponds to. Returns an invalid ID if there is no such tab.
  SessionID source_tab_id() const { return source_tab_id_; }

 private:
  friend class content::WebContentsUserData<SyncSessionsRouterTabHelper>;

  explicit SyncSessionsRouterTabHelper(content::WebContents* web_contents,
                                       SyncSessionsWebContentsRouter* router);

  // Set the tab id of the tab reponsible for creating the tab this helper
  // corresponds to.
  void set_source_tab_id(SessionID id) { source_tab_id_ = id; }

  void NotifyRouter(bool page_load_completed = false);

  // |router_| is a KeyedService and is guaranteed to outlive |this|.
  SyncSessionsWebContentsRouter* router_;
  // Tab id of the tab from which this tab was created. Example events that
  // create this relationship:
  // * From context menu, "Open link in new tab".
  // * From context menu, "Open link in new window".
  // * Ctrl-click.
  // * Click on a link with target='_blank'.
  SessionID source_tab_id_;

  DISALLOW_COPY_AND_ASSIGN(SyncSessionsRouterTabHelper);
};

}  // namespace sync_sessions

#endif  // CHROME_BROWSER_SYNC_SESSIONS_SYNC_SESSIONS_ROUTER_TAB_HELPER_H_
