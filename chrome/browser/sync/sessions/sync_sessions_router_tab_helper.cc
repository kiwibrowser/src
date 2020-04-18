// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sessions/sync_sessions_router_tab_helper.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/sync/sessions/sync_sessions_web_contents_router.h"
#include "components/sync_sessions/synced_tab_delegate.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(sync_sessions::SyncSessionsRouterTabHelper);

namespace sync_sessions {

// static
void SyncSessionsRouterTabHelper::CreateForWebContents(
    content::WebContents* web_contents,
    SyncSessionsWebContentsRouter* router) {
  DCHECK(web_contents);
  if (!FromWebContents(web_contents)) {
    web_contents->SetUserData(UserDataKey(),
                              base::WrapUnique(new SyncSessionsRouterTabHelper(
                                  web_contents, router)));
  }
}

SyncSessionsRouterTabHelper::SyncSessionsRouterTabHelper(
    content::WebContents* web_contents,
    SyncSessionsWebContentsRouter* router)
    : content::WebContentsObserver(web_contents),
      router_(router),
      source_tab_id_(SessionID::InvalidValue()) {}

SyncSessionsRouterTabHelper::~SyncSessionsRouterTabHelper() {}

void SyncSessionsRouterTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle && navigation_handle->IsInMainFrame())
    NotifyRouter();
}

void SyncSessionsRouterTabHelper::TitleWasSet(content::NavigationEntry* entry) {
  NotifyRouter();
}

void SyncSessionsRouterTabHelper::WebContentsDestroyed() {
  NotifyRouter();
}

void SyncSessionsRouterTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  // Only notify when the main frame finishes loading; only the main frame
  // doesn't have a parent.
  if (render_frame_host && !render_frame_host->GetParent())
    NotifyRouter(true);
}

void SyncSessionsRouterTabHelper::DidOpenRequestedURL(
    content::WebContents* new_contents,
    content::RenderFrameHost* source_render_frame_host,
    const GURL& url,
    const content::Referrer& referrer,
    WindowOpenDisposition disposition,
    ui::PageTransition transition,
    bool started_from_context_menu,
    bool renderer_initiated) {
  SetSourceTabIdForChild(new_contents);
}

void SyncSessionsRouterTabHelper::SetSourceTabIdForChild(
    content::WebContents* child_contents) {
  SessionID source_tab_id = SessionTabHelper::IdForTab(web_contents());
  if (child_contents &&
      SyncSessionsRouterTabHelper::FromWebContents(child_contents) &&
      child_contents != web_contents() && source_tab_id.is_valid()) {
    SyncSessionsRouterTabHelper::FromWebContents(child_contents)
        ->set_source_tab_id(source_tab_id);
  }
  NotifyRouter();
}

void SyncSessionsRouterTabHelper::NotifyRouter(bool page_load_completed) {
  if (router_)
    router_->NotifyTabModified(web_contents(), page_load_completed);
}

}  // namespace sync_sessions
