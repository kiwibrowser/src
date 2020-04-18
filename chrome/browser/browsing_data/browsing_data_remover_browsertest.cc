// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data/browsing_data_helper.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"
#include "chrome/browser/browsing_data/counters/cache_counter.h"
#include "chrome/browser/browsing_data/counters/site_data_counting_helper.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/browsing_data/core/browsing_data_utils.h"
#include "components/browsing_data/core/features.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/browsing_data_remover_test_util.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/simple_url_loader_test_helper.h"
#include "media/mojo/services/video_decode_perf_history.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::BrowserThread;
using content::BrowsingDataFilterBuilder;

namespace {
static const char* kExampleHost = "example.com";
}

class BrowsingDataRemoverBrowserTest : public InProcessBrowserTest {
 public:
  BrowsingDataRemoverBrowserTest() {}

  void SetUpOnMainThread() override {
    feature_list_.InitWithFeatures(
        {browsing_data::features::kRemoveNavigationHistory}, {});
    base::FilePath path;
    base::PathService::Get(content::DIR_TEST_DATA, &path);
    host_resolver()->AddRule(kExampleHost, "127.0.0.1");
    embedded_test_server()->ServeFilesFromDirectory(path);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void RunScriptAndCheckResult(const std::string& script,
                               const std::string& result) {
    std::string data;
    ASSERT_TRUE(content::ExecuteScriptAndExtractString(
        browser()->tab_strip_model()->GetActiveWebContents(), script, &data));
    ASSERT_EQ(data, result);
  }

  bool RunScriptAndGetBool(const std::string& script) {
    bool data;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        browser()->tab_strip_model()->GetActiveWebContents(), script, &data));
    return data;
  }

  void VerifyDownloadCount(size_t expected) {
    content::DownloadManager* download_manager =
        content::BrowserContext::GetDownloadManager(browser()->profile());
    std::vector<download::DownloadItem*> downloads;
    download_manager->GetAllDownloads(&downloads);
    EXPECT_EQ(expected, downloads.size());
  }

  void DownloadAnItem() {
    // Start a download.
    content::DownloadManager* download_manager =
        content::BrowserContext::GetDownloadManager(browser()->profile());
    std::unique_ptr<content::DownloadTestObserver> observer(
        new content::DownloadTestObserverTerminal(
            download_manager, 1,
            content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_ACCEPT));

    GURL download_url = ui_test_utils::GetTestUrl(
        base::FilePath().AppendASCII("downloads"),
        base::FilePath().AppendASCII("a_zip_file.zip"));
    ui_test_utils::NavigateToURL(browser(), download_url);
    observer->WaitForFinished();

    VerifyDownloadCount(1u);
  }

  void RemoveAndWait(int remove_mask) {
    content::BrowsingDataRemover* remover =
        content::BrowserContext::GetBrowsingDataRemover(browser()->profile());
    content::BrowsingDataRemoverCompletionObserver completion_observer(remover);
    remover->RemoveAndReply(
        base::Time(), base::Time::Max(), remove_mask,
        content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB,
        &completion_observer);
    completion_observer.BlockUntilCompletion();
  }

  void RemoveWithFilterAndWait(
      int remove_mask,
      std::unique_ptr<BrowsingDataFilterBuilder> filter_builder) {
    content::BrowsingDataRemover* remover =
        content::BrowserContext::GetBrowsingDataRemover(browser()->profile());
    content::BrowsingDataRemoverCompletionObserver completion_observer(remover);
    remover->RemoveWithFilterAndReply(
        base::Time(), base::Time::Max(), remove_mask,
        content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB,
        std::move(filter_builder), &completion_observer);
    completion_observer.BlockUntilCompletion();
  }

  // Test a data type by creating a value and checking it is counted by the
  // cookie counter. Then it deletes the value and checks that it has been
  // deleted and the cookie counter is back to zero.
  void TestSiteData(const std::string& type) {
    EXPECT_EQ(0, GetSiteDataCount());
    GURL url = embedded_test_server()->GetURL("/browsing_data/site_data.html");
    ui_test_utils::NavigateToURL(browser(), url);

    EXPECT_EQ(0, GetSiteDataCount());
    EXPECT_FALSE(HasDataForType(type));

    SetDataForType(type);
    EXPECT_EQ(1, GetSiteDataCount());
    EXPECT_TRUE(HasDataForType(type));

    RemoveAndWait(ChromeBrowsingDataRemoverDelegate::DATA_TYPE_SITE_DATA);
    EXPECT_EQ(0, GetSiteDataCount());
    EXPECT_FALSE(HasDataForType(type));
  }

  // Test that storage systems like filesystem and websql, where just an access
  // creates an empty store, are counted and deleted correctly.
  void TestEmptySiteData(const std::string& type) {
    EXPECT_EQ(0, GetSiteDataCount());
    GURL url = embedded_test_server()->GetURL("/browsing_data/site_data.html");
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(0, GetSiteDataCount());
    // Opening a store of this type creates a site data entry.
    EXPECT_FALSE(HasDataForType(type));
    EXPECT_EQ(1, GetSiteDataCount());
    RemoveAndWait(ChromeBrowsingDataRemoverDelegate::DATA_TYPE_SITE_DATA);
    EXPECT_EQ(0, GetSiteDataCount());
  }

  bool HasDataForType(const std::string& type) {
    return RunScriptAndGetBool("has" + type + "()");
  }

  void SetDataForType(const std::string& type) {
    ASSERT_TRUE(RunScriptAndGetBool("set" + type + "()"));
  }

  int GetSiteDataCount() {
    base::RunLoop run_loop;
    int count = -1;
    (new SiteDataCountingHelper(
         browser()->profile(), base::Time(),
         base::Bind(&BrowsingDataRemoverBrowserTest::OnCookieCountResult,
                    base::Unretained(this), base::Unretained(&run_loop),
                    base::Unretained(&count))))
        ->CountAndDestroySelfWhenFinished();

    run_loop.Run();
    return count;
  }

  void OnVideoDecodePerfInfo(base::RunLoop* run_loop,
                             bool* out_is_smooth,
                             bool* out_is_power_efficient,
                             bool is_smooth,
                             bool is_power_efficient) {
    *out_is_smooth = is_smooth;
    *out_is_power_efficient = is_power_efficient;
    run_loop->QuitWhenIdle();
  }

  network::mojom::NetworkContext* network_context() const {
    return content::BrowserContext::GetDefaultStoragePartition(
               browser()->profile())
        ->GetNetworkContext();
  }

 private:
  void OnCacheSizeResult(
      base::RunLoop* run_loop,
      browsing_data::BrowsingDataCounter::ResultInt* out_size,
      std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result) {
    if (!result->Finished())
      return;

    *out_size =
        static_cast<browsing_data::BrowsingDataCounter::FinishedResult*>(
            result.get())
            ->Value();
    run_loop->Quit();
  }

  void OnCookieCountResult(base::RunLoop* run_loop, int* out_count, int count) {
    *out_count = count;
    run_loop->Quit();
  }

  base::test::ScopedFeatureList feature_list_;
};

