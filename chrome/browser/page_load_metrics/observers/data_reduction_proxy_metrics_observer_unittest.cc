// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/data_reduction_proxy_metrics_observer.h"

#include <stdint.h>

#include <functional>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial.h"
#include "base/optional.h"
#include "base/process/kill.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/loader/chrome_navigation_data.h"
#include "chrome/browser/page_load_metrics/metrics_web_contents_observer.h"
#include "chrome/browser/page_load_metrics/observers/histogram_suffixes.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "chrome/browser/previews/previews_infobar_delegate.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "chrome/common/page_load_metrics/test/page_load_metrics_test_util.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/data_reduction_proxy/content/browser/data_reduction_proxy_pingback_client_impl.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_page_load_timing.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "content/public/test/web_contents_tester.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"

namespace data_reduction_proxy {

namespace {

const char kDefaultTestUrl[] = "http://google.com";
const int kMemoryKb = 1024;

data_reduction_proxy::DataReductionProxyData* DataForNavigationHandle(
    content::WebContents* web_contents,
    content::NavigationHandle* navigation_handle) {
  ChromeNavigationData* chrome_navigation_data = new ChromeNavigationData();
  content::WebContentsTester::For(web_contents)
      ->SetNavigationData(navigation_handle,
                          base::WrapUnique(chrome_navigation_data));
  data_reduction_proxy::DataReductionProxyData* data =
      new data_reduction_proxy::DataReductionProxyData();
  chrome_navigation_data->SetDataReductionProxyData(base::WrapUnique(data));

  return data;
}

// Pingback client responsible for recording the timing information it receives
// from a SendPingback call.
class TestPingbackClient
    : public data_reduction_proxy::DataReductionProxyPingbackClientImpl {
 public:
  TestPingbackClient()
      : data_reduction_proxy::DataReductionProxyPingbackClientImpl(
            nullptr,
            base::ThreadTaskRunnerHandle::Get()),
        send_pingback_called_(false) {}
  ~TestPingbackClient() override {}

  void SendPingback(
      const data_reduction_proxy::DataReductionProxyData& data,
      const data_reduction_proxy::DataReductionProxyPageLoadTiming& timing)
      override {
    timing_.reset(
        new data_reduction_proxy::DataReductionProxyPageLoadTiming(timing));
    send_pingback_called_ = true;
    data_ = data.DeepCopy();
  }

  data_reduction_proxy::DataReductionProxyPageLoadTiming* timing() const {
    return timing_.get();
  }

  const data_reduction_proxy::DataReductionProxyData& data() const {
    return *data_;
  }

  bool send_pingback_called() const { return send_pingback_called_; }

  void Reset() {
    send_pingback_called_ = false;
    timing_.reset();
  }

 private:
  std::unique_ptr<data_reduction_proxy::DataReductionProxyPageLoadTiming>
      timing_;
  std::unique_ptr<data_reduction_proxy::DataReductionProxyData> data_;
  bool send_pingback_called_;

  DISALLOW_COPY_AND_ASSIGN(TestPingbackClient);
};

}  // namespace

// DataReductionProxyMetricsObserver responsible for modifying data about the
// navigation in OnCommit. It is also responsible for using a passed in
// DataReductionProxyPingbackClient instead of the default.
class TestDataReductionProxyMetricsObserver
    : public DataReductionProxyMetricsObserver {
 public:
  TestDataReductionProxyMetricsObserver(content::WebContents* web_contents,
                                        TestPingbackClient* pingback_client,
                                        bool data_reduction_proxy_used,
                                        bool lite_page_used)
      : web_contents_(web_contents),
        pingback_client_(pingback_client),
        data_reduction_proxy_used_(data_reduction_proxy_used),
        lite_page_used_(lite_page_used) {}

  ~TestDataReductionProxyMetricsObserver() override {}

  // page_load_metrics::PageLoadMetricsObserver implementation:
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override {
    DataReductionProxyData* data =
        DataForNavigationHandle(web_contents_, navigation_handle);
    data->set_used_data_reduction_proxy(data_reduction_proxy_used_);
    data->set_request_url(GURL(kDefaultTestUrl));
    data->set_lite_page_received(lite_page_used_);
    return DataReductionProxyMetricsObserver::OnCommit(navigation_handle,
                                                       source_id);
  }

  DataReductionProxyPingbackClient* GetPingbackClient() const override {
    return pingback_client_;
  }

  void RequestProcessDump(
      base::ProcessId pid,
      memory_instrumentation::MemoryInstrumentation::RequestGlobalDumpCallback
          callback) override {
    memory_instrumentation::mojom::GlobalMemoryDumpPtr global_dump(
        memory_instrumentation::mojom::GlobalMemoryDump::New());

    memory_instrumentation::mojom::ProcessMemoryDumpPtr pmd(
        memory_instrumentation::mojom::ProcessMemoryDump::New());
    pmd->pid = pid;
    pmd->process_type = memory_instrumentation::mojom::ProcessType::RENDERER;
    pmd->os_dump = memory_instrumentation::mojom::OSMemDump::New();
    pmd->os_dump->private_footprint_kb = kMemoryKb;

    global_dump->process_dumps.push_back(std::move(pmd));
    callback.Run(true, memory_instrumentation::GlobalMemoryDump::MoveFrom(
                           std::move(global_dump)));
  }

 private:
  content::WebContents* web_contents_;
  TestPingbackClient* pingback_client_;
  bool data_reduction_proxy_used_;
  bool lite_page_used_;

  DISALLOW_COPY_AND_ASSIGN(TestDataReductionProxyMetricsObserver);
};

class DataReductionProxyMetricsObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  DataReductionProxyMetricsObserverTest()
      : pingback_client_(new TestPingbackClient()),
        data_reduction_proxy_used_(false),
        is_using_lite_page_(false),
        opt_out_expected_(false) {}

