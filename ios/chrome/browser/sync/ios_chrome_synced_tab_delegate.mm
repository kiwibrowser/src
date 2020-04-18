// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/sync/ios_chrome_synced_tab_delegate.h"

#include "base/memory/ref_counted.h"
#include "components/sessions/ios/ios_serialized_navigation_builder.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "components/sync_sessions/synced_window_delegate.h"
#include "components/sync_sessions/synced_window_delegates_getter.h"
#include "components/sync_sessions/tab_node_pool.h"
#include "ios/chrome/browser/sessions/ios_chrome_session_tab_helper.h"
#include "ios/web/public/favicon_status.h"
#include "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::NavigationItem;

DEFINE_WEB_STATE_USER_DATA_KEY(IOSChromeSyncedTabDelegate);

namespace {

// Helper to access the correct NavigationItem, accounting for pending entries.
NavigationItem* GetPossiblyPendingItemAtIndex(web::WebState* web_state, int i) {
  int pending_index = web_state->GetNavigationManager()->GetPendingItemIndex();
  return (pending_index == i)
             ? web_state->GetNavigationManager()->GetPendingItem()
             : web_state->GetNavigationManager()->GetItemAtIndex(i);
}

}  // namespace

IOSChromeSyncedTabDelegate::IOSChromeSyncedTabDelegate(web::WebState* web_state)
    : web_state_(web_state),
      sync_session_id_(sync_sessions::TabNodePool::kInvalidTabNodeID) {}

IOSChromeSyncedTabDelegate::~IOSChromeSyncedTabDelegate() {}

SessionID IOSChromeSyncedTabDelegate::GetWindowId() const {
  return IOSChromeSessionTabHelper::FromWebState(web_state_)->window_id();
}

SessionID IOSChromeSyncedTabDelegate::GetSessionId() const {
  return IOSChromeSessionTabHelper::FromWebState(web_state_)->session_id();
}

bool IOSChromeSyncedTabDelegate::IsBeingDestroyed() const {
  return web_state_->IsBeingDestroyed();
}

// todo(pnoland): add logic to store and return the source tab id on ios.
// http://crbug/695241
SessionID IOSChromeSyncedTabDelegate::GetSourceTabID() const {
  return SessionID::InvalidValue();
}

std::string IOSChromeSyncedTabDelegate::GetExtensionAppId() const {
  return std::string();
}

bool IOSChromeSyncedTabDelegate::IsInitialBlankNavigation() const {
  return web_state_->GetNavigationManager()->GetItemCount() == 0;
}

int IOSChromeSyncedTabDelegate::GetCurrentEntryIndex() const {
  return web_state_->GetNavigationManager()->GetLastCommittedItemIndex();
}

int IOSChromeSyncedTabDelegate::GetEntryCount() const {
  return web_state_->GetNavigationManager()->GetItemCount();
}

GURL IOSChromeSyncedTabDelegate::GetVirtualURLAtIndex(int i) const {
  NavigationItem* item = GetPossiblyPendingItemAtIndex(web_state_, i);
  return item ? item->GetVirtualURL() : GURL();
}

GURL IOSChromeSyncedTabDelegate::GetFaviconURLAtIndex(int i) const {
  NavigationItem* item = GetPossiblyPendingItemAtIndex(web_state_, i);
  return (item->GetFavicon().valid ? item->GetFavicon().url : GURL());
}

ui::PageTransition IOSChromeSyncedTabDelegate::GetTransitionAtIndex(
    int i) const {
  NavigationItem* item = GetPossiblyPendingItemAtIndex(web_state_, i);
  return item->GetTransitionType();
}

void IOSChromeSyncedTabDelegate::GetSerializedNavigationAtIndex(
    int i,
    sessions::SerializedNavigationEntry* serialized_entry) const {
  NavigationItem* item = GetPossiblyPendingItemAtIndex(web_state_, i);
  if (item) {
    *serialized_entry =
        sessions::IOSSerializedNavigationBuilder::FromNavigationItem(i, *item);
  }
}

bool IOSChromeSyncedTabDelegate::ProfileIsSupervised() const {
  return false;
}

const std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>*
IOSChromeSyncedTabDelegate::GetBlockedNavigations() const {
  NOTREACHED();
  return nullptr;
}

bool IOSChromeSyncedTabDelegate::IsPlaceholderTab() const {
  return false;
}

int IOSChromeSyncedTabDelegate::GetSyncId() const {
  return sync_session_id_;
}

void IOSChromeSyncedTabDelegate::SetSyncId(int sync_id) {
  sync_session_id_ = sync_id;
}

bool IOSChromeSyncedTabDelegate::ShouldSync(
    sync_sessions::SyncSessionsClient* sessions_client) {
  if (sessions_client->GetSyncedWindowDelegatesGetter()->FindById(
          GetWindowId()) == nullptr)
    return false;

  if (IsInitialBlankNavigation())
    return false;  // This deliberately ignores a new pending entry.

  int entry_count = GetEntryCount();
  for (int i = 0; i < entry_count; ++i) {
    const GURL& virtual_url = GetVirtualURLAtIndex(i);
    if (!virtual_url.is_valid())
      continue;

    if (sessions_client->ShouldSyncURL(virtual_url))
      return true;
  }
  return false;
}
