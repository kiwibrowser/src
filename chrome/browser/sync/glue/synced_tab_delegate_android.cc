// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/synced_tab_delegate_android.h"

#include "base/memory/ref_counted.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/glue/synced_window_delegates_getter_android.h"
#include "chrome/browser/sync/sessions/sync_sessions_router_tab_helper.h"
#include "chrome/browser/ui/sync/tab_contents_synced_tab_delegate.h"
#include "components/sync_sessions/synced_window_delegate.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"

using content::NavigationEntry;

namespace browser_sync {
SyncedTabDelegateAndroid::SyncedTabDelegateAndroid(TabAndroid* tab_android)
    : web_contents_(nullptr),
      tab_android_(tab_android),
      tab_contents_delegate_(nullptr) {
}

SyncedTabDelegateAndroid::~SyncedTabDelegateAndroid() {}

SessionID SyncedTabDelegateAndroid::GetWindowId() const {
  return tab_contents_delegate_->GetWindowId();
}

SessionID SyncedTabDelegateAndroid::GetSessionId() const {
  return tab_android_->session_id();
}

bool SyncedTabDelegateAndroid::IsBeingDestroyed() const {
  return tab_contents_delegate_->IsBeingDestroyed();
}

SessionID SyncedTabDelegateAndroid::GetSourceTabID() const {
  return tab_contents_delegate_->GetSourceTabID();
}

std::string SyncedTabDelegateAndroid::GetExtensionAppId() const {
  return tab_contents_delegate_->GetExtensionAppId();
}

bool SyncedTabDelegateAndroid::IsInitialBlankNavigation() const {
  return tab_contents_delegate_->IsInitialBlankNavigation();
}

int SyncedTabDelegateAndroid::GetCurrentEntryIndex() const {
  return tab_contents_delegate_->GetCurrentEntryIndex();
}

int SyncedTabDelegateAndroid::GetEntryCount() const {
  return tab_contents_delegate_->GetEntryCount();
}

GURL SyncedTabDelegateAndroid::GetVirtualURLAtIndex(int i) const {
  return tab_contents_delegate_->GetVirtualURLAtIndex(i);
}

GURL SyncedTabDelegateAndroid::GetFaviconURLAtIndex(int i) const {
  return tab_contents_delegate_->GetFaviconURLAtIndex(i);
}

ui::PageTransition SyncedTabDelegateAndroid::GetTransitionAtIndex(int i) const {
  return tab_contents_delegate_->GetTransitionAtIndex(i);
}

void SyncedTabDelegateAndroid::GetSerializedNavigationAtIndex(
    int i,
    sessions::SerializedNavigationEntry* serialized_entry) const {
  tab_contents_delegate_->GetSerializedNavigationAtIndex(i, serialized_entry);
}

bool SyncedTabDelegateAndroid::IsPlaceholderTab() const {
  return web_contents_ == nullptr;
}

void SyncedTabDelegateAndroid::SetWebContents(
    content::WebContents* web_contents,
    content::WebContents* source_web_contents) {
  web_contents_ = web_contents;
  TabContentsSyncedTabDelegate::CreateForWebContents(web_contents_);
  // Store the TabContentsSyncedTabDelegate object that was created.
  tab_contents_delegate_ =
      TabContentsSyncedTabDelegate::FromWebContents(web_contents_);
  if (source_web_contents) {
    sync_sessions::SyncSessionsRouterTabHelper::FromWebContents(
        source_web_contents)
        ->SetSourceTabIdForChild(web_contents_);
  }
}

void SyncedTabDelegateAndroid::ResetWebContents() {
  web_contents_ = nullptr;
}

bool SyncedTabDelegateAndroid::ProfileIsSupervised() const {
  return tab_contents_delegate_->ProfileIsSupervised();
}

const std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>*
SyncedTabDelegateAndroid::GetBlockedNavigations() const {
  return tab_contents_delegate_->GetBlockedNavigations();
}

int SyncedTabDelegateAndroid::GetSyncId() const {
  return tab_android_->GetSyncId();
}

void SyncedTabDelegateAndroid::SetSyncId(int sync_id) {
  tab_android_->SetSyncId(sync_id);
}

bool SyncedTabDelegateAndroid::ShouldSync(
    sync_sessions::SyncSessionsClient* sessions_client) {
  return tab_contents_delegate_->ShouldSync(sessions_client);
}

}  // namespace browser_sync
