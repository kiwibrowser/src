// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SYNC_TAB_CONTENTS_SYNCED_TAB_DELEGATE_H_
#define CHROME_BROWSER_UI_SYNC_TAB_CONTENTS_SYNCED_TAB_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/sessions/core/session_id.h"
#include "components/sync_sessions/synced_tab_delegate.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class TabContentsSyncedTabDelegate
    : public sync_sessions::SyncedTabDelegate,
      public content::WebContentsUserData<TabContentsSyncedTabDelegate> {
 public:
  ~TabContentsSyncedTabDelegate() override;

  // SyncedTabDelegate:
  SessionID GetWindowId() const override;
  SessionID GetSessionId() const override;
  bool IsBeingDestroyed() const override;
  std::string GetExtensionAppId() const override;
  bool IsInitialBlankNavigation() const override;
  int GetCurrentEntryIndex() const override;
  int GetEntryCount() const override;
  GURL GetVirtualURLAtIndex(int i) const override;
  GURL GetFaviconURLAtIndex(int i) const override;
  ui::PageTransition GetTransitionAtIndex(int i) const override;
  void GetSerializedNavigationAtIndex(
      int i,
      sessions::SerializedNavigationEntry* serialized_entry) const override;
  bool ProfileIsSupervised() const override;
  const std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>*
  GetBlockedNavigations() const override;
  bool IsPlaceholderTab() const override;
  int GetSyncId() const override;
  void SetSyncId(int sync_id) override;
  bool ShouldSync(sync_sessions::SyncSessionsClient* sessions_client) override;
  SessionID GetSourceTabID() const override;

 private:
  explicit TabContentsSyncedTabDelegate(content::WebContents* web_contents);
  friend class content::WebContentsUserData<TabContentsSyncedTabDelegate>;

  content::WebContents* web_contents_;
  int sync_session_id_;

  DISALLOW_COPY_AND_ASSIGN(TabContentsSyncedTabDelegate);
};

#endif  // CHROME_BROWSER_UI_SYNC_TAB_CONTENTS_SYNCED_TAB_DELEGATE_H_
