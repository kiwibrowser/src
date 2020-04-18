// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "chrome/browser/background_fetch/background_fetch_delegate_impl.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/offline_items_collection/offline_content_aggregator_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/download/public/background_service/download_service.h"
#include "components/download/public/background_service/logger.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "components/offline_items_collection/core/offline_content_provider.h"
#include "components/offline_items_collection/core/offline_item.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/origin.h"

using offline_items_collection::ContentId;
using offline_items_collection::OfflineContentProvider;
using offline_items_collection::OfflineItem;
using offline_items_collection::OfflineItemFilter;
using offline_items_collection::OfflineItemProgressUnit;
using offline_items_collection::OfflineItemVisuals;

namespace {

// URL of the test helper page that helps drive these tests.
const char kHelperPage[] = "/background_fetch/background_fetch.html";

// Name of the Background Fetch client as known by the download service.
const char kBackgroundFetchClient[] = "BackgroundFetch";

// Stringified values of request services made to the download service.
const char kResultAccepted[] = "ACCEPTED";

// Title of a Background Fetch started by StartSingleFileDownload().
const char kSingleFileDownloadTitle[] = "Single-file Background Fetch";

// Size of the downloaded resource, used in BackgroundFetch tests.
const int kDownloadedResourceSizeInBytes = 82;

// Incorrect downloadTotal, when it's set too high in the JavaScript file
// loaded by this test (chrome/test/data/background_fetch/background_fetch.js)
const int kDownloadTotalBytesTooHigh = 1000;

// Incorrect downloadTotal, when it's set too low in the JavaScript file
// loaded by this test (chrome/test/data/background_fetch/background_fetch.js)
const int kDownloadTotalBytesTooLow = 80;

// Implementation of a download system logger that provides the ability to wait
// for certain events to happen, notably added and progressing downloads.
class WaitableDownloadLoggerObserver : public download::Logger::Observer {
 public:
  using DownloadAcceptedCallback =
      base::OnceCallback<void(const std::string& guid)>;

  WaitableDownloadLoggerObserver() = default;
  ~WaitableDownloadLoggerObserver() override = default;

  // Sets the |callback| to be invoked when a download has been accepted.
  void set_download_accepted_callback(DownloadAcceptedCallback callback) {
    download_accepted_callback_ = std::move(callback);
  }

  // download::Logger::Observer implementation:
  void OnServiceStatusChanged(const base::Value& service_status) override {}
  void OnServiceDownloadsAvailable(
      const base::Value& service_downloads) override {}
  void OnServiceDownloadChanged(const base::Value& service_download) override {}
  void OnServiceDownloadFailed(const base::Value& service_download) override {}
  void OnServiceRequestMade(const base::Value& service_request) override {
    const std::string& client = service_request.FindKey("client")->GetString();
    const std::string& guid = service_request.FindKey("guid")->GetString();
    const std::string& result = service_request.FindKey("result")->GetString();

    if (client != kBackgroundFetchClient)
      return;  // this event is not targeted to us

    if (result == kResultAccepted && download_accepted_callback_)
      std::move(download_accepted_callback_).Run(guid);
  }

 private:
  DownloadAcceptedCallback download_accepted_callback_;

  DISALLOW_COPY_AND_ASSIGN(WaitableDownloadLoggerObserver);
};

// Observes the offline item collection's content provider and then invokes the
// associated test callbacks when one has been provided.
class OfflineContentProviderObserver : public OfflineContentProvider::Observer {
 public:
  using ItemsAddedCallback =
      base::OnceCallback<void(const std::vector<OfflineItem>&)>;
  using FinishedProcessingItemCallback =
      base::OnceCallback<void(const OfflineItem&)>;

  OfflineContentProviderObserver() = default;
  ~OfflineContentProviderObserver() final = default;

  void set_items_added_callback(ItemsAddedCallback callback) {
    items_added_callback_ = std::move(callback);
  }

  void set_finished_processing_item_callback(
      FinishedProcessingItemCallback callback) {
    finished_processing_item_callback_ = std::move(callback);
  }

  // OfflineContentProvider::Observer implementation:
  void OnItemsAdded(
      const OfflineContentProvider::OfflineItemList& items) override {
    if (items_added_callback_)
      std::move(items_added_callback_).Run(items);
  }

  void OnItemRemoved(const ContentId& id) override {}
  void OnItemUpdated(const OfflineItem& item) override {
    if (item.state != offline_items_collection::OfflineItemState::IN_PROGRESS &&
        item.state != offline_items_collection::OfflineItemState::PENDING &&
        finished_processing_item_callback_)
      std::move(finished_processing_item_callback_).Run(item);
  }

