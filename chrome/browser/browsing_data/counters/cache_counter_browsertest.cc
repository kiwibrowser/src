// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Note that this file only tests the basic behavior of the cache counter, as in
// when it counts and when not, when result is nonzero and when not. It does not
// test whether the result of the counting is correct. This is the
// responsibility of a lower layer, and is tested in
// DiskCacheBackendTest.CalculateSizeOfAllEntries in net_unittests.

#include "chrome/browser/browsing_data/counters/cache_counter.h"

#include "base/run_loop.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/browsing_data/core/browsing_data_utils.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

using content::BrowserContext;
using content::BrowserThread;

namespace {

class CacheCounterTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    SetCacheDeletionPref(true);
    SetDeletionPeriodPref(browsing_data::TimePeriod::ALL_TIME);
  }

  void SetCacheDeletionPref(bool value) {
    browser()->profile()->GetPrefs()->SetBoolean(
        browsing_data::prefs::kDeleteCache, value);
  }

  void SetDeletionPeriodPref(browsing_data::TimePeriod period) {
    browser()->profile()->GetPrefs()->SetInteger(
        browsing_data::prefs::kDeleteTimePeriod, static_cast<int>(period));
  }

  // One step in the process of creating a cache entry. Every step must be
  // executed on IO thread after the previous one has finished.
  void CreateCacheEntryStep(int return_value) {
    net::CompletionCallback callback =
        base::Bind(&CacheCounterTest::CreateCacheEntryStep,
                   base::Unretained(this));

    switch (next_step_) {
      case GET_CACHE: {
        next_step_ = CREATE_ENTRY;
        net::URLRequestContextGetter* context_getter =
            storage_partition_->GetURLRequestContext();
        net::HttpCache* http_cache = context_getter->GetURLRequestContext()->
            http_transaction_factory()->GetCache();
        return_value = http_cache->GetBackend(&backend_, callback);
        break;
      }

      case CREATE_ENTRY: {
        next_step_ = WRITE_DATA;
        return_value = backend_->CreateEntry("entry_key", &entry_, callback);
        break;
      }

      case WRITE_DATA: {
        next_step_ = DONE;
        std::string data = "entry data";
        scoped_refptr<net::StringIOBuffer> buffer =
            new net::StringIOBuffer(data);
        return_value =
            entry_->WriteData(0, 0, buffer.get(), data.size(), callback, true);
        break;
      }

      case DONE: {
        entry_->Close();
        BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                                base::BindOnce(&CacheCounterTest::Callback,
                                               base::Unretained(this)));
        return;
      }
    }

    if (return_value >= 0) {
      // Success.
      CreateCacheEntryStep(net::OK);
    } else if (return_value == net::ERR_IO_PENDING) {
      // The callback will trigger the next step.
    } else {
      // Error.
      NOTREACHED();
    }
  }

  // Create a cache entry on the IO thread.
  void CreateCacheEntry() {
    storage_partition_ =
        BrowserContext::GetDefaultStoragePartition(browser()->profile());
    next_step_ = GET_CACHE;

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&CacheCounterTest::CreateCacheEntryStep,
                       base::Unretained(this), net::OK));
    WaitForIOThread();
  }

  // Wait for IO thread operations, such as cache creation, counting, writing,
  // deletion etc.
  void WaitForIOThread() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

  // General completion callback.
  void Callback() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    if (run_loop_)
      run_loop_->Quit();
  }

  // Callback from the counter.
  void CountingCallback(
      std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    finished_ = result->Finished();

    if (finished_) {
      auto* cache_result =
          static_cast<CacheCounter::CacheResult*>(result.get());
      result_ = cache_result->cache_size();
      is_upper_limit_ = cache_result->is_upper_limit();
    }

    if (run_loop_ && finished_)
      run_loop_->Quit();
  }

  browsing_data::BrowsingDataCounter::ResultInt GetResult() {
    DCHECK(finished_);
    return result_;
  }

  bool IsUpperLimit() {
    DCHECK(finished_);
    return is_upper_limit_;
  }

 private:
  enum CacheEntryCreationStep {
    GET_CACHE,
    CREATE_ENTRY,
    WRITE_DATA,
    DONE
  };
  CacheEntryCreationStep next_step_;
  content::StoragePartition* storage_partition_;
  disk_cache::Backend* backend_;
  disk_cache::Entry* entry_;

  std::unique_ptr<base::RunLoop> run_loop_;

  bool finished_;
  browsing_data::BrowsingDataCounter::ResultInt result_;
  bool is_upper_limit_;
};

// Tests that for the empty cache, the result is zero.
IN_PROC_BROWSER_TEST_F(CacheCounterTest, Empty) {
  Profile* profile = browser()->profile();

  CacheCounter counter(profile);
  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&CacheCounterTest::CountingCallback, base::Unretained(this)));
  counter.Restart();

  WaitForIOThread();
  EXPECT_EQ(0u, GetResult());
}

// Tests that for a non-empty cache, the result is nonzero.
IN_PROC_BROWSER_TEST_F(CacheCounterTest, NonEmpty) {
  CreateCacheEntry();

  Profile* profile = browser()->profile();
  CacheCounter counter(profile);
  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&CacheCounterTest::CountingCallback, base::Unretained(this)));
  counter.Restart();

  WaitForIOThread();

  EXPECT_NE(0u, GetResult());
}

// Tests that after dooming a nonempty cache, the result is zero.
IN_PROC_BROWSER_TEST_F(CacheCounterTest, AfterDoom) {
  CreateCacheEntry();

  Profile* profile = browser()->profile();
  CacheCounter counter(profile);
  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&CacheCounterTest::CountingCallback, base::Unretained(this)));

  content::BrowserContext::GetDefaultStoragePartition(browser()->profile())
      ->ClearHttpAndMediaCaches(
          base::Time(), base::Time::Max(), base::Callback<bool(const GURL&)>(),
          base::Bind(&CacheCounter::Restart, base::Unretained(&counter)));

  WaitForIOThread();
  EXPECT_EQ(0u, GetResult());
}

// Tests that the counter starts counting automatically when the deletion
// pref changes to true.
IN_PROC_BROWSER_TEST_F(CacheCounterTest, PrefChanged) {
  SetCacheDeletionPref(false);

  Profile* profile = browser()->profile();
  CacheCounter counter(profile);
  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&CacheCounterTest::CountingCallback, base::Unretained(this)));
  SetCacheDeletionPref(true);

  WaitForIOThread();
  EXPECT_EQ(0u, GetResult());
}

// Tests that the counting is restarted when the time period changes.
IN_PROC_BROWSER_TEST_F(CacheCounterTest, PeriodChanged) {
  CreateCacheEntry();

  Profile* profile = browser()->profile();
  CacheCounter counter(profile);
  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&CacheCounterTest::CountingCallback, base::Unretained(this)));

  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_HOUR);
  WaitForIOThread();
  browsing_data::BrowsingDataCounter::ResultInt result = GetResult();

  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_DAY);
  WaitForIOThread();
  EXPECT_EQ(result, GetResult());

  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_WEEK);
  WaitForIOThread();
  EXPECT_EQ(result, GetResult());

  SetDeletionPeriodPref(browsing_data::TimePeriod::FOUR_WEEKS);
  WaitForIOThread();
  EXPECT_EQ(result, GetResult());

  SetDeletionPeriodPref(browsing_data::TimePeriod::ALL_TIME);
  WaitForIOThread();
  EXPECT_EQ(result, GetResult());
  EXPECT_FALSE(IsUpperLimit());
}

}  // namespace
