// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/top_sites_database.h"

#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites.h"
#include "components/history/core/common/thumbnail_score.h"
#include "sql/connection.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "third_party/sqlite/sqlite3.h"

namespace history {

// Description of database table:
//
// thumbnails
//   url              URL of the sites for which we have a thumbnail.
//   url_rank         Index of the URL in that thumbnail, 0-based. The thumbnail
//                    with the highest rank will be the next one evicted. Forced
//                    thumbnails have a rank of -1.
//   title            The title to display under that thumbnail.
//   redirects        A space separated list of URLs that are known to redirect
//                    to this url.
//   boring_score     How "boring" that thumbnail is. See ThumbnailScore.
//   good_clipping    True if the thumbnail was clipped from the bottom, keeping
//                    the entire width of the window. See ThumbnailScore.
//   at_top           True if the thumbnail was captured at the top of the
//                    website.
//   last_updated     The time at which this thumbnail was last updated.
//   load_completed   True if the thumbnail was captured after the page load was
//                    completed.
//   last_forced      If this is a forced thumbnail, records the last time it
//                    was forced. If it's not a forced thumbnail, 0.

namespace {

// For this database, schema migrations are deprecated after two
// years.  This means that the oldest non-deprecated version should be
// two years old or greater (thus the migrations to get there are
// older).  Databases containing deprecated versions will be cleared
// at startup.  Since this database is a cache, losing old data is not
// fatal (in fact, very old data may be expired immediately at startup
// anyhow).

// Version 3: b6d6a783/r231648 by beaudoin@chromium.org on 2013-10-29
// Version 2: eb0b24e6/r87284 by satorux@chromium.org on 2011-05-31 (deprecated)
// Version 1: 809cc4d8/r64072 by sky@chromium.org on 2010-10-27 (deprecated)

// NOTE(shess): When changing the version, add a new golden file for
// the new version and a test to verify that Init() works with it.
static const int kVersionNumber = 3;
static const int kDeprecatedVersionNumber = 2;  // and earlier.

bool InitTables(sql::Connection* db) {
  const char kThumbnailsSql[] =
      "CREATE TABLE IF NOT EXISTS thumbnails ("
      "url LONGVARCHAR PRIMARY KEY,"
      "url_rank INTEGER,"
      "title LONGVARCHAR,"
      "thumbnail BLOB,"
      "redirects LONGVARCHAR,"
      "boring_score DOUBLE DEFAULT 1.0,"
      "good_clipping INTEGER DEFAULT 0,"
      "at_top INTEGER DEFAULT 0,"
      "last_updated INTEGER DEFAULT 0,"
      "load_completed INTEGER DEFAULT 0,"
      "last_forced INTEGER DEFAULT 0)";
  return db->Execute(kThumbnailsSql);
}

// Encodes redirects into a string.
std::string GetRedirects(const MostVisitedURL& url) {
  std::vector<base::StringPiece> redirects;
  for (const auto& redirect : url.redirects)
    redirects.push_back(redirect.spec());
  return base::JoinString(redirects, " ");
}

// Decodes redirects from a string and sets them for the url.
void SetRedirects(const std::string& redirects, MostVisitedURL* url) {
  for (const std::string& redirect : base::SplitString(
           redirects, base::kWhitespaceASCII,
           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    GURL redirect_url(redirect);
    if (redirect_url.is_valid())
      url->redirects.push_back(redirect_url);
  }
}

// Track various failure (and success) cases in recovery code.
//
// TODO(shess): The recovery code is complete, but by nature runs in challenging
// circumstances, so errors will happen.  This histogram is intended to expose
// the failures seen in the fleet.  Frequent failure cases can be explored more
// deeply to see if the complexity to fix them is warranted.  Infrequent failure
// cases can be resolved by marking the database unrecoverable (which will
// delete the data).
//
// Based on the thumbnail_database.cc recovery code, FAILED_SCOPER should
// dominate, followed distantly by FAILED_META, with few or no other failures.
enum RecoveryEventType {
  // Database successfully recovered.
  RECOVERY_EVENT_RECOVERED = 0,

  // Database successfully deprecated.
  RECOVERY_EVENT_DEPRECATED,

  // Sqlite.RecoveryEvent can usually be used to get more detail about the
  // specific failure (see sql/recovery.cc).
  OBSOLETE_RECOVERY_EVENT_FAILED_SCOPER,
  RECOVERY_EVENT_FAILED_META_VERSION,
  RECOVERY_EVENT_FAILED_META_WRONG_VERSION,
  OBSOLETE_RECOVERY_EVENT_FAILED_META_INIT,
  OBSOLETE_RECOVERY_EVENT_FAILED_SCHEMA_INIT,
  OBSOLETE_RECOVERY_EVENT_FAILED_AUTORECOVER_THUMBNAILS,
  RECOVERY_EVENT_FAILED_COMMIT,

  // Track invariants resolved by FixThumbnailsTable().
  RECOVERY_EVENT_INVARIANT_RANK,
  RECOVERY_EVENT_INVARIANT_REDIRECT,
  RECOVERY_EVENT_INVARIANT_CONTIGUOUS,

  // Track automated full-database recovery.
  RECOVERY_EVENT_FAILED_AUTORECOVER,

  // Always keep this at the end.
  RECOVERY_EVENT_MAX,
};

void RecordRecoveryEvent(RecoveryEventType recovery_event) {
  UMA_HISTOGRAM_ENUMERATION("History.TopSitesRecovery", recovery_event,
                            RECOVERY_EVENT_MAX);
}

// Most corruption comes down to atomic updates between pages being broken
// somehow.  This can result in either missing data, or overlapping data,
// depending on the operation broken.  This table has large rows, which will use
// overflow pages, so it is possible (though unlikely) that a chain could fit
// together and yield a row with errors.
void FixThumbnailsTable(sql::Connection* db) {
  // Enforce invariant separating forced and non-forced thumbnails.
  const char kFixRankSql[] =
      "DELETE FROM thumbnails "
      "WHERE (url_rank = -1 AND last_forced = 0) "
      "OR (url_rank <> -1 AND last_forced <> 0)";
  ignore_result(db->Execute(kFixRankSql));
  if (db->GetLastChangeCount() > 0)
    RecordRecoveryEvent(RECOVERY_EVENT_INVARIANT_RANK);

  // Enforce invariant that url is in its own redirects.
  const char kFixRedirectsSql[] =
      "DELETE FROM thumbnails "
      "WHERE url <> substr(redirects, -length(url), length(url))";
  ignore_result(db->Execute(kFixRedirectsSql));
  if (db->GetLastChangeCount() > 0)
    RecordRecoveryEvent(RECOVERY_EVENT_INVARIANT_REDIRECT);

  // Enforce invariant that url_rank>=0 forms a contiguous series.
  // TODO(shess): I have not found an UPDATE+SUBSELECT method of managing this.
  // It can be done with a temporary table and a subselect, but doing it
  // manually is easier to follow.  Another option would be to somehow integrate
  // the renumbering into the table recovery code.
  const char kByRankSql[] =
      "SELECT url_rank, rowid FROM thumbnails WHERE url_rank <> -1 "
      "ORDER BY url_rank";
  sql::Statement select_statement(db->GetUniqueStatement(kByRankSql));

  const char kAdjustRankSql[] =
      "UPDATE thumbnails SET url_rank = ? WHERE rowid = ?";
  sql::Statement update_statement(db->GetUniqueStatement(kAdjustRankSql));

  // Update any rows where |next_rank| doesn't match |url_rank|.
  int next_rank = 0;
  bool adjusted = false;
  while (select_statement.Step()) {
    const int url_rank = select_statement.ColumnInt(0);
    if (url_rank != next_rank) {
      adjusted = true;
      update_statement.Reset(true);
      update_statement.BindInt(0, next_rank);
      update_statement.BindInt64(1, select_statement.ColumnInt64(1));
      update_statement.Run();
    }
    ++next_rank;
  }
  if (adjusted)
    RecordRecoveryEvent(RECOVERY_EVENT_INVARIANT_CONTIGUOUS);
}

// Recover the database to the extent possible, then fixup any broken
// constraints.
void RecoverAndFixup(sql::Connection* db, const base::FilePath& db_path) {
  // NOTE(shess): If the version changes, review this code.
  DCHECK_EQ(3, kVersionNumber);

  std::unique_ptr<sql::Recovery> recovery =
      sql::Recovery::BeginRecoverDatabase(db, db_path);
  if (!recovery) {
    RecordRecoveryEvent(RECOVERY_EVENT_FAILED_AUTORECOVER);
    return;
  }

  // If the [meta] table does not exist, or the [version] key cannot be found,
  // then the schema is indeterminate.  The only plausible approach would be to
  // validate that the schema contains all of the tables and indices and columns
  // expected, but that complexity may not be warranted, this case has only been
  // seen for a few thousand database files.
  int version = 0;
  if (!recovery->SetupMeta() || !recovery->GetMetaVersionNumber(&version)) {
    sql::Recovery::Unrecoverable(std::move(recovery));
    RecordRecoveryEvent(RECOVERY_EVENT_FAILED_META_VERSION);
    return;
  }

  // In this case the next open will clear the database anyhow.
  if (version <= kDeprecatedVersionNumber) {
    sql::Recovery::Unrecoverable(std::move(recovery));
    RecordRecoveryEvent(RECOVERY_EVENT_DEPRECATED);
    return;
  }

  // TODO(shess): Consider marking corrupt databases from the future
  // Unrecoverable(), since this histogram value has never been seen.  OTOH,
  // this may be too risky, because if future code was correlated with
  // corruption then rollback would be a sensible response.
  if (version > kVersionNumber) {
    RecordRecoveryEvent(RECOVERY_EVENT_FAILED_META_WRONG_VERSION);
    sql::Recovery::Rollback(std::move(recovery));
    return;
  }

  // TODO(shess): Inline this?
  FixThumbnailsTable(recovery->db());

  if (!sql::Recovery::Recovered(std::move(recovery))) {
    // TODO(shess): Very unclear what this failure would actually mean, and what
    // should be done.  Add histograms to Recovered() implementation to get some
    // insight.
    RecordRecoveryEvent(RECOVERY_EVENT_FAILED_COMMIT);
    return;
  }

  RecordRecoveryEvent(RECOVERY_EVENT_RECOVERED);
}

void DatabaseErrorCallback(sql::Connection* db,
                           const base::FilePath& db_path,
                           int extended_error,
                           sql::Statement* stmt) {
  // TODO(shess): Assert that this is running on a safe thread.  AFAICT, should
  // be the history thread, but at this level I can't see how to reach that.

  // Attempt to recover corrupt databases.
  if (sql::Recovery::ShouldRecover(extended_error)) {
    // Prevent reentrant calls.
    db->reset_error_callback();

    // After this call, the |db| handle is poisoned so that future calls will
    // return errors until the handle is re-opened.
    RecoverAndFixup(db, db_path);

    // The DLOG(FATAL) below is intended to draw immediate attention to errors
    // in newly-written code.  Database corruption is generally a result of OS
    // or hardware issues, not coding errors at the client level, so displaying
    // the error would probably lead to confusion.  The ignored call signals the
    // test-expectation framework that the error was handled.
    ignore_result(sql::Connection::IsExpectedSqliteError(extended_error));
    return;
  }

  // TODO(shess): This database's error histograms look like:
  // 84% SQLITE_CORRUPT, SQLITE_CANTOPEN, SQLITE_NOTADB
  //  7% SQLITE_ERROR
  //  6% SQLITE_IOERR variants
  //  2% SQLITE_READONLY
  // .4% SQLITE_FULL
  // nominal SQLITE_TOBIG, SQLITE_AUTH, and SQLITE_BUSY.  In the case of
  // thumbnail_database.cc, as soon as the recovery code landed, SQLITE_IOERR
  // shot to leadership.  If the I/O error is system-level, there is probably no
  // hope, but if it is restricted to something about the database file, it is
  // possible that the recovery code could be brought to bear.  In fact, it is
  // possible that running recovery would be a reasonable default when errors
  // are seen.

  // The default handling is to assert on debug and to ignore on release.
  if (!sql::Connection::IsExpectedSqliteError(extended_error))
    DLOG(FATAL) << db->GetErrorMessage();
}

}  // namespace

// static
const int TopSitesDatabase::kRankOfForcedURL = -1;

// static
const int TopSitesDatabase::kRankOfNonExistingURL = -2;

TopSitesDatabase::TopSitesDatabase() {
}

TopSitesDatabase::~TopSitesDatabase() {
}

bool TopSitesDatabase::Init(const base::FilePath& db_name) {
  // Retry failed InitImpl() in case the recovery system fixed things.
  // TODO(shess): Instrument to figure out if there are any persistent failure
  // cases which do not resolve themselves.
  const size_t kAttempts = 2;

  for (size_t i = 0; i < kAttempts; ++i) {
    if (InitImpl(db_name))
      return true;

    meta_table_.Reset();
    db_.reset();
  }
  return false;
}

bool TopSitesDatabase::InitImpl(const base::FilePath& db_name) {
  const bool file_existed = base::PathExists(db_name);

  db_.reset(CreateDB(db_name));
  if (!db_)
    return false;

  // An older version had data with no meta table.  Deprecate by razing.
  // TODO(shess): Just have RazeIfDeprecated() handle this case.
  const bool does_meta_exist = sql::MetaTable::DoesTableExist(db_.get());
  if (!does_meta_exist && file_existed) {
    if (!db_->Raze())
      return false;
  }

  // Clear databases which are too old to process.
  DCHECK_LT(kDeprecatedVersionNumber, kVersionNumber);
  sql::MetaTable::RazeIfDeprecated(db_.get(), kDeprecatedVersionNumber);

  // Scope initialization in a transaction so we can't be partially
  // initialized.
  sql::Transaction transaction(db_.get());
  // TODO(shess): Failure to open transaction is bad, address it.
  if (!transaction.Begin())
    return false;

  if (!meta_table_.Init(db_.get(), kVersionNumber, kVersionNumber))
    return false;

  if (!InitTables(db_.get()))
    return false;

  if (meta_table_.GetVersionNumber() == 2) {
    if (!UpgradeToVersion3()) {
      LOG(WARNING) << "Unable to upgrade top sites database to version 3.";
      return false;
    }
  }

  // Version check.
  if (meta_table_.GetVersionNumber() != kVersionNumber)
    return false;

  // Initialization is complete.
  if (!transaction.Commit())
    return false;

  return true;
}

void TopSitesDatabase::ApplyDelta(const TopSitesDelta& delta) {
  sql::Transaction transaction(db_.get());
  transaction.Begin();

  for (size_t i = 0; i < delta.deleted.size(); ++i) {
    if (!RemoveURLNoTransaction(delta.deleted[i]))
      return;
  }

  for (size_t i = 0; i < delta.added.size(); ++i) {
    SetPageThumbnailNoTransaction(delta.added[i].url, delta.added[i].rank,
                                  Images());
  }

  for (size_t i = 0; i < delta.moved.size(); ++i)
    UpdatePageRankNoTransaction(delta.moved[i].url, delta.moved[i].rank);

  transaction.Commit();
}

bool TopSitesDatabase::UpgradeToVersion3() {
  // Add 'last_forced' column.
  if (!db_->Execute(
          "ALTER TABLE thumbnails ADD last_forced INTEGER DEFAULT 0")) {
    NOTREACHED();
    return false;
  }
  meta_table_.SetVersionNumber(3);
  return true;
}

void TopSitesDatabase::GetPageThumbnails(MostVisitedURLList* urls,
                                         URLToImagesMap* thumbnails) {
  sql::Statement statement(db_->GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT url, url_rank, title, thumbnail, redirects, "
      "boring_score, good_clipping, at_top, last_updated, load_completed, "
      "last_forced FROM thumbnails ORDER BY url_rank, last_forced"));