  void ResetTest() {
    page_load_metrics::InitPageLoadTimingForTest(&timing_);
    // Reset to the default testing state. Does not reset histogram state.
    timing_.navigation_start = base::Time::FromDoubleT(1);
    timing_.response_start = base::TimeDelta::FromSeconds(2);
    timing_.parse_timing->parse_start = base::TimeDelta::FromSeconds(3);
    timing_.paint_timing->first_contentful_paint =
        base::TimeDelta::FromSeconds(4);
    timing_.paint_timing->first_paint = base::TimeDelta::FromSeconds(4);
    timing_.paint_timing->first_meaningful_paint =
        base::TimeDelta::FromSeconds(8);
    timing_.paint_timing->first_image_paint = base::TimeDelta::FromSeconds(5);
    timing_.paint_timing->first_text_paint = base::TimeDelta::FromSeconds(6);
    timing_.document_timing->load_event_start = base::TimeDelta::FromSeconds(7);
    timing_.parse_timing->parse_stop = base::TimeDelta::FromSeconds(4);
    timing_.parse_timing->parse_blocked_on_script_load_duration =
        base::TimeDelta::FromSeconds(1);
    PopulateRequiredTimingFields(&timing_);
  }

  void RunTest(bool data_reduction_proxy_used,
               bool is_using_lite_page,
               bool opt_out_expected) {
    data_reduction_proxy_used_ = data_reduction_proxy_used;
    is_using_lite_page_ = is_using_lite_page;
    opt_out_expected_ = opt_out_expected;
    NavigateAndCommit(GURL(kDefaultTestUrl));
    SimulateTimingUpdate(timing_);
    pingback_client_->Reset();
  }

  void RunTestAndNavigateToUntrackedUrl(bool data_reduction_proxy_used,
                                        bool is_using_lite_page,
                                        bool opt_out_expected) {
    RunTest(data_reduction_proxy_used, is_using_lite_page, opt_out_expected);
    NavigateToUntrackedUrl();
  }

  void SimulateRendererCrash() {
    observer()->RenderProcessGone(
        base::TerminationStatus::TERMINATION_STATUS_ABNORMAL_TERMINATION);
  }

  // Verify that, if expected and actual are set, their values are equal.
  // Otherwise, verify that both are unset.
  void ExpectEqualOrUnset(const base::Optional<base::TimeDelta>& expected,
                          const base::Optional<base::TimeDelta>& actual) {
    if (expected && actual) {
      EXPECT_EQ(expected.value(), actual.value());
    } else {
      EXPECT_TRUE(!expected);
      EXPECT_TRUE(!actual);
    }
  }

