// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <map>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites_database.h"
#include "components/history/core/test/database_test_utils.h"
#include "components/history/core/test/thumbnail-inl.h"
#include "sql/connection.h"
#include "sql/recovery.h"
#include "sql/test/scoped_error_expecter.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/sqlite/sqlite3.h"
#include "url/gurl.h"

namespace {

// URL with url_rank 0 in golden files.
const GURL kUrl0 = GURL("http://www.google.com/");

// URL with url_rank 1 in golden files.
const GURL kUrl1 = GURL("http://www.google.com/chrome/intl/en/welcome.html");

// URL with url_rank 2 in golden files.
const GURL kUrl2 = GURL("https://chrome.google.com/webstore?hl=en");

// Verify that the up-to-date database has the expected tables and
// columns.  Functional tests only check whether the things which
// should be there are, but do not check if extraneous items are
// present.  Any extraneous items have the potential to interact
// negatively with future schema changes.
void VerifyTablesAndColumns(sql::Connection* db) {
  // [meta] and [thumbnails].
  EXPECT_EQ(2u, sql::test::CountSQLTables(db));

  // Implicit index on [meta], index on [thumbnails].
  EXPECT_EQ(2u, sql::test::CountSQLIndices(db));

  // [key] and [value].
  EXPECT_EQ(2u, sql::test::CountTableColumns(db, "meta"));

  // [url], [url_rank], [title], [thumbnail], [redirects],
  // [boring_score], [good_clipping], [at_top], [last_updated], and
  // [load_completed], [last_forced]
  EXPECT_EQ(11u, sql::test::CountTableColumns(db, "thumbnails"));
}

void VerifyDatabaseEmpty(sql::Connection* db) {
  size_t rows = 0;
  EXPECT_TRUE(sql::test::CountTableRows(db, "thumbnails", &rows));
  EXPECT_EQ(0u, rows);
}

}  // namespace

