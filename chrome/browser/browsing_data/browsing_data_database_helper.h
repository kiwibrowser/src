// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_DATABASE_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_DATABASE_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "storage/browser/database/database_tracker.h"
#include "storage/common/database/database_identifier.h"
#include "url/gurl.h"

class Profile;

// This class fetches database information in the FILE thread, and notifies
// the UI thread upon completion.
// A client of this class need to call StartFetching from the UI thread to
// initiate the flow, and it'll be notified by the callback in its UI
// thread at some later point.
class BrowsingDataDatabaseHelper
    : public base::RefCountedThreadSafe<BrowsingDataDatabaseHelper> {
 public:
  // Contains detailed information about a web database.
  struct DatabaseInfo {
    DatabaseInfo(const storage::DatabaseIdentifier& identifier,
                 const std::string& database_name,
                 const std::string& description,
                 int64_t size,
                 base::Time last_modified);
    DatabaseInfo(const DatabaseInfo& other);
    ~DatabaseInfo();

    storage::DatabaseIdentifier identifier;
    std::string database_name;
    std::string description;
    int64_t size;
    base::Time last_modified;
  };

  using FetchCallback =
      base::OnceCallback<void(const std::list<DatabaseInfo>&)>;

  explicit BrowsingDataDatabaseHelper(Profile* profile);

  // Starts the fetching process, which will notify its completion via
  // callback.
  // This must be called only in the UI thread.
  virtual void StartFetching(FetchCallback callback);

  // Requests a single database to be deleted. This must be called in the UI
  // thread.
  virtual void DeleteDatabase(const std::string& origin_identifier,
                              const std::string& name);

 protected:
  friend class base::RefCountedThreadSafe<BrowsingDataDatabaseHelper>;
  virtual ~BrowsingDataDatabaseHelper();

 private:
  scoped_refptr<storage::DatabaseTracker> tracker_;

  DISALLOW_COPY_AND_ASSIGN(BrowsingDataDatabaseHelper);
};

// This class is a thin wrapper around BrowsingDataDatabaseHelper that does not
// fetch its information from the database tracker, but gets them passed as
// a parameter during construction.
class CannedBrowsingDataDatabaseHelper : public BrowsingDataDatabaseHelper {
 public:
  struct PendingDatabaseInfo {
    PendingDatabaseInfo(const GURL& origin,
                        const std::string& name,
                        const std::string& description);
    ~PendingDatabaseInfo();

    // The operator is needed to store |PendingDatabaseInfo| objects in a set.
    bool operator<(const PendingDatabaseInfo& other) const;

    GURL origin;
    std::string name;
    std::string description;
  };

  explicit CannedBrowsingDataDatabaseHelper(Profile* profile);

  // Add a database to the set of canned databases that is returned by this
  // helper.
  void AddDatabase(const GURL& origin,
                   const std::string& name,
                   const std::string& description);

  // Clear the list of canned databases.
  void Reset();

  // True if no databases are currently stored.
  bool empty() const;

  // Returns the number of currently stored databases.
  size_t GetDatabaseCount() const;

  // Returns the current list of web databases.
  const std::set<PendingDatabaseInfo>& GetPendingDatabaseInfo();

  // BrowsingDataDatabaseHelper implementation.
  void StartFetching(FetchCallback callback) override;
  void DeleteDatabase(const std::string& origin_identifier,
                      const std::string& name) override;

 private:
  ~CannedBrowsingDataDatabaseHelper() override;

  std::set<PendingDatabaseInfo> pending_database_info_;

  DISALLOW_COPY_AND_ASSIGN(CannedBrowsingDataDatabaseHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_DATABASE_HELPER_H_