  void ValidateTimes() {
    EXPECT_TRUE(pingback_client_->send_pingback_called());
    EXPECT_EQ(timing_.navigation_start,
              pingback_client_->timing()->navigation_start);
    ExpectEqualOrUnset(timing_.paint_timing->first_contentful_paint,
                       pingback_client_->timing()->first_contentful_paint);
    ExpectEqualOrUnset(
        timing_.paint_timing->first_meaningful_paint,
        pingback_client_->timing()->experimental_first_meaningful_paint);
    ExpectEqualOrUnset(timing_.response_start,
              pingback_client_->timing()->response_start);
    ExpectEqualOrUnset(timing_.document_timing->load_event_start,
                       pingback_client_->timing()->load_event_start);
    ExpectEqualOrUnset(timing_.paint_timing->first_image_paint,
                       pingback_client_->timing()->first_image_paint);
    EXPECT_EQ(opt_out_expected_, pingback_client_->timing()->opt_out_occurred);
    EXPECT_EQ(timing_.document_timing->load_event_start
                  ? static_cast<int64_t>(kMemoryKb)
                  : 0,
              pingback_client_->timing()->renderer_memory_usage_kb);
  }

  void ValidateLoFiInPingback(bool lofi_expected) {
    EXPECT_TRUE(pingback_client_->send_pingback_called());
    EXPECT_EQ(lofi_expected, pingback_client_->data().lofi_received());
  }

  void ValidateRendererCrash(bool renderer_crashed) {
    EXPECT_TRUE(pingback_client_->send_pingback_called());
    EXPECT_EQ(renderer_crashed,
              pingback_client_->timing()->host_id !=
                  content::ChildProcessHost::kInvalidUniqueID);
  }

  void ValidateHistograms() {
    ValidateHistogramsForSuffix(
        ::internal::kHistogramDOMContentLoadedEventFiredSuffix,
        timing_.document_timing->dom_content_loaded_event_start);
    ValidateHistogramsForSuffix(::internal::kHistogramFirstLayoutSuffix,
                                timing_.document_timing->first_layout);
    ValidateHistogramsForSuffix(::internal::kHistogramLoadEventFiredSuffix,
                                timing_.document_timing->load_event_start);
    ValidateHistogramsForSuffix(
        ::internal::kHistogramFirstContentfulPaintSuffix,
        timing_.paint_timing->first_contentful_paint);
    ValidateHistogramsForSuffix(
        ::internal::kHistogramFirstMeaningfulPaintSuffix,
        timing_.paint_timing->first_meaningful_paint);
    ValidateHistogramsForSuffix(::internal::kHistogramFirstImagePaintSuffix,
                                timing_.paint_timing->first_image_paint);
    ValidateHistogramsForSuffix(::internal::kHistogramFirstPaintSuffix,
                                timing_.paint_timing->first_paint);
    ValidateHistogramsForSuffix(::internal::kHistogramFirstTextPaintSuffix,
                                timing_.paint_timing->first_text_paint);
    ValidateHistogramsForSuffix(::internal::kHistogramParseStartSuffix,
                                timing_.parse_timing->parse_start);
    ValidateHistogramsForSuffix(
        ::internal::kHistogramParseBlockedOnScriptLoadSuffix,
        timing_.parse_timing->parse_blocked_on_script_load_duration);
    ValidateHistogramsForSuffix(::internal::kHistogramParseDurationSuffix,
                                timing_.parse_timing->parse_stop.value() -
                                    timing_.parse_timing->parse_start.value());
  }

