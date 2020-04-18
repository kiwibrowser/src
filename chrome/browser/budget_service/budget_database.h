// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BUDGET_SERVICE_BUDGET_DATABASE_H_
#define CHROME_BROWSER_BUDGET_SERVICE_BUDGET_DATABASE_H_

#include <list>
#include <map>
#include <memory>

#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/leveldb_proto/proto_database.h"
#include "third_party/blink/public/platform/modules/budget_service/budget_service.mojom.h"

namespace base {
class Clock;
class Time;
}

namespace budget_service {
class Budget;
}

namespace url {
class Origin;
}

class Profile;

// A class used to asynchronously read and write details of the budget
// assigned to an origin. The class uses an underlying LevelDB.
class BudgetDatabase {
 public:
  // Callback for getting a list of all budget chunks.
  using GetBudgetCallback = blink::mojom::BudgetService::GetBudgetCallback;

  // This is invoked only after the spend has been written to the database.
  using SpendBudgetCallback =
      base::Callback<void(blink::mojom::BudgetServiceErrorType error_type,
                          bool success)>;

  // The database_dir specifies the location of the budget information on disk.
  BudgetDatabase(Profile* profile, const base::FilePath& database_dir);
  ~BudgetDatabase();

  // Get the full budget expectation for the origin. This will return a
  // sequence of time points and the expected budget at those times.
  void GetBudgetDetails(const url::Origin& origin, GetBudgetCallback callback);

  // Spend a particular amount of budget for an origin. The callback indicates
  // whether there was an error and if the origin had enough budget.
  void SpendBudget(const url::Origin& origin,
                   double amount,
                   SpendBudgetCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(BudgetDatabaseTest,
                           DefaultSiteEngagementInIncognitoProfile);
  friend class BudgetDatabaseTest;

  // Used to allow tests to change time for testing.
  void SetClockForTesting(std::unique_ptr<base::Clock> clock);

  // Holds information about individual pieces of awarded budget. There is a
  // one-to-one mapping of these to the chunks in the underlying database.
  struct BudgetChunk {
    BudgetChunk(double amount, base::Time expiration)
        : amount(amount), expiration(expiration) {}
    BudgetChunk(const BudgetChunk& other)
        : amount(other.amount), expiration(other.expiration) {}

    double amount;
    base::Time expiration;
  };

  // Data structure for caching budget information.
  using BudgetChunks = std::list<BudgetChunk>;

  // Holds information about the overall budget for a site. This includes the
  // time the budget was last incremented, as well as a list of budget chunks
  // which have been awarded.
  struct BudgetInfo {
    BudgetInfo();
    BudgetInfo(const BudgetInfo&& other);
    ~BudgetInfo();

    base::Time last_engagement_award;
    BudgetChunks chunks;

    DISALLOW_COPY_AND_ASSIGN(BudgetInfo);
  };

  // Callback for writing budget values to the database.
  using StoreBudgetCallback = base::OnceCallback<void(bool success)>;

  using CacheCallback = base::OnceCallback<void(bool success)>;

  void OnDatabaseInit(bool success);

  bool IsCached(const url::Origin& origin) const;

  double GetBudget(const url::Origin& origin) const;

  void AddToCache(const url::Origin& origin,
                  CacheCallback callback,
                  bool success,
                  std::unique_ptr<budget_service::Budget> budget);

  void GetBudgetAfterSync(const url::Origin& origin,
                          GetBudgetCallback callback,
                          bool success);

  void SpendBudgetAfterSync(const url::Origin& origin,
                            double amount,
                            SpendBudgetCallback callback,
                            bool success);

  void SpendBudgetAfterWrite(SpendBudgetCallback callback, bool success);

  void WriteCachedValuesToDatabase(const url::Origin& origin,
                                   StoreBudgetCallback callback);

  void SyncCache(const url::Origin& origin, CacheCallback callback);
  void SyncLoadedCache(const url::Origin& origin,
                       CacheCallback callback,
                       bool success);

  // Add budget based on engagement with an origin. The method queries for the
  // engagement score of the origin, and then calculates when engagement budget
  // was last awarded and awards a portion of the score based on that.
  // This only writes budget to the cache.
  void AddEngagementBudget(const url::Origin& origin);

  bool CleanupExpiredBudget(const url::Origin& origin);

  // Gets the current Site Engagement Score for |origin|. Will return a fixed
  // score of zero when |profile_| is off the record.
  double GetSiteEngagementScoreForOrigin(const url::Origin& origin) const;

  Profile* profile_;

  // The database for storing budget information.
  std::unique_ptr<leveldb_proto::ProtoDatabase<budget_service::Budget>> db_;

  // Cached data for the origins which have been loaded.
  std::map<url::Origin, BudgetInfo> budget_map_;

  // The clock used to vend times.
  std::unique_ptr<base::Clock> clock_;

  base::WeakPtrFactory<BudgetDatabase> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BudgetDatabase);
};

#endif  // CHROME_BROWSER_BUDGET_SERVICE_BUDGET_DATABASE_H_