  if (!statement.is_valid()) {
    LOG(WARNING) << db_->GetErrorMessage();
    return;
  }

  urls->clear();
  thumbnails->clear();

  while (statement.Step()) {
    // Results are sorted by url_rank. For forced thumbnails with url_rank = -1,
    // thumbnails are sorted by last_forced.
    MostVisitedURL url;
    GURL gurl(statement.ColumnString(0));
    url.url = gurl;
    url.title = statement.ColumnString16(2);
    url.last_forced_time = base::Time() + base::TimeDelta::FromMicroseconds(
                                              statement.ColumnInt64(10));
    std::string redirects = statement.ColumnString(4);
    SetRedirects(redirects, &url);
    urls->push_back(url);

    std::vector<unsigned char> data;
    statement.ColumnBlobAsVector(3, &data);
    Images thumbnail;
    if (!data.empty())
      thumbnail.thumbnail = base::RefCountedBytes::TakeVector(&data);
    thumbnail.thumbnail_score.boring_score = statement.ColumnDouble(5);
    thumbnail.thumbnail_score.good_clipping = statement.ColumnBool(6);
    thumbnail.thumbnail_score.at_top = statement.ColumnBool(7);
    thumbnail.thumbnail_score.time_at_snapshot =
        base::Time() +
        base::TimeDelta::FromMicroseconds(statement.ColumnInt64(8));
    thumbnail.thumbnail_score.load_completed = statement.ColumnBool(9);
    (*thumbnails)[gurl] = thumbnail;
  }
}

bool TopSitesDatabase::GetPageThumbnail(const GURL& url, Images* thumbnail) {
  sql::Statement statement(db_->GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT thumbnail, boring_score, good_clipping, at_top, last_updated "
      "FROM thumbnails WHERE url=?"));
  statement.BindString(0, url.spec());
  if (!statement.Step())
    return false;