 private:
  ItemsAddedCallback items_added_callback_;
  FinishedProcessingItemCallback finished_processing_item_callback_;

  DISALLOW_COPY_AND_ASSIGN(OfflineContentProviderObserver);
};

class BackgroundFetchBrowserTest : public InProcessBrowserTest {
 public:
  BackgroundFetchBrowserTest()
      : offline_content_provider_observer_(
            std::make_unique<OfflineContentProviderObserver>()) {}
  ~BackgroundFetchBrowserTest() override = default;

  // InProcessBrowserTest overrides:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Background Fetch is available as an experimental Web Platform feature.
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
  }

  void SetUpOnMainThread() override {
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_->Start());

    Profile* profile = browser()->profile();

    download_observer_ = std::make_unique<WaitableDownloadLoggerObserver>();

    download_service_ = DownloadServiceFactory::GetForBrowserContext(profile);
    download_service_->GetLogger()->AddObserver(download_observer_.get());

    // Register our observer for the offline items collection.
    OfflineContentAggregatorFactory::GetInstance()
        ->GetForBrowserContext(profile)
        ->AddObserver(offline_content_provider_observer_.get());

    // Load the helper page that helps drive these tests.
    ui_test_utils::NavigateToURL(browser(), https_server_->GetURL(kHelperPage));

    // Register the Service Worker that's required for Background Fetch. The
    // behaviour without an activated worker is covered by layout tests.
    {
      std::string script_result;
      ASSERT_TRUE(RunScript("RegisterServiceWorker()", &script_result));
      ASSERT_EQ("ok - service worker registered", script_result);
    }
  }

  void TearDownOnMainThread() override {
    OfflineContentAggregatorFactory::GetInstance()
        ->GetForBrowserContext(browser()->profile())
        ->RemoveObserver(offline_content_provider_observer_.get());

    download_service_->GetLogger()->RemoveObserver(download_observer_.get());
    download_service_ = nullptr;
  }

  // ---------------------------------------------------------------------------
  // Test execution functions.

  // Runs the |script| and waits for one or more items to have been added to the
  // offline items collection. Wrap in ASSERT_NO_FATAL_FAILURE().
  void RunScriptAndWaitForOfflineItems(const std::string& script,
                                       std::vector<OfflineItem>* items) {
    DCHECK(items);

    base::RunLoop run_loop;
    offline_content_provider_observer_->set_items_added_callback(
        base::BindOnce(&BackgroundFetchBrowserTest::DidAddItems,
                       base::Unretained(this), run_loop.QuitClosure(), items));

    std::string result;
    ASSERT_NO_FATAL_FAILURE(RunScript(script, &result));
    ASSERT_EQ("ok", result);

    run_loop.Run();
  }

  void GetVisualsForOfflineItemSync(
      const ContentId& offline_item_id,
      std::unique_ptr<OfflineItemVisuals>* out_visuals) {
    base::RunLoop run_loop;
    BackgroundFetchDelegateImpl* delegate =
        static_cast<BackgroundFetchDelegateImpl*>(
            browser()->profile()->GetBackgroundFetchDelegate());
    DCHECK(delegate);
    delegate->GetVisualsForItem(
        offline_item_id, base::Bind(&BackgroundFetchBrowserTest::DidGetVisuals,
                                    base::Unretained(this),
                                    run_loop.QuitClosure(), out_visuals));
    run_loop.Run();
  }

  // ---------------------------------------------------------------------------
  // Helper functions.

  // Helper function for getting the expected title given the |developer_title|.
  std::string GetExpectedTitle(const char* developer_title) {
    url::Origin origin = url::Origin::Create(https_server_->base_url());

    return base::StringPrintf("%s (%s)", developer_title,
                              origin.Serialize().c_str());
  }

  // Runs the |script| in the current tab and writes the output to |*result|.
  bool RunScript(const std::string& script, std::string* result) {
    return content::ExecuteScriptAndExtractString(
        browser()->tab_strip_model()->GetActiveWebContents()->GetMainFrame(),
        script, result);
  }

  // Runs the given |function| and asserts that it responds with "ok".
  // Must be wrapped with ASSERT_NO_FATAL_FAILURE().
  void RunScriptFunction(const std::string& function) {
    std::string result;
    ASSERT_TRUE(RunScript(function, &result));
    ASSERT_EQ("ok", result);
  }

  // Callback for WaitableDownloadLoggerObserver::DownloadAcceptedCallback().
  void DidAcceptDownloadCallback(base::OnceClosure quit_closure,
                                 std::string* out_guid,
                                 const std::string& guid) {
    DCHECK(out_guid);
    *out_guid = guid;

    std::move(quit_closure).Run();
  }

  // Called when the an offline item has been processed.
  void DidFinishProcessingItem(base::OnceClosure quit_closure,
                               OfflineItem* out_item,
                               const OfflineItem& processed_item) {
    DCHECK(out_item);
    *out_item = processed_item;
    std::move(quit_closure).Run();
  }

 protected:
  download::DownloadService* download_service_{nullptr};

  std::unique_ptr<WaitableDownloadLoggerObserver> download_observer_;
  std::unique_ptr<OfflineContentProviderObserver>
      offline_content_provider_observer_;

 private:
  // Callback for RunScriptAndWaitForOfflineItems(), called when the |items|
  // have been added to the offline items collection.
  void DidAddItems(base::OnceClosure quit_closure,
                   std::vector<OfflineItem>* out_items,
                   const std::vector<OfflineItem>& items) {
    *out_items = items;
    std::move(quit_closure).Run();
  }

  void DidGetVisuals(base::OnceClosure quit_closure,
                     std::unique_ptr<OfflineItemVisuals>* out_visuals,
                     const ContentId& offline_item_id,
                     std::unique_ptr<OfflineItemVisuals> visuals) {
    *out_visuals = std::move(visuals);
    std::move(quit_closure).Run();
  }

  std::unique_ptr<net::EmbeddedTestServer> https_server_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchBrowserTest);
};