// Test BrowsingDataRemover for downloads.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, Download) {
  DownloadAnItem();
  RemoveAndWait(content::BrowsingDataRemover::DATA_TYPE_DOWNLOADS);
  VerifyDownloadCount(0u);
}

// Test that the salt for media device IDs is reset when cookies are cleared.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, MediaDeviceIdSalt) {
  std::string original_salt = browser()->profile()->GetMediaDeviceIDSalt();
  RemoveAndWait(content::BrowsingDataRemover::DATA_TYPE_COOKIES);
  std::string new_salt = browser()->profile()->GetMediaDeviceIDSalt();
  EXPECT_NE(original_salt, new_salt);
}

// The call to Remove() should crash in debug (DCHECK), but the browser-test
// process model prevents using a death test.
#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
// Test BrowsingDataRemover for prohibited downloads. Note that this only
// really exercises the code in a Release build.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, DownloadProhibited) {
  PrefService* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(prefs::kAllowDeletingBrowserHistory, false);

  DownloadAnItem();
  RemoveAndWait(content::BrowsingDataRemover::DATA_TYPE_DOWNLOADS);
  VerifyDownloadCount(1u);
}
#endif

// Verify VideoDecodePerfHistory is cleared when deleting all history from
// beginning of time.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, VideoDecodePerfHistory) {
  media::VideoDecodePerfHistory* video_decode_perf_history =
      browser()->profile()->GetVideoDecodePerfHistory();

  // Save a video decode record. Note: we avoid using a web page to generate the
  // stats as this takes at least 5 seconds and even then is not a guarantee
  // depending on scheduler. Manual injection is quick and non-flaky.
  const media::VideoCodecProfile kProfile = media::VP9PROFILE_PROFILE0;
  const gfx::Size kSize(100, 200);
  const int kFrameRate = 30;
  const int kFramesDecoded = 1000;
  const int kFramesDropped = .9 * kFramesDecoded;
  const int kFramesPowerEfficient = 0;
  const url::Origin kOrigin = url::Origin::Create(GURL("http://example.com"));
  const bool kIsTopFrame = true;
  const uint64_t kPlayerId = 1234u;

  media::mojom::PredictionFeatures prediction_features;
  prediction_features.profile = kProfile;
  prediction_features.video_size = kSize;
  prediction_features.frames_per_sec = kFrameRate;

  media::mojom::PredictionTargets prediction_targets;
  prediction_targets.frames_decoded = kFramesDecoded;
  prediction_targets.frames_dropped = kFramesDropped;
  prediction_targets.frames_decoded_power_efficient = kFramesPowerEfficient;

  {
    base::RunLoop run_loop;
    video_decode_perf_history->SavePerfRecord(
        kOrigin, kIsTopFrame, prediction_features, prediction_targets,
        kPlayerId, run_loop.QuitWhenIdleClosure());
    run_loop.Run();
  }

  // Verify history exists.
  // Expect |is_smooth| = false and |is_power_efficient| = false given that 90%
  // of recorded frames were dropped and 0 were power efficient.
  bool is_smooth = true;
  bool is_power_efficient = true;
  {
    base::RunLoop run_loop;
    video_decode_perf_history->GetPerfInfo(
        media::mojom::PredictionFeatures::New(prediction_features),
        base::BindOnce(&BrowsingDataRemoverBrowserTest::OnVideoDecodePerfInfo,
                       base::Unretained(this), &run_loop, &is_smooth,
                       &is_power_efficient));
    run_loop.Run();
  }
  EXPECT_FALSE(is_smooth);
  EXPECT_FALSE(is_power_efficient);

  // Clear history.
  RemoveAndWait(ChromeBrowsingDataRemoverDelegate::DATA_TYPE_HISTORY);

  // Verify history no longer exists. Both |is_smooth| and |is_power_efficient|
  // should now report true because the VideoDecodePerfHistory optimistically
  // returns true when it has no data.
  {
    base::RunLoop run_loop;
    video_decode_perf_history->GetPerfInfo(
        media::mojom::PredictionFeatures::New(prediction_features),
        base::BindOnce(&BrowsingDataRemoverBrowserTest::OnVideoDecodePerfInfo,
                       base::Unretained(this), &run_loop, &is_smooth,
                       &is_power_efficient));
    run_loop.Run();
  }
  EXPECT_TRUE(is_smooth);
  EXPECT_TRUE(is_power_efficient);
}

