// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_CORE_IN_MEMORY_TAB_RESTORE_SERVICE_H_
#define COMPONENTS_SESSIONS_CORE_IN_MEMORY_TAB_RESTORE_SERVICE_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "components/sessions/core/sessions_export.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/sessions/core/tab_restore_service_client.h"
#include "components/sessions/core/tab_restore_service_helper.h"

namespace sessions {

class TabRestoreServiceClient;

// Tab restore service that doesn't persist tabs on disk. This is used on
// Android where tabs persistence is implemented on the application side in
// Java. Other platforms should use PersistentTabRestoreService which can be
// instantiated through the TabRestoreServiceFactory.
class SESSIONS_EXPORT InMemoryTabRestoreService : public TabRestoreService {
 public:
  // Creates a new TabRestoreService and provides an object that provides the
  // current time. The TabRestoreService does not take ownership of
  // |time_factory|.
  InMemoryTabRestoreService(std::unique_ptr<TabRestoreServiceClient> client,
                            TimeFactory* time_factory);

  ~InMemoryTabRestoreService() override;

  // TabRestoreService:
  void AddObserver(TabRestoreServiceObserver* observer) override;
  void RemoveObserver(TabRestoreServiceObserver* observer) override;
  void CreateHistoricalTab(LiveTab* live_tab, int index) override;
  void BrowserClosing(LiveTabContext* context) override;
  void BrowserClosed(LiveTabContext* context) override;
  void ClearEntries() override;
  void DeleteNavigationEntries(const DeletionPredicate& predicate) override;
  const Entries& entries() const override;
  std::vector<LiveTab*> RestoreMostRecentEntry(
      LiveTabContext* context) override;
  std::unique_ptr<Tab> RemoveTabEntryById(SessionID id) override;
  std::vector<LiveTab*> RestoreEntryById(
      LiveTabContext* context,
      SessionID id,
      WindowOpenDisposition disposition) override;
  void LoadTabsFromLastSession() override;
  bool IsLoaded() const override;
  void DeleteLastSession() override;
  bool IsRestoring() const override;
  void Shutdown() override;

 private:
  std::unique_ptr<TabRestoreServiceClient> client_;
  TabRestoreServiceHelper helper_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryTabRestoreService);
};

}  // namespace sessions

#endif  // COMPONENTS_SESSIONS_CORE_IN_MEMORY_TAB_RESTORE_SERVICE_H_