  void ValidateHistogramsForSuffix(
      const std::string& histogram_suffix,
      const base::Optional<base::TimeDelta>& event) {
    histogram_tester().ExpectTotalCount(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(histogram_suffix),
        data_reduction_proxy_used_ ? 1 : 0);
    histogram_tester().ExpectTotalCount(
        std::string(internal::kHistogramDataReductionProxyLitePagePrefix)
            .append(histogram_suffix),
        is_using_lite_page_ ? 1 : 0);
    if (!data_reduction_proxy_used_)
      return;
    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(histogram_suffix),
        static_cast<base::HistogramBase::Sample>(
            event.value().InMilliseconds()),
        1);
    if (!is_using_lite_page_)
      return;
    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyLitePagePrefix)
            .append(histogram_suffix),
        event.value().InMilliseconds(), is_using_lite_page_ ? 1 : 0);
  }

  void ValidateDataHistograms(int network_resources,
                              int drp_resources,
                              int64_t network_bytes,
                              int64_t drp_bytes,
                              int64_t ocl_bytes) {
    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kResourcesPercentProxied),
        100 * drp_resources / network_resources, 1);

    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kBytesPercentProxied),
        static_cast<int>(100 * drp_bytes / network_bytes), 1);

    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kNetworkResources),
        network_resources, 1);

    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kResourcesProxied),
        drp_resources, 1);

    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kResourcesNotProxied),
        network_resources - drp_resources, 1);

    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kNetworkBytes),
        static_cast<int>(network_bytes / 1024), 1);

    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kBytesProxied),
        static_cast<int>(drp_bytes / 1024), 1);

    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kBytesNotProxied),
        static_cast<int>((network_bytes - drp_bytes) / 1024), 1);

    histogram_tester().ExpectUniqueSample(
        std::string(internal::kHistogramDataReductionProxyPrefix)
            .append(internal::kBytesOriginal),
        static_cast<int>(ocl_bytes / 1024), 1);
    if (ocl_bytes < network_bytes) {
      histogram_tester().ExpectUniqueSample(
          std::string(internal::kHistogramDataReductionProxyPrefix)
              .append(internal::kBytesInflationPercent),
          static_cast<int>(100 * network_bytes / ocl_bytes - 100), 1);

      histogram_tester().ExpectUniqueSample(
          std::string(internal::kHistogramDataReductionProxyPrefix)
              .append(internal::kBytesInflation),
          static_cast<int>((network_bytes - ocl_bytes) / 1024), 1);
    } else {
      histogram_tester().ExpectUniqueSample(
          std::string(internal::kHistogramDataReductionProxyPrefix)
              .append(internal::kBytesCompressionRatio),
          static_cast<int>(100 * network_bytes / ocl_bytes), 1);

      histogram_tester().ExpectUniqueSample(
          std::string(internal::kHistogramDataReductionProxyPrefix)
              .append(internal::kBytesSavings),
          static_cast<int>((ocl_bytes - network_bytes) / 1024), 1);
    }
  }

 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        std::make_unique<TestDataReductionProxyMetricsObserver>(
            web_contents(), pingback_client_.get(), data_reduction_proxy_used_,
            is_using_lite_page_));
  }

  std::unique_ptr<TestPingbackClient> pingback_client_;
  page_load_metrics::mojom::PageLoadTiming timing_;

 private:
  bool data_reduction_proxy_used_;
  bool is_using_lite_page_;
  bool opt_out_expected_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyMetricsObserverTest);
};

TEST_F(DataReductionProxyMetricsObserverTest, DataReductionProxyOff) {
  ResetTest();
  // Verify that when the data reduction proxy was not used, no UMA is reported.
  RunTest(false, false, false);
  ValidateHistograms();
}

TEST_F(DataReductionProxyMetricsObserverTest, DataReductionProxyOn) {
  ResetTest();
  // Verify that when the data reduction proxy was used, but lite page was not
  // used, the correpsonding UMA is reported.
  RunTest(true, false, false);
  ValidateHistograms();
}

TEST_F(DataReductionProxyMetricsObserverTest, LitePageEnabled) {
  ResetTest();
  // Verify that when the data reduction proxy was used and lite page was used,
  // both histograms are reported.
  RunTest(true, true, false);
  ValidateHistograms();
}