IN_PROC_BROWSER_TEST_F(BackgroundFetchBrowserTest, DownloadService_Acceptance) {
  // Starts a Background Fetch for a single to-be-downloaded file and waits for
  // that request to be scheduled with the Download Service.

  std::string guid;
  {
    base::RunLoop run_loop;
    download_observer_->set_download_accepted_callback(
        base::BindOnce(&BackgroundFetchBrowserTest::DidAcceptDownloadCallback,
                       base::Unretained(this), run_loop.QuitClosure(), &guid));

    ASSERT_NO_FATAL_FAILURE(RunScriptFunction("StartSingleFileDownload()"));
    run_loop.Run();
  }

  EXPECT_FALSE(guid.empty());
}

IN_PROC_BROWSER_TEST_F(BackgroundFetchBrowserTest,
                       OfflineItemCollection_SingleFileMetadata) {
  // Starts a Background Fetch for a single to-be-downloaded file and waits for
  // the fetch to be registered with the offline items collection. We then
  // verify that all the appropriate values have been set.

  std::vector<OfflineItem> items;
  ASSERT_NO_FATAL_FAILURE(
      RunScriptAndWaitForOfflineItems("StartSingleFileDownload()", &items));
  ASSERT_EQ(items.size(), 1u);

  const OfflineItem& offline_item = items[0];

  // Verify that the appropriate data is being set.
  EXPECT_EQ(offline_item.title, GetExpectedTitle(kSingleFileDownloadTitle));
  EXPECT_EQ(offline_item.filter, OfflineItemFilter::FILTER_OTHER);
  EXPECT_TRUE(offline_item.is_transient);
  EXPECT_FALSE(offline_item.is_suggested);
  EXPECT_FALSE(offline_item.is_off_the_record);

  // When downloadTotal isn't specified, we report progress by parts.
  EXPECT_EQ(offline_item.progress.value, 0);
  EXPECT_EQ(offline_item.progress.max.value(), 1);
  EXPECT_EQ(offline_item.progress.unit, OfflineItemProgressUnit::PERCENTAGE);

  // Change-detector tests for values we might want to provide or change.
  EXPECT_TRUE(offline_item.description.empty());
  EXPECT_TRUE(offline_item.page_url.is_empty());
  EXPECT_FALSE(offline_item.is_resumable);
}

IN_PROC_BROWSER_TEST_F(BackgroundFetchBrowserTest,
                       OfflineItemCollection_VerifyIconReceived) {
  // Starts a Background Fetch for a single to-be-downloaded file and waits for
  // the fetch to be registered with the offline items collection. We then
  // verify that the expected icon is associated with the newly added offline
  // item.

  std::vector<OfflineItem> items;
  ASSERT_NO_FATAL_FAILURE(
      RunScriptAndWaitForOfflineItems("StartSingleFileDownload()", &items));
  ASSERT_EQ(items.size(), 1u);

  const OfflineItem& offline_item = items[0];

  // Verify that the appropriate data is being set.
  EXPECT_EQ(offline_item.progress.value, 0);
  EXPECT_EQ(offline_item.progress.max.value(), 1);
  EXPECT_EQ(offline_item.progress.unit, OfflineItemProgressUnit::PERCENTAGE);

  // Get visuals associated with the newly added offline item.
  std::unique_ptr<OfflineItemVisuals> out_visuals;
  GetVisualsForOfflineItemSync(offline_item.id, &out_visuals);
#if defined(OS_ANDROID)
  EXPECT_FALSE(out_visuals->icon.IsEmpty());
  EXPECT_EQ(out_visuals->icon.Size().width(), 100);
  EXPECT_EQ(out_visuals->icon.Size().height(), 100);
#else
  EXPECT_TRUE(out_visuals->icon.IsEmpty());
#endif
}