// Verify can modify database after deleting it.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, Database) {
  GURL url = embedded_test_server()->GetURL("/simple_database.html");
  ui_test_utils::NavigateToURL(browser(), url);

  RunScriptAndCheckResult("createTable()", "done");
  RunScriptAndCheckResult("insertRecord('text')", "done");
  RunScriptAndCheckResult("getRecords()", "text");

  RemoveAndWait(ChromeBrowsingDataRemoverDelegate::DATA_TYPE_SITE_DATA);

  ui_test_utils::NavigateToURL(browser(), url);
  RunScriptAndCheckResult("createTable()", "done");
  RunScriptAndCheckResult("insertRecord('text2')", "done");
  RunScriptAndCheckResult("getRecords()", "text2");
}

// Verifies that cache deletion finishes successfully. Completes deletion of
// cache should leave it empty, and partial deletion should leave nonzero
// amount of data. Note that this tests the integration of BrowsingDataRemover
// with ConditionalCacheDeletionHelper. Whether ConditionalCacheDeletionHelper
// actually deletes the correct entries is tested
// in ConditionalCacheDeletionHelperBrowsertest.
// TODO(crbug.com/817417): check the cache size instead of stopping the server
// and loading the request again.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, Cache) {
  // Load several resources.
  GURL url1 = embedded_test_server()->GetURL("/cachetime");
  GURL url2 = embedded_test_server()->GetURL(kExampleHost, "/cachetime");
  ASSERT_FALSE(url::IsSameOriginWith(url1, url2));

  EXPECT_EQ(net::OK, content::LoadBasicRequest(network_context(), url1));
  EXPECT_EQ(net::OK, content::LoadBasicRequest(network_context(), url2));

  // Check that the cache has been populated by revisiting these pages with the
  // server stopped.
  ASSERT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
  EXPECT_EQ(net::OK, content::LoadBasicRequest(network_context(), url1));
  EXPECT_EQ(net::OK, content::LoadBasicRequest(network_context(), url2));

  // Partially delete cache data. Delete data for localhost, which is the origin
  // of |url1|, but not for |kExampleHost|, which is the origin of |url2|.
  std::unique_ptr<BrowsingDataFilterBuilder> filter_builder =
      BrowsingDataFilterBuilder::Create(BrowsingDataFilterBuilder::WHITELIST);
  filter_builder->AddOrigin(url::Origin::Create(url1));
  RemoveWithFilterAndWait(content::BrowsingDataRemover::DATA_TYPE_CACHE,
                          std::move(filter_builder));

  // After the partial deletion, the cache should be smaller but still nonempty.
  EXPECT_NE(net::OK, content::LoadBasicRequest(network_context(), url1));
  EXPECT_EQ(net::OK, content::LoadBasicRequest(network_context(), url2));

  // Another partial deletion with the same filter should have no effect.
  filter_builder =
      BrowsingDataFilterBuilder::Create(BrowsingDataFilterBuilder::WHITELIST);
  filter_builder->AddOrigin(url::Origin::Create(url1));
  RemoveWithFilterAndWait(content::BrowsingDataRemover::DATA_TYPE_CACHE,
                          std::move(filter_builder));
  EXPECT_NE(net::OK, content::LoadBasicRequest(network_context(), url1));
  EXPECT_EQ(net::OK, content::LoadBasicRequest(network_context(), url2));

  // Delete the remaining data.
  RemoveAndWait(content::BrowsingDataRemover::DATA_TYPE_CACHE);

  // The cache should be empty.
  EXPECT_NE(net::OK, content::LoadBasicRequest(network_context(), url1));
  EXPECT_NE(net::OK, content::LoadBasicRequest(network_context(), url2));
}

