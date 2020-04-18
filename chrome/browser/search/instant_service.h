// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_INSTANT_SERVICE_H_
#define CHROME_BROWSER_SEARCH_INSTANT_SERVICE_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "build/build_config.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/ntp_tile.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#error "Instant is only used on desktop";
#endif

class InstantIOContext;
class InstantServiceObserver;
class Profile;
struct InstantMostVisitedItem;
struct ThemeBackgroundInfo;

namespace content {
class RenderProcessHost;
}

// Tracks render process host IDs that are associated with Instant, i.e.
// processes that are used to render an NTP. Also responsible for keeping
// necessary information (most visited tiles and theme info) updated in those
// renderer processes.
class InstantService : public KeyedService,
                       public content::NotificationObserver,
                       public ntp_tiles::MostVisitedSites::Observer {
 public:
  explicit InstantService(Profile* profile);
  ~InstantService() override;

  // Add, remove, and query RenderProcessHost IDs that are associated with
  // Instant processes.
  void AddInstantProcess(int process_id);
  bool IsInstantProcess(int process_id) const;

  // Adds/Removes InstantService observers.
  void AddObserver(InstantServiceObserver* observer);
  void RemoveObserver(InstantServiceObserver* observer);

  // Register prefs associated with the NTP.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

#if defined(UNIT_TEST)
  int GetInstantProcessCount() const {
    return process_ids_.size();
  }
#endif

  // Invoked whenever an NTP is opened. Causes an async refresh of Most Visited
  // items.
  void OnNewTabPageOpened();

  // Most visited item APIs.
  //
  // Invoked when the Instant page wants to delete a Most Visited item.
  void DeleteMostVisitedItem(const GURL& url);
  // Invoked when the Instant page wants to undo the deletion.
  void UndoMostVisitedDeletion(const GURL& url);
  // Invoked when the Instant page wants to undo all Most Visited deletions.
  void UndoAllMostVisitedDeletions();

  // Invoked by the InstantController to update theme information for NTP.
  //
  // TODO(kmadhusu): Invoking this from InstantController shouldn't be
  // necessary. Investigate more and remove this from here.
  void UpdateThemeInfo(bool force_update);

  // Invoked by the InstantController to update most visited items details for
  // NTP.
  void UpdateMostVisitedItemsInfo();

  // Sends the current NTP URL to a renderer process.
  void SendNewTabPageURLToRenderer(content::RenderProcessHost* rph);

  // Invoked when a custom background is selected on the NTP.
  void SetCustomBackgroundURL(const GURL& url);

 private:
  friend class InstantExtendedTest;
  friend class InstantUnitTestBase;

  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, ProcessIsolation);
  FRIEND_TEST_ALL_PREFIXES(InstantServiceTest, GetNTPTileSuggestion);

  // KeyedService:
  void Shutdown() override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Called when a renderer process is terminated.
  void OnRendererProcessTerminated(int process_id);

  // ntp_tiles::MostVisitedSites::Observer implementation.
  void OnURLsAvailable(
      const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
          sections) override;
  void OnIconMadeAvailable(const GURL& site_url) override;

  void NotifyAboutMostVisitedItems();
  void NotifyAboutThemeInfo();

  void BuildThemeInfo();

  Profile* const profile_;

  // The process ids associated with Instant processes.
  std::set<int> process_ids_;

  // InstantMostVisitedItems for NTP tiles, received from |most_visited_sites_|.
  std::vector<InstantMostVisitedItem> most_visited_items_;

  // Theme-related data for NTP overlay to adopt themes.
  std::unique_ptr<ThemeBackgroundInfo> theme_info_;

  base::ObserverList<InstantServiceObserver> observers_;

  content::NotificationRegistrar registrar_;

  scoped_refptr<InstantIOContext> instant_io_context_;

  // Data source for NTP tiles (aka Most Visited tiles). May be null.
  std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites_;

  DISALLOW_COPY_AND_ASSIGN(InstantService);
};

#endif  // CHROME_BROWSER_SEARCH_INSTANT_SERVICE_H_
