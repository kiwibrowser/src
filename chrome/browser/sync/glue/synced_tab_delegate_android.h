// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_SYNCED_TAB_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_SYNC_GLUE_SYNCED_TAB_DELEGATE_ANDROID_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/sync_sessions/synced_tab_delegate.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class TabAndroid;
class TabContentsSyncedTabDelegate;

namespace browser_sync {
// On Android a tab can exist even without web contents.

// SyncedTabDelegateAndroid wraps TabContentsSyncedTabDelegate and provides
// a method to set web contents later when tab is brought to memory.
class SyncedTabDelegateAndroid : public sync_sessions::SyncedTabDelegate {
 public:
  explicit SyncedTabDelegateAndroid(TabAndroid* owning_tab_);
  ~SyncedTabDelegateAndroid() override;

  // SyncedTabDelegate:
  SessionID GetWindowId() const override;
  SessionID GetSessionId() const override;
  bool IsBeingDestroyed() const override;
  SessionID GetSourceTabID() const override;
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
  bool IsPlaceholderTab() const override;
  int GetSyncId() const override;
  void SetSyncId(int sync_id) override;
  bool ShouldSync(sync_sessions::SyncSessionsClient* sessions_client) override;
  bool ProfileIsSupervised() const override;
  const std::vector<std::unique_ptr<const sessions::SerializedNavigationEntry>>*
  GetBlockedNavigations() const override;

  // Set the web contents for this tab. Also creates
  // TabContentsSyncedTabDelegate for this tab and handles source tab id
  // initialization.
  virtual void SetWebContents(content::WebContents* web_contents,
                              content::WebContents* source_web_contents);
  // Set web contents to null.
  virtual void ResetWebContents();

 private:
  content::WebContents* web_contents_;
  TabAndroid* tab_android_;
  TabContentsSyncedTabDelegate* tab_contents_delegate_;

  DISALLOW_COPY_AND_ASSIGN(SyncedTabDelegateAndroid);
};
}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_SYNCED_TAB_DELEGATE_ANDROID_H_
