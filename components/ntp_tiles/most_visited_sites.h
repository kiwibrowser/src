// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_TILES_MOST_VISITED_SITES_H_
#define COMPONENTS_NTP_TILES_MOST_VISITED_SITES_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "base/strings/string16.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/ntp_tiles/ntp_tile.h"
#include "components/ntp_tiles/popular_sites.h"
#include "components/ntp_tiles/section_type.h"
#include "components/ntp_tiles/tile_source.h"
#include "components/suggestions/proto/suggestions.pb.h"
#include "components/suggestions/suggestions_service.h"
#include "url/gurl.h"

namespace history {
class TopSites;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefService;

namespace ntp_tiles {

class IconCacher;

// Shim interface for SupervisedUserService.
class MostVisitedSitesSupervisor {
 public:
  struct Whitelist {
    base::string16 title;
    GURL entry_point;
    base::FilePath large_icon_path;
  };

  class Observer {
   public:
    virtual void OnBlockedSitesChanged() = 0;

   protected:
    ~Observer() {}
  };

  virtual ~MostVisitedSitesSupervisor() {}

  // Pass non-null to set observer, or null to remove observer.
  // If setting observer, there must not yet be an observer set.
  // If removing observer, there must already be one to remove.
  // Does not take ownership. Observer must outlive this object.
  virtual void SetObserver(Observer* new_observer) = 0;

  // If true, |url| should not be shown on the NTP.
  virtual bool IsBlocked(const GURL& url) = 0;

  // Explicitly-specified sites to show on NTP.
  virtual std::vector<Whitelist> GetWhitelists() = 0;