TEST_F(DataReductionProxyMetricsObserverTest, OnCompletePingback) {
  ResetTest();
  // Verify that when data reduction proxy was used the correct timing
  // information is sent to SendPingback.
  RunTestAndNavigateToUntrackedUrl(true, false, false);
  ValidateTimes();

  ResetTest();
  // Verify that when data reduction proxy was used but first image paint is
  // unset, the correct timing information is sent to SendPingback.
  timing_.paint_timing->first_image_paint = base::nullopt;
  RunTestAndNavigateToUntrackedUrl(true, false, false);
  ValidateTimes();

  ResetTest();
  // Verify that when data reduction proxy was used but first contentful paint
  // is unset, SendPingback is not called.
  timing_.paint_timing->first_contentful_paint = base::nullopt;
  RunTestAndNavigateToUntrackedUrl(true, false, false);
  ValidateTimes();

  ResetTest();
  // Verify that when data reduction proxy was used but first meaningful paint
  // is unset, SendPingback is not called.
  timing_.paint_timing->first_meaningful_paint = base::nullopt;
  RunTestAndNavigateToUntrackedUrl(true, false, false);
  ValidateTimes();

  ResetTest();
  // Verify that when data reduction proxy was used but load event start is
  // unset, SendPingback is not called.
  timing_.document_timing->load_event_start = base::nullopt;
  RunTestAndNavigateToUntrackedUrl(true, false, false);
  ValidateTimes();
  ValidateLoFiInPingback(false);

  ResetTest();
  // Verify that when an opt out occurs, that it is reported in the pingback.
  timing_.document_timing->load_event_start = base::nullopt;
  RunTest(true, true, true);
  observer()->BroadcastEventToObservers(
      PreviewsInfoBarDelegate::OptOutEventKey());
  NavigateToUntrackedUrl();
  ValidateTimes();
  ValidateLoFiInPingback(false);

  ResetTest();
  std::unique_ptr<DataReductionProxyData> data =
      std::make_unique<DataReductionProxyData>();
  data->set_used_data_reduction_proxy(true);
  data->set_request_url(GURL(kDefaultTestUrl));
  data->set_lofi_received(true);

  // Verify LoFi is tracked when a LoFi response is received.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kResourceUrl),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      true /*was_cached*/,
      1024 * 40 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      std::move(data),
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  RunTest(true, false, false);
  SimulateLoadedResource(resource);
  NavigateToUntrackedUrl();
  ValidateTimes();
  ValidateLoFiInPingback(true);

  ResetTest();
  // Verify that when data reduction proxy was not used, SendPingback is not
  // called.
  RunTestAndNavigateToUntrackedUrl(false, false, false);
  EXPECT_FALSE(pingback_client_->send_pingback_called());

  ResetTest();
  // Verify that when the holdback experiment is enabled, a pingback is sent.
  base::FieldTrialList field_trial_list(nullptr);
  ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
      "DataCompressionProxyHoldback", "Enabled"));
  RunTestAndNavigateToUntrackedUrl(true, false, false);
  EXPECT_TRUE(pingback_client_->send_pingback_called());
}

TEST_F(DataReductionProxyMetricsObserverTest, ByteInformationCompression) {
  ResetTest();

  RunTest(true, false, false);

  std::unique_ptr<DataReductionProxyData> data =
      std::make_unique<DataReductionProxyData>();
  data->set_used_data_reduction_proxy(true);
  data->set_request_url(GURL(kDefaultTestUrl));

  // Prepare 4 resources of varying size and configurations.
  page_load_metrics::ExtraRequestCompleteInfo resources[] = {
      // Cached request.
      {GURL(kResourceUrl),
       net::HostPortPair(),
       -1 /* frame_tree_node_id */,
       true /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       0 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_SCRIPT,
       0,
       {} /* load_timing_info */},
      // Uncached non-proxied request.
      {GURL(kResourceUrl),
       net::HostPortPair(),
       -1 /* frame_tree_node_id */,
       false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_SCRIPT,
       0,
       {} /* load_timing_info */},
      // Uncached proxied request with .1 compression ratio.
      {GURL(kResourceUrl),
       net::HostPortPair(),
       -1 /* frame_tree_node_id */,
       false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 10 /* original_network_content_length */,
       data->DeepCopy(),
       content::ResourceType::RESOURCE_TYPE_SCRIPT,
       0,
       {} /* load_timing_info */},
      // Uncached proxied request with .5 compression ratio.
      {GURL(kResourceUrl),
       net::HostPortPair(),
       -1 /* frame_tree_node_id */,
       false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 * 5 /* original_network_content_length */,
       std::move(data),
       content::ResourceType::RESOURCE_TYPE_SCRIPT,
       0,
       {} /* load_timing_info */},
  };

  int network_resources = 0;
  int drp_resources = 0;
  int64_t insecure_network_bytes = 0;
  int64_t secure_network_bytes = 0;
  int64_t drp_bytes = 0;
  int64_t insecure_ocl_bytes = 0;
  int64_t secure_ocl_bytes = 0;
  for (const auto& request : resources) {
    SimulateLoadedResource(request);
    if (!request.was_cached) {
      if (request.url.SchemeIsCryptographic()) {
        secure_network_bytes += request.raw_body_bytes;
        secure_ocl_bytes += request.original_network_content_length;
      } else {
        insecure_network_bytes += request.raw_body_bytes;
        insecure_ocl_bytes += request.original_network_content_length;
      }
      ++network_resources;
    }
    if (request.data_reduction_proxy_data &&
        request.data_reduction_proxy_data->used_data_reduction_proxy()) {
      drp_bytes += request.raw_body_bytes;
      ++drp_resources;
    }
  }

  NavigateToUntrackedUrl();

  ValidateDataHistograms(network_resources, drp_resources,
                         insecure_network_bytes + secure_network_bytes,
                         drp_bytes, insecure_ocl_bytes + secure_ocl_bytes);
}