// Crashes the network service while clearing the HTTP cache to make sure the
// clear operation does complete.
// Note that there is a race between crashing the network service and clearing
// the cache, so the test might flakily fail if the tested behavior does not
// work.
// TODO(crbug.com/813882): test retry behavior by validating the cache is empty
// after the crash.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest,
                       ClearCacheAndNetworkServiceCrashes) {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService))
    return;

  // Clear the cached data with a task posted to crash the network service.
  // The task should be run while waiting for the cache clearing operation to
  // complete, hopefully it happens before the cache has been cleared.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce([]() { content::SimulateNetworkServiceCrash(); }));

  RemoveAndWait(content::BrowsingDataRemover::DATA_TYPE_CACHE);
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest,
                       ExternalProtocolHandlerPrefs) {
  Profile* profile = browser()->profile();
  base::DictionaryValue prefs;
  prefs.SetBoolean("tel", false);
  profile->GetPrefs()->Set(prefs::kExcludedSchemes, prefs);
  ExternalProtocolHandler::BlockState block_state =
      ExternalProtocolHandler::GetBlockState("tel", profile);
  ASSERT_EQ(ExternalProtocolHandler::DONT_BLOCK, block_state);
  RemoveAndWait(ChromeBrowsingDataRemoverDelegate::DATA_TYPE_SITE_DATA);
  block_state = ExternalProtocolHandler::GetBlockState("tel", profile);
  ASSERT_EQ(ExternalProtocolHandler::UNKNOWN, block_state);
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, HistoryDeletion) {
  const std::string kType = "History";
  GURL url = embedded_test_server()->GetURL("/browsing_data/site_data.html");
  // Create a new tab to avoid confusion from having a NTP navigation entry.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  EXPECT_FALSE(HasDataForType(kType));
  SetDataForType(kType);
  EXPECT_TRUE(HasDataForType(kType));
  // Remove history from navigation to site_data.html.
  RemoveAndWait(ChromeBrowsingDataRemoverDelegate::DATA_TYPE_HISTORY);
  EXPECT_FALSE(HasDataForType(kType));
  SetDataForType(kType);
  EXPECT_TRUE(HasDataForType(kType));
  // Remove history from previous pushState() call in setHistory().
  RemoveAndWait(ChromeBrowsingDataRemoverDelegate::DATA_TYPE_HISTORY);
  EXPECT_FALSE(HasDataForType(kType));
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, CookieDeletion) {
  TestSiteData("Cookie");
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, LocalStorageDeletion) {
  TestSiteData("LocalStorage");
}

// TODO(crbug.com/772337): DISABLED until session storage is working correctly.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest,
                       DISABLED_SessionStorageDeletion) {
  TestSiteData("SessionStorage");
}

// Test that session storage is not counted until crbug.com/772337 is fixed.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, SessionStorageCounting) {
  EXPECT_EQ(0, GetSiteDataCount());
  GURL url = embedded_test_server()->GetURL("/browsing_data/site_data.html");
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_EQ(0, GetSiteDataCount());
  SetDataForType("SessionStorage");
  EXPECT_EQ(0, GetSiteDataCount());
  EXPECT_TRUE(HasDataForType("SessionStorage"));
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, ServiceWorkerDeletion) {
  TestSiteData("ServiceWorker");
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, CacheStorageDeletion) {
  TestSiteData("CacheStorage");
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, FileSystemDeletion) {
  TestSiteData("FileSystem");
}

// Test that empty filesystems are deleted correctly.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, EmptyFileSystem) {
  TestEmptySiteData("FileSystem");
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, WebSqlDeletion) {
  TestSiteData("WebSql");
}

// Test that empty websql dbs are deleted correctly.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, EmptyWebSql) {
  TestEmptySiteData("WebSql");
}

IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, IndexedDbDeletion) {
  TestSiteData("IndexedDb");
}

// Test that empty indexed dbs are deleted correctly.
IN_PROC_BROWSER_TEST_F(BrowsingDataRemoverBrowserTest, EmptyIndexedDb) {
  TestEmptySiteData("IndexedDb");
}