  std::vector<unsigned char> data;
  statement.ColumnBlobAsVector(0, &data);
  thumbnail->thumbnail = base::RefCountedBytes::TakeVector(&data);
  thumbnail->thumbnail_score.boring_score = statement.ColumnDouble(1);
  thumbnail->thumbnail_score.good_clipping = statement.ColumnBool(2);
  thumbnail->thumbnail_score.at_top = statement.ColumnBool(3);
  thumbnail->thumbnail_score.time_at_snapshot =
      base::Time() +
      base::TimeDelta::FromMicroseconds(statement.ColumnInt64(4));
  return true;
}

void TopSitesDatabase::SetPageThumbnail(const MostVisitedURL& url,
                                        int new_rank,
                                        const Images& thumbnail) {
  sql::Transaction transaction(db_.get());
  transaction.Begin();
  SetPageThumbnailNoTransaction(url, new_rank, thumbnail);
  transaction.Commit();
}

void TopSitesDatabase::SetPageThumbnailNoTransaction(const MostVisitedURL& url,
                                                     int new_rank,
                                                     const Images& thumbnail) {
  int rank = GetURLRank(url);
  if (rank == kRankOfNonExistingURL) {
    AddPageThumbnail(url, new_rank, thumbnail);
  } else {
    UpdatePageRankNoTransaction(url, new_rank);
    UpdatePageThumbnail(url, thumbnail);
  }
}