  // If true, be conservative about suggesting sites from outside sources.
  virtual bool IsChildProfile() = 0;
};

// Tracks the list of most visited sites and their thumbnails.
class MostVisitedSites : public history::TopSitesObserver,
                         public MostVisitedSitesSupervisor::Observer {
 public:
  // The observer to be notified when the list of most visited sites changes.
  class Observer {
   public:
    // |sections| must at least contain the PERSONALIZED section.
    virtual void OnURLsAvailable(
        const std::map<SectionType, NTPTilesVector>& sections) = 0;
    virtual void OnIconMadeAvailable(const GURL& site_url) = 0;

   protected:
    virtual ~Observer() {}
  };

  // This interface delegates the retrieval of the home page to the
  // platform-specific implementation.
  class HomePageClient {
   public:
    using TitleCallback =
        base::OnceCallback<void(const base::Optional<base::string16>& title)>;

    virtual ~HomePageClient() = default;
    virtual bool IsHomePageEnabled() const = 0;
    virtual bool IsNewTabPageUsedAsHomePage() const = 0;
    virtual GURL GetHomePageUrl() const = 0;
    virtual void QueryHomePageTitle(TitleCallback title_callback) = 0;
  };

  // Construct a MostVisitedSites instance.
  //
  // |prefs| and |suggestions| are required and may not be null. |top_sites|,
  // |popular_sites|, |supervisor| and |home_page_client| are optional and if
  // null, the associated features will be disabled.
  MostVisitedSites(PrefService* prefs,
                   scoped_refptr<history::TopSites> top_sites,
                   suggestions::SuggestionsService* suggestions,
                   std::unique_ptr<PopularSites> popular_sites,
                   std::unique_ptr<IconCacher> icon_cacher,
                   std::unique_ptr<MostVisitedSitesSupervisor> supervisor);

  ~MostVisitedSites() override;

  // Returns true if this object was created with a non-null provider for the
  // given NTP tile source. That source may or may not actually provide tiles,
  // depending on its configuration and the priority of different sources.
  bool DoesSourceExist(TileSource source) const;

  // Returns the corresponding object passed at construction.
  history::TopSites* top_sites() { return top_sites_.get(); }
  suggestions::SuggestionsService* suggestions() {
    return suggestions_service_;
  }
  PopularSites* popular_sites() { return popular_sites_.get(); }
  MostVisitedSitesSupervisor* supervisor() { return supervisor_.get(); }

  // Sets the observer, and immediately fetches the current suggestions.
  // Does not take ownership of |observer|, which must outlive this object and
  // must not be null.
  void SetMostVisitedURLsObserver(Observer* observer, size_t num_sites);

  // Sets the client that provides platform-specific home page preferences.
  // When used to replace an existing client, the new client will first be used
  // during the construction of a new tile set.
  void SetHomePageClient(std::unique_ptr<HomePageClient> client);

  // Requests an asynchronous refresh of the suggestions. Notifies the observer
  // if the request resulted in the set of tiles changing.
  void Refresh();

  // Forces a rebuild of the current tiles to update the pinned home page.
  void OnHomePageStateChanged();

  void AddOrRemoveBlacklistedUrl(const GURL& url, bool add_url);
  void ClearBlacklistedUrls();

  // MostVisitedSitesSupervisor::Observer implementation.
  void OnBlockedSitesChanged() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Workhorse for SaveNewTilesAndNotify. Implemented as a separate static and
  // public method for ease of testing.
  static NTPTilesVector MergeTiles(NTPTilesVector personal_tiles,
                                   NTPTilesVector whitelist_tiles,
                                   NTPTilesVector popular_tiles);

 private:
  FRIEND_TEST_ALL_PREFIXES(MostVisitedSitesTest,
                           ShouldDeduplicateDomainWithNoWwwDomain);
  FRIEND_TEST_ALL_PREFIXES(MostVisitedSitesTest,
                           ShouldDeduplicateDomainByRemovingMobilePrefixes);
  FRIEND_TEST_ALL_PREFIXES(MostVisitedSitesTest,
                           ShouldDeduplicateDomainByReplacingMobilePrefixes);

  // This function tries to match the given |host| to a close fit in
  // |hosts_to_skip| by removing a prefix that is commonly used to redirect from
  // or to mobile pages (m.xyz.com --> xyz.com).
  // If this approach fails, the prefix is replaced by another prefix.
  // That way, true is returned for m.x.com if www.x.com is in |hosts_to_skip|.
  static bool IsHostOrMobilePageKnown(
      const std::set<std::string>& hosts_to_skip,
      const std::string& host);

  // Initialize the query to Top Sites. Called if the SuggestionsService
  // returned no data.
  void InitiateTopSitesQuery();

  // If there's a whitelist entry point for the URL, return the large icon path.
  base::FilePath GetWhitelistLargeIconPath(const GURL& url);

  // Callback for when data is available from TopSites.
  void OnMostVisitedURLsAvailable(
      const history::MostVisitedURLList& visited_list);

  // Callback for when an update is reported by the SuggestionsService.
  void OnSuggestionsProfileChanged(
      const suggestions::SuggestionsProfile& suggestions_profile);

  // Builds the current tileset based on available caches and notifies the
  // observer.
  void BuildCurrentTiles();

  // Same as above the SuggestionsProfile is provided, no need to read cache.
  void BuildCurrentTilesGivenSuggestionsProfile(
      const suggestions::SuggestionsProfile& suggestions_profile);

  // Creates whitelist entry point suggestions whose hosts weren't used yet.
  NTPTilesVector CreateWhitelistEntryPointTiles(
      const std::set<std::string>& used_hosts,
      size_t num_actual_tiles);

  // Creates tiles for all popular site sections. Uses |num_actual_tiles| and
  // |used_hosts| to restrict results for the PERSONALIZED section.
  std::map<SectionType, NTPTilesVector> CreatePopularSitesSections(
      const std::set<std::string>& used_hosts,
      size_t num_actual_tiles);

  // Creates tiles for |sites_vector|. The returned vector will neither contain
  // more than |num_max_tiles| nor include sites in |hosts_to_skip|.
  NTPTilesVector CreatePopularSitesTiles(
      const PopularSites::SitesVector& sites_vector,
      const std::set<std::string>& hosts_to_skip,
      size_t num_max_tiles);

  // Initiates a query for the home page tile if needed and calls
  // |SaveTilesAndNotify| in the end.
  void InitiateNotificationForNewTiles(NTPTilesVector new_tiles);

  // Takes the personal tiles, creates and merges in whitelist and popular tiles
  // if appropriate, and saves the new tiles. Notifies the observer if the tiles
  // were actually changed.
  void SaveTilesAndNotify(NTPTilesVector personal_tiles);

  void OnPopularSitesDownloaded(bool success);

  void OnIconMadeAvailable(const GURL& site_url);

  // Updates the already used hosts and the total tile count based on given new
  // tiles. Enforces that the required amount of tiles is not exceeded.
  void AddToHostsAndTotalCount(const NTPTilesVector& new_tiles,
                               std::set<std::string>* hosts,
                               size_t* total_tile_count) const;

  // Adds the home page as first tile to |tiles| and returns them as new vector.
  // Drops existing tiles with the same host as the home page and tiles that
  // would exceed the maximum.
  NTPTilesVector InsertHomeTile(NTPTilesVector tiles,
                                const base::string16& title) const;

  void OnHomePageTitleDetermined(NTPTilesVector tiles,
                                 const base::Optional<base::string16>& title);

  // Returns true if there is a valid home page that can be pinned as tile.
  bool ShouldAddHomeTile() const;

  // history::TopSitesObserver implementation.
  void TopSitesLoaded(history::TopSites* top_sites) override;
  void TopSitesChanged(history::TopSites* top_sites,
                       ChangeReason change_reason) override;

  PrefService* prefs_;
  scoped_refptr<history::TopSites> top_sites_;
  suggestions::SuggestionsService* suggestions_service_;
  std::unique_ptr<PopularSites> const popular_sites_;
  std::unique_ptr<IconCacher> const icon_cacher_;
  std::unique_ptr<MostVisitedSitesSupervisor> supervisor_;
  std::unique_ptr<HomePageClient> home_page_client_;

  Observer* observer_;

  // The maximum number of most visited sites to return.
  size_t num_sites_;

  std::unique_ptr<
      suggestions::SuggestionsService::ResponseCallbackList::Subscription>
      suggestions_subscription_;

  ScopedObserver<history::TopSites, history::TopSitesObserver>
      top_sites_observer_;

  // The main source of personal tiles - either TOP_SITES or SUGGESTIONS_SEVICE.
  TileSource mv_source_;

  // Current set of tiles. Optional so that the observer can be notified
  // whenever it changes, including possibily an initial change from
  // !current_tiles_.has_value() to current_tiles_->empty().
  base::Optional<NTPTilesVector> current_tiles_;

  // For callbacks may be run after destruction, used exclusively for TopSites
  // (since it's used to detect whether there's a query in flight).
  base::WeakPtrFactory<MostVisitedSites> top_sites_weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MostVisitedSites);
};

}  // namespace ntp_tiles

#endif  // COMPONENTS_NTP_TILES_MOST_VISITED_SITES_H_
