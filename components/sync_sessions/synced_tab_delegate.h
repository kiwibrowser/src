// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SESSIONS_SYNCED_TAB_DELEGATE_H__
#define COMPONENTS_SYNC_SESSIONS_SYNCED_TAB_DELEGATE_H__

#include <memory>
#include <string>
#include <vector>

#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/session_id.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace sync_sessions {
class SyncSessionsClient;
}

namespace sync_sessions {

// A SyncedTabDelegate is used to insulate the sync code from depending
// directly on WebContents, NavigationController, and the extensions TabHelper.
class SyncedTabDelegate {
 public:
  virtual ~SyncedTabDelegate();

  // Methods from TabContents.
  virtual SessionID GetWindowId() const = 0;
  virtual SessionID GetSessionId() const = 0;
  virtual bool IsBeingDestroyed() const = 0;

  // Get the tab id of the tab responsible for opening this tab, if applicable.
  // Returns an invalid ID if no such tab relationship is known.
  virtual SessionID GetSourceTabID() const = 0;

  // Method derived from extensions TabHelper.
  virtual std::string GetExtensionAppId() const = 0;

  // Methods from NavigationController.
  virtual bool IsInitialBlankNavigation() const = 0;
  virtual int GetCurrentEntryIndex() const = 0;
  virtual int GetEntryCount() const = 0;
  virtual GURL GetVirtualURLAtIndex(int i) const = 0;
  virtual GURL GetFaviconURLAtIndex(int i) const = 0;
  virtual ui::PageTransition GetTransitionAtIndex(int i) const = 0;
  virtual void GetSerializedNavigationAtIndex(
      int i,
      sessions::SerializedNavigationEntry* serialized_entry) const = 0;

  // Supervised user related methods.
  virtual bool ProfileIsSupervised() const = 0;
  virtual const std::vector<
      std::unique_ptr<const sessions::SerializedNavigationEntry>>*
  GetBlockedNavigations() const = 0;

  // Session sync related methods.
  virtual int GetSyncId() const = 0;
  virtual void SetSyncId(int sync_id) = 0;
  virtual bool ShouldSync(SyncSessionsClient* sessions_client) = 0;

  // Whether this tab is a placeholder tab. On some platforms, tabs can be
  // restored without bringing all their state into memory, and are just
  // restored as a placeholder. In that case, the previous synced data from that
  // tab should be preserved.
  virtual bool IsPlaceholderTab() const = 0;

 protected:
  SyncedTabDelegate();
};

}  // namespace sync_sessions

#endif  // COMPONENTS_SYNC_SESSIONS_SYNCED_TAB_DELEGATE_H__
