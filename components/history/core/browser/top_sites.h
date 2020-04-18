// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/history/core/common/thumbnail_score.h"
#include "components/keyed_service/core/refcounted_keyed_service.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image.h"

class GURL;

namespace base {
class RefCountedMemory;
}

namespace history {

// PrepopulatedPage stores information for prepopulated page for the
// initial run.
struct PrepopulatedPage {
  PrepopulatedPage();
  PrepopulatedPage(const GURL& url,
                   const base::string16& title,
                   int favicon_id,
                   int thumbnail_id,
                   SkColor color);

  MostVisitedURL most_visited;  // The prepopulated page URL and title.
  int favicon_id;               // The raw data resource for the favicon.
  int thumbnail_id;             // The raw data resource for the thumbnail.
  SkColor color;                // The best color to highlight the page, should
                                // roughly match the favicon.
};

typedef std::vector<PrepopulatedPage> PrepopulatedPageList;

// Interface for TopSites, which stores the data for the top "most visited"
// sites. This includes a cache of the most visited data from history, as well
// as the corresponding thumbnails of those sites.
//
// Some methods should only be called from the UI thread (see method
// descriptions below). All others are assumed to be threadsafe.
class TopSites : public RefcountedKeyedService {
 public:
  TopSites();

  // Sets the given thumbnail for the given URL. Returns true if the thumbnail
  // was updated. False means either the URL wasn't known to us, or we felt
  // that our current thumbnail was superior to the given one. Should be called
  // from the UI thread.
  virtual bool SetPageThumbnail(const GURL& url,
                                const gfx::Image& thumbnail,
                                const ThumbnailScore& score) = 0;

  typedef base::Callback<void(const MostVisitedURLList&)>
      GetMostVisitedURLsCallback;

  // Returns a list of most visited URLs via a callback, if
  // |include_forced_urls| is false includes only non-forced URLs. This may be
  // invoked on any thread. NOTE: the callback is called immediately if we have
  // the data cached. If data is not available yet, callback will later be
  // posted to the thread called this function.
  virtual void GetMostVisitedURLs(const GetMostVisitedURLsCallback& callback,
                                  bool include_forced_urls) = 0;

  // Gets a thumbnail for a given page. Returns true iff we have the thumbnail.
  // This may be invoked on any thread.
  // If an exact thumbnail URL match fails, |prefix_match| specifies whether or
  // not to try harder by matching the query thumbnail URL as URL prefix (as
  // defined by UrlIsPrefix()).
  // As this method may be invoked on any thread the ref count needs to be
  // incremented before this method returns, so this takes a scoped_refptr*.
  virtual bool GetPageThumbnail(
      const GURL& url,
      bool prefix_match,
      scoped_refptr<base::RefCountedMemory>* bytes) = 0;

  // Get a thumbnail score for a given page. Returns true iff we have the
  // thumbnail score.  This may be invoked on any thread. The score will
  // be copied to |score|.
  virtual bool GetPageThumbnailScore(const GURL& url,
                                     ThumbnailScore* score) = 0;

  // Get a temporary thumbnail score for a given page. Returns true iff we
  // have the thumbnail score. Useful when checking if we should update a
  // thumbnail for a given page. The score will be copied to |score|.
  virtual bool GetTemporaryPageThumbnailScore(const GURL& url,
                                              ThumbnailScore* score) = 0;

  // Asks TopSites to refresh what it thinks the top sites are. This may do
  // nothing. Should be called from the UI thread.
  virtual void SyncWithHistory() = 0;

  // Blacklisted URLs

  // Returns true if there is at least one item in the blacklist.
  virtual bool HasBlacklistedItems() const = 0;

  // Add a URL to the blacklist. Should be called from the UI thread.
  virtual void AddBlacklistedURL(const GURL& url) = 0;

  // Removes a URL from the blacklist. Should be called from the UI thread.
  virtual void RemoveBlacklistedURL(const GURL& url) = 0;

  // Returns true if the URL is blacklisted. Should be called from the UI
  // thread.
  virtual bool IsBlacklisted(const GURL& url) = 0;

  // Clear the blacklist. Should be called from the UI thread.
  virtual void ClearBlacklistedURLs() = 0;

  // Returns true if the given URL is known to the top sites service.
  // This function also returns false if TopSites isn't loaded yet.
  virtual bool IsKnownURL(const GURL& url) = 0;

  // Returns true if the top sites list of non-forced URLs is full (i.e. we
  // already have the maximum number of non-forced top sites).  This function
  // also returns false if TopSites isn't loaded yet.
  virtual bool IsNonForcedFull() = 0;

  // Returns true if the top sites list of forced URLs is full (i.e. we already
  // have the maximum number of forced top sites).  This function also returns
  // false if TopSites isn't loaded yet.
  virtual bool IsForcedFull() = 0;

  virtual bool loaded() const = 0;

  // Returns the set of prepopulate pages.
  virtual PrepopulatedPageList GetPrepopulatedPages() = 0;

  // Adds or updates a |url| for which we should force the capture of a
  // thumbnail next time it's visited. If there is already a non-forced URL
  // matching this |url| this call has no effect. Indicate this URL was last
  // forced at |time| so we can evict the older URLs when needed. Should be
  // called from the UI thread.
  virtual bool AddForcedURL(const GURL& url, const base::Time& time) = 0;

  // Called when user has navigated to |url|.
  virtual void OnNavigationCommitted(const GURL& url) = 0;

  // Add Observer to the list.
  void AddObserver(TopSitesObserver* observer);

  // Remove Observer from the list.
  void RemoveObserver(TopSitesObserver* observer);

 protected:
  void NotifyTopSitesLoaded();
  void NotifyTopSitesChanged(const TopSitesObserver::ChangeReason reason);
  ~TopSites() override;

 private:
  friend class base::RefCountedThreadSafe<TopSites>;

  base::ObserverList<TopSitesObserver, true> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(TopSites);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_H_