namespace history {

class TopSitesDatabaseTest : public testing::Test {
 protected:
  void SetUp() override {
    // Get a temporary directory for the test DB files.
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    file_name_ = temp_dir_.GetPath().AppendASCII("TestTopSites.db");
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath file_name_;
};

// Version 1 is deprecated, the resulting schema should be current,
// with no data.
TEST_F(TopSitesDatabaseTest, Version1) {
  ASSERT_TRUE(CreateDatabaseFromSQL(file_name_, "TopSites.v1.sql"));

  TopSitesDatabase db;
  ASSERT_TRUE(db.Init(file_name_));
  VerifyTablesAndColumns(db.db_.get());
  VerifyDatabaseEmpty(db.db_.get());
}

// Version 2 is deprecated, the resulting schema should be current,
// with no data.
TEST_F(TopSitesDatabaseTest, Version2) {
  ASSERT_TRUE(CreateDatabaseFromSQL(file_name_, "TopSites.v2.sql"));

  TopSitesDatabase db;
  ASSERT_TRUE(db.Init(file_name_));
  VerifyTablesAndColumns(db.db_.get());
  VerifyDatabaseEmpty(db.db_.get());
}

TEST_F(TopSitesDatabaseTest, Version3) {
  ASSERT_TRUE(CreateDatabaseFromSQL(file_name_, "TopSites.v3.sql"));

  TopSitesDatabase db;
  ASSERT_TRUE(db.Init(file_name_));

  VerifyTablesAndColumns(db.db_.get());

  // Basic operational check.
  MostVisitedURLList urls;
  std::map<GURL, Images> thumbnails;
  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(3u, urls.size());
  ASSERT_EQ(3u, thumbnails.size());
  EXPECT_EQ(kUrl0, urls[0].url);  // [0] because of url_rank.
  // kGoogleThumbnail includes nul terminator.
  ASSERT_EQ(sizeof(kGoogleThumbnail) - 1,
            thumbnails[urls[0].url].thumbnail->size());
  EXPECT_TRUE(!memcmp(thumbnails[urls[0].url].thumbnail->front(),
                      kGoogleThumbnail, sizeof(kGoogleThumbnail) - 1));

  sql::Transaction transaction(db.db_.get());
  transaction.Begin();
  ASSERT_TRUE(db.RemoveURLNoTransaction(urls[1]));
  transaction.Commit();

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(2u, urls.size());
  ASSERT_EQ(2u, thumbnails.size());
}

// Version 1 is deprecated, the resulting schema should be current,
// with no data.
TEST_F(TopSitesDatabaseTest, Recovery1) {
  // Create an example database.
  EXPECT_TRUE(CreateDatabaseFromSQL(file_name_, "TopSites.v1.sql"));

  // Corrupt the database by adjusting the header size.
  EXPECT_TRUE(sql::test::CorruptSizeInHeader(file_name_));

  // Database is unusable at the SQLite level.
  {
    sql::test::ScopedErrorExpecter expecter;
    expecter.ExpectError(SQLITE_CORRUPT);
    sql::Connection raw_db;
    EXPECT_TRUE(raw_db.Open(file_name_));
    EXPECT_FALSE(raw_db.IsSQLValid("PRAGMA integrity_check"));
    ASSERT_TRUE(expecter.SawExpectedErrors());
  }

  // Corruption should be detected and recovered during Init().
  {
    sql::test::ScopedErrorExpecter expecter;
    expecter.ExpectError(SQLITE_CORRUPT);

    TopSitesDatabase db;
    ASSERT_TRUE(db.Init(file_name_));
    VerifyTablesAndColumns(db.db_.get());
    VerifyDatabaseEmpty(db.db_.get());

    ASSERT_TRUE(expecter.SawExpectedErrors());
  }
}

TEST_F(TopSitesDatabaseTest, Recovery2) {
  // Create an example database.
  EXPECT_TRUE(CreateDatabaseFromSQL(file_name_, "TopSites.v2.sql"));

  // Corrupt the database by adjusting the header.
  EXPECT_TRUE(sql::test::CorruptSizeInHeader(file_name_));

  // Database is unusable at the SQLite level.
  {
    sql::test::ScopedErrorExpecter expecter;
    expecter.ExpectError(SQLITE_CORRUPT);
    sql::Connection raw_db;
    EXPECT_TRUE(raw_db.Open(file_name_));
    EXPECT_FALSE(raw_db.IsSQLValid("PRAGMA integrity_check"));
    ASSERT_TRUE(expecter.SawExpectedErrors());
  }

  // Corruption should be detected and recovered during Init().
  {
    sql::test::ScopedErrorExpecter expecter;
    expecter.ExpectError(SQLITE_CORRUPT);

    TopSitesDatabase db;
    ASSERT_TRUE(db.Init(file_name_));
    VerifyTablesAndColumns(db.db_.get());
    VerifyDatabaseEmpty(db.db_.get());

    ASSERT_TRUE(expecter.SawExpectedErrors());
  }
}

TEST_F(TopSitesDatabaseTest, Recovery3) {
  // Create an example database.
  EXPECT_TRUE(CreateDatabaseFromSQL(file_name_, "TopSites.v3.sql"));

  // Corrupt the database by adjusting the header.
  EXPECT_TRUE(sql::test::CorruptSizeInHeader(file_name_));

  // Database is unusable at the SQLite level.
  {
    sql::test::ScopedErrorExpecter expecter;
    expecter.ExpectError(SQLITE_CORRUPT);
    sql::Connection raw_db;
    EXPECT_TRUE(raw_db.Open(file_name_));
    EXPECT_FALSE(raw_db.IsSQLValid("PRAGMA integrity_check"));
    ASSERT_TRUE(expecter.SawExpectedErrors());
  }

  // Corruption should be detected and recovered during Init().
  {
    sql::test::ScopedErrorExpecter expecter;
    expecter.ExpectError(SQLITE_CORRUPT);

    TopSitesDatabase db;
    ASSERT_TRUE(db.Init(file_name_));

    MostVisitedURLList urls;
    std::map<GURL, Images> thumbnails;
    db.GetPageThumbnails(&urls, &thumbnails);
    ASSERT_EQ(3u, urls.size());
    ASSERT_EQ(3u, thumbnails.size());
    EXPECT_EQ(kUrl0, urls[0].url);  // [0] because of url_rank.
    // kGoogleThumbnail includes nul terminator.
    ASSERT_EQ(sizeof(kGoogleThumbnail) - 1,
              thumbnails[urls[0].url].thumbnail->size());
    EXPECT_TRUE(!memcmp(thumbnails[urls[0].url].thumbnail->front(),
                        kGoogleThumbnail, sizeof(kGoogleThumbnail) - 1));

    ASSERT_TRUE(expecter.SawExpectedErrors());
  }

  // Double-check database integrity.
  {
    sql::Connection raw_db;
    EXPECT_TRUE(raw_db.Open(file_name_));
    ASSERT_EQ("ok", sql::test::IntegrityCheck(&raw_db));
  }

  // Corrupt the thumnails.url auto-index by deleting an element from the table
  // but leaving it in the index.
  const char kIndexName[] = "sqlite_autoindex_thumbnails_1";
  // TODO(shess): Refactor CorruptTableOrIndex() to make parameterized
  // statements easy.
  const char kDeleteSql[] =
      "DELETE FROM thumbnails WHERE url = "
      "'http://www.google.com/chrome/intl/en/welcome.html'";
  EXPECT_TRUE(
      sql::test::CorruptTableOrIndex(file_name_, kIndexName, kDeleteSql));

  // SQLite can operate on the database, but notices the corruption in integrity
  // check.
  {
    sql::Connection raw_db;
    EXPECT_TRUE(raw_db.Open(file_name_));
    ASSERT_NE("ok", sql::test::IntegrityCheck(&raw_db));
  }

  // Open the database and access the corrupt index.
  {
    TopSitesDatabase db;
    ASSERT_TRUE(db.Init(file_name_));

    {
      sql::test::ScopedErrorExpecter expecter;
      expecter.ExpectError(SQLITE_CORRUPT);

      // Data for kUrl1 was deleted, but the index entry remains, this will
      // throw SQLITE_CORRUPT.  The corruption handler will recover the database
      // and poison the handle, so the outer call fails.
      EXPECT_EQ(TopSitesDatabase::kRankOfNonExistingURL,
                db.GetURLRank(MostVisitedURL(kUrl1, base::string16())));

      ASSERT_TRUE(expecter.SawExpectedErrors());
    }
  }

  // Check that the database is recovered at the SQLite level.
  {
    sql::Connection raw_db;
    EXPECT_TRUE(raw_db.Open(file_name_));
    ASSERT_EQ("ok", sql::test::IntegrityCheck(&raw_db));
  }

  // After recovery, the database accesses won't throw errors.  The top-ranked
  // item is removed, but the ranking was revised in post-processing.
  {
    TopSitesDatabase db;
    ASSERT_TRUE(db.Init(file_name_));
    VerifyTablesAndColumns(db.db_.get());

    EXPECT_EQ(TopSitesDatabase::kRankOfNonExistingURL,
              db.GetURLRank(MostVisitedURL(kUrl1, base::string16())));

    MostVisitedURLList urls;
    std::map<GURL, Images> thumbnails;
    db.GetPageThumbnails(&urls, &thumbnails);
    ASSERT_EQ(2u, urls.size());
    ASSERT_EQ(2u, thumbnails.size());
    EXPECT_EQ(kUrl0, urls[0].url);  // [0] because of url_rank.
    EXPECT_EQ(kUrl2, urls[1].url);  // [1] because of url_rank.
  }
}

TEST_F(TopSitesDatabaseTest, AddRemoveEditThumbnails) {
  ASSERT_TRUE(CreateDatabaseFromSQL(file_name_, "TopSites.v3.sql"));

  TopSitesDatabase db;
  ASSERT_TRUE(db.Init(file_name_));

  // Add a new URL, not forced, rank = 1.
  GURL mapsUrl = GURL("http://maps.google.com/");
  MostVisitedURL url1(mapsUrl, base::ASCIIToUTF16("Google Maps"));
  db.SetPageThumbnail(url1, 1, Images());

  MostVisitedURLList urls;
  std::map<GURL, Images> thumbnails;
  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(4u, urls.size());
  ASSERT_EQ(4u, thumbnails.size());
  EXPECT_EQ(kUrl0, urls[0].url);
  EXPECT_EQ(mapsUrl, urls[1].url);

  // Add a new URL, forced.
  GURL driveUrl = GURL("http://drive.google.com/");
  MostVisitedURL url2(driveUrl, base::ASCIIToUTF16("Google Drive"));
  url2.last_forced_time = base::Time::FromJsTime(789714000000);  // 10/1/1995
  db.SetPageThumbnail(url2, TopSitesDatabase::kRankOfForcedURL, Images());

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(5u, urls.size());
  ASSERT_EQ(5u, thumbnails.size());
  EXPECT_EQ(driveUrl, urls[0].url);  // Forced URLs always appear first.
  EXPECT_EQ(kUrl0, urls[1].url);
  EXPECT_EQ(mapsUrl, urls[2].url);

  // Add a new URL, forced (earlier).
  GURL plusUrl = GURL("http://plus.google.com/");
  MostVisitedURL url3(plusUrl, base::ASCIIToUTF16("Google Plus"));
  url3.last_forced_time = base::Time::FromJsTime(787035600000);  // 10/12/1994
  db.SetPageThumbnail(url3, TopSitesDatabase::kRankOfForcedURL, Images());

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(6u, urls.size());
  ASSERT_EQ(6u, thumbnails.size());
  EXPECT_EQ(plusUrl, urls[0].url);  // New forced URL should appear first.
  EXPECT_EQ(driveUrl, urls[1].url);
  EXPECT_EQ(kUrl0, urls[2].url);
  EXPECT_EQ(mapsUrl, urls[3].url);

  // Change the last_forced_time of a forced URL.
  url3.last_forced_time = base::Time::FromJsTime(792392400000);  // 10/2/1995
  db.SetPageThumbnail(url3, TopSitesDatabase::kRankOfForcedURL, Images());

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(6u, urls.size());
  ASSERT_EQ(6u, thumbnails.size());
  EXPECT_EQ(driveUrl, urls[0].url);
  EXPECT_EQ(plusUrl, urls[1].url);  // Forced URL should have moved second.
  EXPECT_EQ(kUrl0, urls[2].url);
  EXPECT_EQ(mapsUrl, urls[3].url);

  sql::Transaction transaction(db.db_.get());

  // Change a non-forced URL to forced using UpdatePageRankNoTransaction.
  url1.last_forced_time = base::Time::FromJsTime(792219600000);  // 8/2/1995
  transaction.Begin();
  db.UpdatePageRankNoTransaction(url1, TopSitesDatabase::kRankOfForcedURL);
  transaction.Commit();

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(6u, urls.size());
  ASSERT_EQ(6u, thumbnails.size());
  EXPECT_EQ(driveUrl, urls[0].url);
  EXPECT_EQ(mapsUrl, urls[1].url);  // Maps moves to second forced URL.
  EXPECT_EQ(plusUrl, urls[2].url);
  EXPECT_EQ(kUrl0, urls[3].url);

  // Change a forced URL to non-forced using SetPageThumbnail.
  url3.last_forced_time = base::Time();
  db.SetPageThumbnail(url3, 1, Images());

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(6u, urls.size());
  ASSERT_EQ(6u, thumbnails.size());
  EXPECT_EQ(driveUrl, urls[0].url);
  EXPECT_EQ(mapsUrl, urls[1].url);
  EXPECT_EQ(kUrl0, urls[2].url);
  EXPECT_EQ(plusUrl, urls[3].url);  // Plus moves to second non-forced URL.

  // Change a non-forced URL to earlier non-forced using
  // UpdatePageRankNoTransaction.
  transaction.Begin();
  db.UpdatePageRankNoTransaction(url3, 0);
  transaction.Commit();

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(6u, urls.size());
  ASSERT_EQ(6u, thumbnails.size());
  EXPECT_EQ(driveUrl, urls[0].url);
  EXPECT_EQ(mapsUrl, urls[1].url);
  EXPECT_EQ(plusUrl, urls[2].url);  // Plus moves to first non-forced URL.
  EXPECT_EQ(kUrl0, urls[3].url);

  // Change a non-forced URL to later non-forced using SetPageThumbnail.
  db.SetPageThumbnail(url3, 2, Images());

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(6u, urls.size());
  ASSERT_EQ(6u, thumbnails.size());
  EXPECT_EQ(driveUrl, urls[0].url);
  EXPECT_EQ(mapsUrl, urls[1].url);
  EXPECT_EQ(kUrl0, urls[2].url);
  EXPECT_EQ(plusUrl, urls[4].url);  // Plus moves to third non-forced URL.

  // Remove a non-forced URL.
  transaction.Begin();
  ASSERT_TRUE(db.RemoveURLNoTransaction(url3));
  transaction.Commit();

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(5u, urls.size());
  ASSERT_EQ(5u, thumbnails.size());
  EXPECT_EQ(driveUrl, urls[0].url);
  EXPECT_EQ(mapsUrl, urls[1].url);
  EXPECT_EQ(kUrl0, urls[2].url);

  // Remove a forced URL.
  transaction.Begin();
  ASSERT_TRUE(db.RemoveURLNoTransaction(url2));
  transaction.Commit();

  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(4u, urls.size());
  ASSERT_EQ(4u, thumbnails.size());
  EXPECT_EQ(mapsUrl, urls[0].url);
  EXPECT_EQ(kUrl0, urls[1].url);
}

TEST_F(TopSitesDatabaseTest, ApplyDelta) {
  ASSERT_TRUE(CreateDatabaseFromSQL(file_name_, "TopSites.v3.sql"));

  TopSitesDatabase db;
  ASSERT_TRUE(db.Init(file_name_));

  GURL mapsUrl = GURL("http://maps.google.com/");

  TopSitesDelta delta;
  // Delete kUrl0. Now db has kUrl1 and kUrl2.
  MostVisitedURL url_to_delete(kUrl0, base::ASCIIToUTF16("Google"));
  delta.deleted.push_back(url_to_delete);

  // Add a new URL, not forced, rank = 0. Now db has mapsUrl, kUrl1 and kUrl2.
  MostVisitedURLWithRank url_to_add;
  url_to_add.url = MostVisitedURL(mapsUrl, base::ASCIIToUTF16("Google Maps"));
  url_to_add.rank = 0;
  delta.added.push_back(url_to_add);

  // Move kUrl1 by updating its rank to 2. Now db has mapsUrl, kUrl2 and kUrl1.
  MostVisitedURLWithRank url_to_move;
  url_to_move.url = MostVisitedURL(kUrl1, base::ASCIIToUTF16("Google Chrome"));
  url_to_move.rank = 2;
  delta.moved.push_back(url_to_move);

  // Update db.
  db.ApplyDelta(delta);

  // Read db and verify.
  MostVisitedURLList urls;
  std::map<GURL, Images> thumbnails;
  db.GetPageThumbnails(&urls, &thumbnails);
  ASSERT_EQ(3u, urls.size());
  ASSERT_EQ(3u, thumbnails.size());
  EXPECT_EQ(mapsUrl, urls[0].url);
  EXPECT_EQ(kUrl2, urls[1].url);
  EXPECT_EQ(kUrl1, urls[2].url);
}

}  // namespace history
