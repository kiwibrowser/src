// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/sync/sessions/ios_chrome_local_session_event_router.h"

#include <stddef.h>

#include "base/logging.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "components/sync_sessions/synced_tab_delegate.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/sync/glue/sync_start_util.h"
#include "ios/chrome/browser/sync/ios_chrome_synced_tab_delegate.h"
#include "ios/chrome/browser/tab_parenting_global_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

sync_sessions::SyncedTabDelegate* GetSyncedTabDelegateFromWebState(
    web::WebState* web_state) {
  sync_sessions::SyncedTabDelegate* delegate =
      IOSChromeSyncedTabDelegate::FromWebState(web_state);
  return delegate;
}

}  // namespace

IOSChromeLocalSessionEventRouter::IOSChromeLocalSessionEventRouter(
    ios::ChromeBrowserState* browser_state,
    sync_sessions::SyncSessionsClient* sessions_client,
    const syncer::SyncableService::StartSyncFlare& flare)
    : handler_(NULL),
      browser_state_(browser_state),
      sessions_client_(sessions_client),
      flare_(flare) {
  tab_parented_subscription_ =
      TabParentingGlobalObserver::GetInstance()->RegisterCallback(
          base::Bind(&IOSChromeLocalSessionEventRouter::OnTabParented,
                     base::Unretained(this)));

  history::HistoryService* history_service =
      ios::HistoryServiceFactory::GetForBrowserState(
          browser_state, ServiceAccessType::EXPLICIT_ACCESS);
  if (history_service) {
    favicon_changed_subscription_ = history_service->AddFaviconsChangedCallback(
        base::Bind(&IOSChromeLocalSessionEventRouter::OnFaviconsChanged,
                   base::Unretained(this)));
  }
}

IOSChromeLocalSessionEventRouter::~IOSChromeLocalSessionEventRouter() {}

void IOSChromeLocalSessionEventRouter::NavigationItemsPruned(
    web::WebState* web_state,
    size_t pruned_item_count) {
  OnWebStateChange(web_state);
}

void IOSChromeLocalSessionEventRouter::NavigationItemChanged(
    web::WebState* web_state) {
  OnWebStateChange(web_state);
}

void IOSChromeLocalSessionEventRouter::NavigationItemCommitted(
    web::WebState* web_state,
    const web::LoadCommittedDetails& load_details) {
  OnWebStateChange(web_state);
}

void IOSChromeLocalSessionEventRouter::PageLoaded(
    web::WebState* web_state,
    web::PageLoadCompletionStatus load_completion_status) {
  OnWebStateChange(web_state);
}

void IOSChromeLocalSessionEventRouter::WebStateDestroyed(
    web::WebState* web_state) {
  OnWebStateChange(web_state);
}

void IOSChromeLocalSessionEventRouter::OnTabParented(web::WebState* web_state) {
  OnWebStateChange(web_state);
}

void IOSChromeLocalSessionEventRouter::OnWebStateChange(
    web::WebState* web_state) {
  if (web_state->GetBrowserState() != browser_state_)
    return;
  sync_sessions::SyncedTabDelegate* tab =
      GetSyncedTabDelegateFromWebState(web_state);
  if (!tab)
    return;
  if (handler_)
    handler_->OnLocalTabModified(tab);
  if (!tab->ShouldSync(sessions_client_))
    return;

  if (!flare_.is_null()) {
    flare_.Run(syncer::SESSIONS);
    flare_.Reset();
  }
}

void IOSChromeLocalSessionEventRouter::OnFaviconsChanged(
    const std::set<GURL>& page_urls,
    const GURL& icon_url) {
  if (handler_ && !page_urls.empty())
    handler_->OnFaviconsChanged(page_urls, icon_url);
}

void IOSChromeLocalSessionEventRouter::StartRoutingTo(
    sync_sessions::LocalSessionEventHandler* handler) {
  DCHECK(!handler_);
  handler_ = handler;
}

void IOSChromeLocalSessionEventRouter::Stop() {
  handler_ = nullptr;
}