IN_PROC_BROWSER_TEST_F(
    BackgroundFetchBrowserTest,
    OfflineItemCollection_VerifyResourceDownloadedWhenDownloadTotalLargerThanActualSize) {
  // Starts a Background Fetch for a single to-be-downloaded file and waits for
  // the fetch to be registered with the offline items collection.
  std::vector<OfflineItem> items;
  ASSERT_NO_FATAL_FAILURE(RunScriptAndWaitForOfflineItems(
      "StartSingleFileDownloadWithBiggerThanActualDownloadTotal()", &items));
  ASSERT_EQ(items.size(), 1u);

  OfflineItem offline_item = items[0];

  // Verify that the appropriate data is being set when we start downloading.
  EXPECT_EQ(offline_item.progress.value, 0);
  EXPECT_EQ(offline_item.progress.max.value(), kDownloadTotalBytesTooHigh);
  EXPECT_EQ(offline_item.progress.unit, OfflineItemProgressUnit::PERCENTAGE);

  // Wait for the download to be completed.
  {
    base::RunLoop run_loop;
    offline_content_provider_observer_->set_finished_processing_item_callback(
        base::BindOnce(&BackgroundFetchBrowserTest::DidFinishProcessingItem,
                       base::Unretained(this), run_loop.QuitClosure(),
                       &offline_item));
    run_loop.Run();
  }

  // Download total is too high; check that we're still reporting by size,
  // but have set the max value of the progress bar to the actual download size.
  EXPECT_EQ(offline_item.state,
            offline_items_collection::OfflineItemState::COMPLETE);
  EXPECT_EQ(offline_item.progress.max.value(), offline_item.progress.value);
  EXPECT_EQ(offline_item.progress.max.value(), kDownloadedResourceSizeInBytes);
}

IN_PROC_BROWSER_TEST_F(
    BackgroundFetchBrowserTest,
    OfflineItemCollection_VerifyResourceDownloadedWhenDownloadTotalSmallerThanActualSize) {
  // Starts a Background Fetch for a single to-be-downloaded file and waits for
  // the fetch to be registered with the offline items collection.
  std::vector<OfflineItem> items;
  ASSERT_NO_FATAL_FAILURE(RunScriptAndWaitForOfflineItems(
      "StartSingleFileDownloadWithSmallerThanActualDownloadTotal()", &items));
  ASSERT_EQ(items.size(), 1u);

  OfflineItem offline_item = items[0];

  // Verify that the appropriate data is being set when we start downloading.
  EXPECT_EQ(offline_item.progress.value, 0);
  EXPECT_EQ(offline_item.progress.max.value(), kDownloadTotalBytesTooLow);
  EXPECT_EQ(offline_item.progress.unit, OfflineItemProgressUnit::PERCENTAGE);

  // Wait for the offline_item to be processed.
  {
    base::RunLoop run_loop;
    offline_content_provider_observer_->set_finished_processing_item_callback(
        base::BindOnce(&BackgroundFetchBrowserTest::DidFinishProcessingItem,
                       base::Unretained(this), run_loop.QuitClosure(),
                       &offline_item));
    run_loop.Run();
  }

  // Download total is too low; check that we cancel the download when the
  // bytes downloaded exceed downloadTotal.
  EXPECT_EQ(offline_item.state,
            offline_items_collection::OfflineItemState::CANCELLED);
}

IN_PROC_BROWSER_TEST_F(
    BackgroundFetchBrowserTest,
    OfflineItemCollection_VerifyResourceDownloadedWhenCorrectDownloadTotalSpecified) {
  // Starts a Background Fetch for a single to-be-downloaded file and waits for
  // the fetch to be registered with the offline items collection.

  std::vector<OfflineItem> items;
  ASSERT_NO_FATAL_FAILURE(RunScriptAndWaitForOfflineItems(
      "StartSingleFileDownloadWithCorrectDownloadTotal()", &items));
  ASSERT_EQ(items.size(), 1u);

  const OfflineItem& offline_item = items[0];

  // Verify that the appropriate data is being set when downloadTotal is
  // correctly set.
  EXPECT_EQ(offline_item.progress.value, 0);
  EXPECT_EQ(offline_item.progress.max.value(), kDownloadedResourceSizeInBytes);
  EXPECT_EQ(offline_item.progress.unit, OfflineItemProgressUnit::PERCENTAGE);
}

}  // namespace