TEST_F(DataReductionProxyMetricsObserverTest, ByteInformationInflation) {
  ResetTest();

  RunTest(true, false, false);

  std::unique_ptr<DataReductionProxyData> data =
      std::make_unique<DataReductionProxyData>();
  data->set_used_data_reduction_proxy(true);
  data->set_request_url(GURL(kDefaultTestUrl));

  // Prepare 4 resources of varying size and configurations.
  page_load_metrics::ExtraRequestCompleteInfo resources[] = {
      // Cached request.
      {GURL(kResourceUrl),
       net::HostPortPair(),
       -1 /* frame_tree_node_id */,
       true /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       0 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_SCRIPT,
       0,
       {} /* load_timing_info */},
      // Uncached non-proxied request.
      {GURL(kResourceUrl),
       net::HostPortPair(),
       -1 /* frame_tree_node_id */,
       false /*was_cached*/,
       1024 * 40 /* raw_body_bytes */,
       1024 * 40 /* original_network_content_length */,
       nullptr /* data_reduction_proxy_data */,
       content::ResourceType::RESOURCE_TYPE_SCRIPT,
       0,
       {} /* load_timing_info */},
      // Uncached proxied request with .1 compression ratio.
      {GURL(kResourceUrl),
       net::HostPortPair(),
       -1 /* frame_tree_node_id */,
       false /*was_cached*/,
       1024 * 40 * 10 /* raw_body_bytes */,
       1024 * 40 /* original_network_content_length */,
       data->DeepCopy(),
       content::ResourceType::RESOURCE_TYPE_SCRIPT,
       0,
       {} /* load_timing_info */},
      // Uncached proxied request with .5 compression ratio.
      {GURL(kResourceUrl),
       net::HostPortPair(),
       -1 /* frame_tree_node_id */,
       false /*was_cached*/,
       1024 * 40 * 5 /* raw_body_bytes */,
       1024 * 40 /* original_network_content_length */,
       std::move(data),
       content::ResourceType::RESOURCE_TYPE_SCRIPT,
       0,
       {} /* load_timing_info */},
  };

  int network_resources = 0;
  int drp_resources = 0;
  int64_t insecure_network_bytes = 0;
  int64_t secure_network_bytes = 0;
  int64_t drp_bytes = 0;
  int64_t secure_drp_bytes = 0;
  int64_t insecure_ocl_bytes = 0;
  int64_t secure_ocl_bytes = 0;
  for (const auto& request : resources) {
    SimulateLoadedResource(request);
    const bool is_secure = request.url.SchemeIsCryptographic();
    if (!request.was_cached) {
      if (is_secure) {
        secure_network_bytes += request.raw_body_bytes;
        secure_ocl_bytes += request.original_network_content_length;
      } else {
        insecure_network_bytes += request.raw_body_bytes;
        insecure_ocl_bytes += request.original_network_content_length;
      }
      ++network_resources;
    }
    if (request.data_reduction_proxy_data &&
        request.data_reduction_proxy_data->used_data_reduction_proxy()) {
      if (is_secure)
        secure_drp_bytes += request.raw_body_bytes;
      else
        drp_bytes += request.raw_body_bytes;
      ++drp_resources;
    }
  }

  NavigateToUntrackedUrl();

  ValidateDataHistograms(network_resources, drp_resources,
                         insecure_network_bytes + secure_network_bytes,
                         drp_bytes + secure_drp_bytes,
                         insecure_ocl_bytes + secure_ocl_bytes);
}

TEST_F(DataReductionProxyMetricsObserverTest, ProcessIdSentOnRendererCrash) {
  ResetTest();
  RunTest(true, false, false);
  std::unique_ptr<DataReductionProxyData> data =
      std::make_unique<DataReductionProxyData>();
  data->set_used_data_reduction_proxy(true);
  data->set_request_url(GURL(kDefaultTestUrl));
  SimulateRendererCrash();

  // When the renderer crashes, the pingback should report that.
  ValidateRendererCrash(true);

  ResetTest();
  RunTest(true, false, false);
  data = std::make_unique<DataReductionProxyData>();
  data->set_used_data_reduction_proxy(true);
  data->set_request_url(GURL(kDefaultTestUrl));
  NavigateToUntrackedUrl();

  // When the renderer does not crash, the pingback should report that.
  ValidateRendererCrash(false);
}

}  //  namespace data_reduction_proxy