void TopSitesDatabase::AddPageThumbnail(const MostVisitedURL& url,
                                        int new_rank,
                                        const Images& thumbnail) {
  sql::Statement statement(db_->GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT OR REPLACE INTO thumbnails "
      "(url, url_rank, title, thumbnail, redirects, "
      "boring_score, good_clipping, at_top, last_updated, load_completed, "
      "last_forced) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
  statement.BindString(0, url.url.spec());
  statement.BindInt(1, kRankOfForcedURL);  // Fist make it a forced thumbnail.
  statement.BindString16(2, url.title);
  if (thumbnail.thumbnail.get() && thumbnail.thumbnail->front()) {
    statement.BindBlob(3, thumbnail.thumbnail->front(),
                       static_cast<int>(thumbnail.thumbnail->size()));
  }
  statement.BindString(4, GetRedirects(url));
  const ThumbnailScore& score = thumbnail.thumbnail_score;
  statement.BindDouble(5, score.boring_score);
  statement.BindBool(6, score.good_clipping);
  statement.BindBool(7, score.at_top);
  statement.BindInt64(8,
                      score.time_at_snapshot.since_origin().InMicroseconds());
  statement.BindBool(9, score.load_completed);
  int64_t last_forced = url.last_forced_time.since_origin().InMicroseconds();
  DCHECK((last_forced == 0) == (new_rank != kRankOfForcedURL))
      << "Thumbnail without a forced time stamp has a forced rank, or the "
      << "opposite.";
  statement.BindInt64(10, last_forced);
  if (!statement.Run())
    return;

  // Update rank if this is not a forced thumbnail.
  if (new_rank != kRankOfForcedURL)
    UpdatePageRankNoTransaction(url, new_rank);
}

bool TopSitesDatabase::UpdatePageThumbnail(const MostVisitedURL& url,
                                           const Images& thumbnail) {
  sql::Statement statement(db_->GetCachedStatement(
      SQL_FROM_HERE,
      "UPDATE thumbnails SET "
      "title = ?, thumbnail = ?, redirects = ?, "
      "boring_score = ?, good_clipping = ?, at_top = ?, last_updated = ?, "
      "load_completed = ?, last_forced = ?"
      "WHERE url = ? "));
  statement.BindString16(0, url.title);
  if (thumbnail.thumbnail.get() && thumbnail.thumbnail->front()) {
    statement.BindBlob(1, thumbnail.thumbnail->front(),
                       static_cast<int>(thumbnail.thumbnail->size()));
  }
  statement.BindString(2, GetRedirects(url));
  const ThumbnailScore& score = thumbnail.thumbnail_score;
  statement.BindDouble(3, score.boring_score);
  statement.BindBool(4, score.good_clipping);
  statement.BindBool(5, score.at_top);
  statement.BindInt64(6,
                      score.time_at_snapshot.since_origin().InMicroseconds());
  statement.BindBool(7, score.load_completed);
  statement.BindInt64(8, url.last_forced_time.since_origin().InMicroseconds());
  statement.BindString(9, url.url.spec());

  return statement.Run();
}

int TopSitesDatabase::GetURLRank(const MostVisitedURL& url) {
  sql::Statement select_statement(
      db_->GetCachedStatement(SQL_FROM_HERE,
                              "SELECT url_rank "
                              "FROM thumbnails WHERE url=?"));
  select_statement.BindString(0, url.url.spec());
  if (select_statement.Step())
    return select_statement.ColumnInt(0);

  return kRankOfNonExistingURL;
}

void TopSitesDatabase::UpdatePageRankNoTransaction(const MostVisitedURL& url,
                                                   int new_rank) {
  DCHECK_GT(db_->transaction_nesting(), 0);
  DCHECK((url.last_forced_time.is_null()) == (new_rank != kRankOfForcedURL))
      << "Thumbnail without a forced time stamp has a forced rank, or the "
      << "opposite.";

  int prev_rank = GetURLRank(url);
  if (prev_rank == kRankOfNonExistingURL) {
    LOG(WARNING) << "Updating rank of an unknown URL: " << url.url.spec();
    return;
  }

  // Shift the ranks.
  if (prev_rank > new_rank) {
    if (new_rank == kRankOfForcedURL) {
      // From non-forced to forced, shift down.
      // Example: 2 -> -1
      // -1, -1, -1, 0, 1, [2 -> -1], [3 -> 2], [4 -> 3]
      sql::Statement shift_statement(
          db_->GetCachedStatement(SQL_FROM_HERE,
                                  "UPDATE thumbnails "
                                  "SET url_rank = url_rank - 1 "
                                  "WHERE url_rank > ?"));
      shift_statement.BindInt(0, prev_rank);
      shift_statement.Run();
    } else {
      // From non-forced to non-forced, shift up.
      // Example: 3 -> 1
      // -1, -1, -1, 0, [1 -> 2], [2 -> 3], [3 -> 1], 4
      sql::Statement shift_statement(
          db_->GetCachedStatement(SQL_FROM_HERE,
                                  "UPDATE thumbnails "
                                  "SET url_rank = url_rank + 1 "
                                  "WHERE url_rank >= ? AND url_rank < ?"));
      shift_statement.BindInt(0, new_rank);
      shift_statement.BindInt(1, prev_rank);
      shift_statement.Run();
    }
  } else if (prev_rank < new_rank) {
    if (prev_rank == kRankOfForcedURL) {
      // From non-forced to forced, shift up.
      // Example: -1 -> 2
      // -1, [-1 -> 2], -1, 0, 1, [2 -> 3], [3 -> 4], [4 -> 5]
      sql::Statement shift_statement(
          db_->GetCachedStatement(SQL_FROM_HERE,
                                  "UPDATE thumbnails "
                                  "SET url_rank = url_rank + 1 "
                                  "WHERE url_rank >= ?"));
      shift_statement.BindInt(0, new_rank);
      shift_statement.Run();
    } else {
      // From non-forced to non-forced, shift down.
      // Example: 1 -> 3.
      // -1, -1, -1, 0, [1 -> 3], [2 -> 1], [3 -> 2], 4
      sql::Statement shift_statement(
          db_->GetCachedStatement(SQL_FROM_HERE,
                                  "UPDATE thumbnails "
                                  "SET url_rank = url_rank - 1 "
                                  "WHERE url_rank > ? AND url_rank <= ?"));
      shift_statement.BindInt(0, prev_rank);
      shift_statement.BindInt(1, new_rank);
      shift_statement.Run();
    }
  }

  // Set the url's rank and last_forced, since the latter changes when a URL
  // goes from forced to non-forced and vice-versa.
  sql::Statement set_statement(
      db_->GetCachedStatement(SQL_FROM_HERE,
                              "UPDATE thumbnails "
                              "SET url_rank = ?, last_forced = ? "
                              "WHERE url == ?"));
  set_statement.BindInt(0, new_rank);
  set_statement.BindInt64(1,
                          url.last_forced_time.since_origin().InMicroseconds());
  set_statement.BindString(2, url.url.spec());
  set_statement.Run();
}

bool TopSitesDatabase::RemoveURLNoTransaction(const MostVisitedURL& url) {
  int old_rank = GetURLRank(url);
  if (old_rank == kRankOfNonExistingURL)
    return true;

  if (old_rank != kRankOfForcedURL) {
    // Decrement all following ranks.
    sql::Statement shift_statement(
        db_->GetCachedStatement(SQL_FROM_HERE,
                                "UPDATE thumbnails "
                                "SET url_rank = url_rank - 1 "
                                "WHERE url_rank > ?"));
    shift_statement.BindInt(0, old_rank);

    if (!shift_statement.Run())
      return false;
  }

  sql::Statement delete_statement(db_->GetCachedStatement(
      SQL_FROM_HERE, "DELETE FROM thumbnails WHERE url = ?"));
  delete_statement.BindString(0, url.url.spec());

  return delete_statement.Run();
}

sql::Connection* TopSitesDatabase::CreateDB(const base::FilePath& db_name) {
  std::unique_ptr<sql::Connection> db(new sql::Connection());
  // Settings copied from ThumbnailDatabase.
  db->set_histogram_tag("TopSites");
  db->set_error_callback(base::Bind(&DatabaseErrorCallback, db.get(), db_name));
  db->set_page_size(4096);
  db->set_cache_size(32);

  if (!db->Open(db_name))
    return nullptr;
  return db.release();
}

}  // namespace history
