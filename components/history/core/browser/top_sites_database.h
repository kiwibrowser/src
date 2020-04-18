// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_DATABASE_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_DATABASE_H_

#include <map>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "components/history/core/browser/history_types.h"
#include "sql/meta_table.h"
#include "sql/transaction.h"

namespace base {
class FilePath;
}

namespace sql {
class Connection;
}

namespace history {

class TopSitesDatabase {
 public:
  TopSitesDatabase();
  ~TopSitesDatabase();

  // Must be called after creation but before any other methods are called.
  // Returns true on success. If false, no other functions should be called.
  bool Init(const base::FilePath& db_name);

  // Updates the database according to the changes recorded in |delta|.
  void ApplyDelta(const TopSitesDelta& delta);

  // Returns a list of all URLs currently in the table.
  // WARNING: clears both input arguments.
  void GetPageThumbnails(MostVisitedURLList* urls,
                         std::map<GURL, Images>* thumbnails);

  // Sets a thumbnail for a URL. |new_rank| is the position of the URL
  // in the list of TopURLs, zero-based.
  // If the URL is not in the table, add it. If it is, replaces its
  // thumbnail and rank. Shifts the ranks of other URLs if necessary.
  void SetPageThumbnail(const MostVisitedURL& url,
                        int new_rank,
                        const Images& thumbnail);

 private:
  FRIEND_TEST_ALL_PREFIXES(TopSitesDatabaseTest, Version1);
  FRIEND_TEST_ALL_PREFIXES(TopSitesDatabaseTest, Version2);
  FRIEND_TEST_ALL_PREFIXES(TopSitesDatabaseTest, Version3);
  FRIEND_TEST_ALL_PREFIXES(TopSitesDatabaseTest, Recovery1);
  FRIEND_TEST_ALL_PREFIXES(TopSitesDatabaseTest, Recovery2);
  FRIEND_TEST_ALL_PREFIXES(TopSitesDatabaseTest, Recovery3);
  FRIEND_TEST_ALL_PREFIXES(TopSitesDatabaseTest, AddRemoveEditThumbnails);

  // Rank of all URLs that are forced and therefore cannot be automatically
  // evicted.
  static const int kRankOfForcedURL;

  // Rank used to indicate that a URL is not stored in the database.
  static const int kRankOfNonExistingURL;

  // Upgrades the thumbnail table to version 3, returning true if the
  // upgrade was successful.
  bool UpgradeToVersion3();

  // Sets a thumbnail for a URL. |new_rank| is the position of the URL
  // in the list of TopURLs, zero-based.
  // If the URL is not in the table, add it. If it is, replaces its
  // thumbnail and rank. Shifts the ranks of other URLs if necessary.
  // Should be called within an open transaction.
  void SetPageThumbnailNoTransaction(const MostVisitedURL& url,
                                     int new_rank,
                                     const Images& thumbnail);

  // Adds a new URL to the database.
  void AddPageThumbnail(const MostVisitedURL& url,
                        int new_rank,
                        const Images& thumbnail);

  // Gets a thumbnail for a given page. Returns true iff we have the thumbnail.
  bool GetPageThumbnail(const GURL& url, Images* thumbnail);

  // Updates thumbnail of a URL that's already in the database.
  // Returns true if the database query succeeds.
  bool UpdatePageThumbnail(const MostVisitedURL& url, const Images& thumbnail);

  // Returns |url|'s current rank or kRankOfNonExistingURL if not present.
  int GetURLRank(const MostVisitedURL& url);

  // Sets the rank for a given URL. The URL must be in the database.
  // Uses SetPageThumbnail if it's not. Should be called within an open
  // transaction.
  void UpdatePageRankNoTransaction(const MostVisitedURL& url, int new_rank);

  // Removes the record for this URL. Returns false iff there is a failure in
  // running the statement. Should be called within an open transaction.
  bool RemoveURLNoTransaction(const MostVisitedURL& url);

  // Helper function to implement internals of Init().  This allows
  // Init() to retry in case of failure, since some failures will
  // invoke recovery code.
  bool InitImpl(const base::FilePath& db_name);

  sql::Connection* CreateDB(const base::FilePath& db_name);

  std::unique_ptr<sql::Connection> db_;
  sql::MetaTable meta_table_;

  DISALLOW_COPY_AND_ASSIGN(TopSitesDatabase);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_DATABASE_H_
