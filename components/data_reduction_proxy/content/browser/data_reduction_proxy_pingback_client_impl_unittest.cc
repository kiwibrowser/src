// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/content/browser/data_reduction_proxy_pingback_client_impl.h"

#include <stdint.h>

#include <list>
#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/sys_info.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_util.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_page_load_timing.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_switches.h"
#include "components/data_reduction_proxy/proto/client_config.pb.h"
#include "components/data_reduction_proxy/proto/pageload_metrics.pb.h"
#include "content/public/common/child_process_host.h"
#include "net/base/net_errors.h"
#include "net/base/network_change_notifier.h"
#include "net/nqe/effective_connection_type.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace data_reduction_proxy {

namespace {

static const char kHistogramSucceeded[] =
    "DataReductionProxy.Pingback.Succeeded";
static const char kHistogramAttempted[] =
    "DataReductionProxy.Pingback.Attempted";
static const char kSessionKey[] = "fake-session";
static const char kFakeURL[] = "http://www.google.com/";
static const int64_t kBytes = 10000;
static const int64_t kBytesOriginal = 1000000;
static const int64_t kTotalPageSizeBytes = 20000;
static const float kCachedFraction = 0.5;
static const int kCrashProcessId = 1;
static const int64_t kRendererMemory = 1024;

}  // namespace

// Controls whether a pingback is sent or not.
class TestDataReductionProxyPingbackClientImpl
    : public DataReductionProxyPingbackClientImpl {
 public:
  TestDataReductionProxyPingbackClientImpl(
      net::URLRequestContextGetter* url_request_context_getter,
      scoped_refptr<base::SingleThreadTaskRunner> thread_task_runner)
      : DataReductionProxyPingbackClientImpl(url_request_context_getter,
                                             std::move(thread_task_runner)),
        should_override_random_(false),
        override_value_(0.0f),
        current_time_(base::Time::Now()) {}

  // Overrides the bahvior of the random float generator in
  // DataReductionProxyPingbackClientImpl.
  // If |should_override_random| is true, the typically random value that is
  // compared with reporting fraction will deterministically be
  // |override_value|.
  void OverrideRandom(bool should_override_random, float override_value) {
    should_override_random_ = should_override_random;
    override_value_ = override_value;
  }

  // Sets the time used for the metrics reporting time.
  void set_current_time(base::Time current_time) {
    current_time_ = current_time;
  }

 private:
  float GenerateRandomFloat() const override {
    if (should_override_random_)
      return override_value_;
    return DataReductionProxyPingbackClientImpl::GenerateRandomFloat();
  }

  base::Time CurrentTime() const override { return current_time_; }

  bool should_override_random_;
  float override_value_;
  base::Time current_time_;
};

class DataReductionProxyPingbackClientImplTest : public testing::Test {
 public:
  DataReductionProxyPingbackClientImplTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME,
            base::test::ScopedTaskEnvironment::ExecutionMode::ASYNC) {}

  TestDataReductionProxyPingbackClientImpl* pingback_client() const {
    return pingback_client_.get();
  }

  void Init() {
    request_context_getter_ = new net::TestURLRequestContextGetter(
        scoped_task_environment_.GetMainThreadTaskRunner());
    pingback_client_ =
        std::make_unique<TestDataReductionProxyPingbackClientImpl>(
            request_context_getter_.get(),
            scoped_task_environment_.GetMainThreadTaskRunner());
    page_id_ = 0u;
  }

  void CreateAndSendPingback(bool lofi_received,
                             bool client_lofi_requested,
                             bool lite_page_received,
                             bool app_background_occurred,
                             bool opt_out_occurred,
                             bool crash) {
    timing_ = std::make_unique<DataReductionProxyPageLoadTiming>(
        base::Time::FromJsTime(1500) /* navigation_start */,
        base::Optional<base::TimeDelta>(
            base::TimeDelta::FromMilliseconds(1600)) /* response_start */,
        base::Optional<base::TimeDelta>(
            base::TimeDelta::FromMilliseconds(1700)) /* load_event_start */,
        base::Optional<base::TimeDelta>(
            base::TimeDelta::FromMilliseconds(1800)) /* first_image_paint */,
        base::Optional<base::TimeDelta>(base::TimeDelta::FromMilliseconds(
            1900)) /* first_contentful_paint */,
        base::Optional<base::TimeDelta>(base::TimeDelta::FromMilliseconds(
            2000)) /* experimental_first_meaningful_paint */,
        base::Optional<base::TimeDelta>(
            base::TimeDelta::FromMilliseconds(3000)) /* first_input_delay */,
        base::Optional<base::TimeDelta>(base::TimeDelta::FromMilliseconds(
            100)) /* parse_blocked_on_script_load_duration */,
        base::Optional<base::TimeDelta>(
            base::TimeDelta::FromMilliseconds(2000)) /* parse_stop */,
        kBytes /* network_bytes */, kBytesOriginal /* original_network_bytes */,
        kTotalPageSizeBytes /* total_page_size_bytes */,
        kCachedFraction /* cached_fraction */, app_background_occurred,
        opt_out_occurred, kRendererMemory,
        crash ? kCrashProcessId : content::ChildProcessHost::kInvalidUniqueID);

    DataReductionProxyData request_data;
    request_data.set_session_key(kSessionKey);
    request_data.set_request_url(GURL(kFakeURL));
    request_data.set_effective_connection_type(
        net::EFFECTIVE_CONNECTION_TYPE_OFFLINE);
    request_data.set_connection_type(
        net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
    request_data.set_lofi_received(lofi_received);
    request_data.set_client_lofi_requested(client_lofi_requested);
    request_data.set_lite_page_received(lite_page_received);
    request_data.set_page_id(page_id_);
    factory()->set_remove_fetcher_on_delete(true);
    static_cast<DataReductionProxyPingbackClient*>(pingback_client())
        ->SendPingback(request_data, *timing_);
    page_id_++;
  }

  // Send a fake crash report from breakpad.
  void ReportCrash(bool oom) {
#if defined(OS_ANDROID)
    breakpad::CrashDumpManager::CrashDumpDetails details = {
        kCrashProcessId, content::PROCESS_TYPE_RENDERER, oom,
        base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES};
    details.file_size = oom ? 0 : 1;
    static_cast<breakpad::CrashDumpManager::Observer*>(pingback_client_.get())
        ->OnCrashDumpProcessed(details);
#endif
  }

  net::TestURLFetcherFactory* factory() { return &factory_; }

  const DataReductionProxyPageLoadTiming& timing() { return *timing_; }

  const base::HistogramTester& histogram_tester() { return histogram_tester_; }

  uint64_t page_id() const { return page_id_; }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  std::unique_ptr<TestDataReductionProxyPingbackClientImpl> pingback_client_;
  net::TestURLFetcherFactory factory_;
  std::unique_ptr<DataReductionProxyPageLoadTiming> timing_;
  base::HistogramTester histogram_tester_;
  uint64_t page_id_;
};

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyPingbackContent) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  base::Time current_time = base::Time::UnixEpoch();
  pingback_client()->set_current_time(current_time);
  uint64_t data_page_id = page_id();
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  EXPECT_EQ(current_time, protobuf_parser::TimestampToTime(
                              batched_request.metrics_sent_time()));
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(
      timing().navigation_start,
      protobuf_parser::TimestampToTime(pageload_metrics.first_request_time()));
  EXPECT_EQ(timing().response_start.value(),
            protobuf_parser::DurationToTimeDelta(
                pageload_metrics.time_to_first_byte()));
  EXPECT_EQ(
      timing().load_event_start.value(),
      protobuf_parser::DurationToTimeDelta(pageload_metrics.page_load_time()));
  EXPECT_EQ(timing().first_image_paint.value(),
            protobuf_parser::DurationToTimeDelta(
                pageload_metrics.time_to_first_image_paint()));
  EXPECT_EQ(timing().first_contentful_paint.value(),
            protobuf_parser::DurationToTimeDelta(
                pageload_metrics.time_to_first_contentful_paint()));
  EXPECT_EQ(
      timing().experimental_first_meaningful_paint.value(),
      protobuf_parser::DurationToTimeDelta(
          pageload_metrics.experimental_time_to_first_meaningful_paint()));
  EXPECT_EQ(timing().first_input_delay.value(),
            protobuf_parser::DurationToTimeDelta(
                pageload_metrics.first_input_delay()));
  EXPECT_EQ(timing().parse_blocked_on_script_load_duration.value(),
            protobuf_parser::DurationToTimeDelta(
                pageload_metrics.parse_blocked_on_script_load_duration()));
  EXPECT_EQ(timing().parse_stop.value(), protobuf_parser::DurationToTimeDelta(
                                             pageload_metrics.parse_stop()));

  EXPECT_EQ(kSessionKey, pageload_metrics.session_key());
  EXPECT_EQ(kFakeURL, pageload_metrics.first_request_url());
  EXPECT_EQ(kBytes, pageload_metrics.compressed_page_size_bytes());
  EXPECT_EQ(kBytesOriginal, pageload_metrics.original_page_size_bytes());
  EXPECT_EQ(kTotalPageSizeBytes, pageload_metrics.total_page_size_bytes());
  EXPECT_EQ(kCachedFraction, pageload_metrics.cached_fraction());
  EXPECT_EQ(data_page_id, pageload_metrics.page_id());

  EXPECT_EQ(PageloadMetrics_PreviewsType_NONE,
            pageload_metrics.previews_type());
  EXPECT_EQ(PageloadMetrics_PreviewsOptOut_UNKNOWN,
            pageload_metrics.previews_opt_out());

  EXPECT_EQ(
      PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_OFFLINE,
      pageload_metrics.effective_connection_type());
  EXPECT_EQ(PageloadMetrics_ConnectionType_CONNECTION_UNKNOWN,
            pageload_metrics.connection_type());
  EXPECT_EQ(kRendererMemory, pageload_metrics.renderer_memory_usage_kb());
  EXPECT_EQ(std::string(), pageload_metrics.holdback_group());
  EXPECT_EQ(PageloadMetrics_RendererCrashType_NO_CRASH,
            pageload_metrics.renderer_crash_type());
  EXPECT_EQ(base::SysInfo::AmountOfPhysicalMemory() / 1024,
            batched_request.device_info().total_device_memory_kb());

  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 1);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyHoldback) {
  base::FieldTrialList field_trial_list(nullptr);
  ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial(
      "DataCompressionProxyHoldback", "Enabled"));
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ("Enabled", pageload_metrics.holdback_group());
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 1);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest,
       VerifyTwoPingbacksBatchedContent) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  base::Time current_time = base::Time::UnixEpoch();
  pingback_client()->set_current_time(current_time);
  // First pingback
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);

  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  // Two more pingbacks batched together.
  std::list<uint64_t> page_ids;
  page_ids.push_back(page_id());
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 2);
  page_ids.push_back(page_id());
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 3);

  // Ignore the first pingback.
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 1);

  // Check the state of the second pingback.
  test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 2);
  EXPECT_EQ(current_time, protobuf_parser::TimestampToTime(
                              batched_request.metrics_sent_time()));
  EXPECT_EQ(base::SysInfo::AmountOfPhysicalMemory() / 1024,
            batched_request.device_info().total_device_memory_kb());

  // Verify the content of both pingbacks.
  for (size_t i = 0; i < 2; ++i) {
    PageloadMetrics pageload_metrics = batched_request.pageloads(i);
    EXPECT_EQ(timing().navigation_start,
              protobuf_parser::TimestampToTime(
                  pageload_metrics.first_request_time()));
    EXPECT_EQ(timing().response_start.value(),
              protobuf_parser::DurationToTimeDelta(
                  pageload_metrics.time_to_first_byte()));
    EXPECT_EQ(timing().load_event_start.value(),
              protobuf_parser::DurationToTimeDelta(
                  pageload_metrics.page_load_time()));
    EXPECT_EQ(timing().first_image_paint.value(),
              protobuf_parser::DurationToTimeDelta(
                  pageload_metrics.time_to_first_image_paint()));
    EXPECT_EQ(timing().first_contentful_paint.value(),
              protobuf_parser::DurationToTimeDelta(
                  pageload_metrics.time_to_first_contentful_paint()));
    EXPECT_EQ(
        timing().experimental_first_meaningful_paint.value(),
        protobuf_parser::DurationToTimeDelta(
            pageload_metrics.experimental_time_to_first_meaningful_paint()));
    EXPECT_EQ(timing().first_input_delay.value(),
              protobuf_parser::DurationToTimeDelta(
                  pageload_metrics.first_input_delay()));
    EXPECT_EQ(timing().parse_blocked_on_script_load_duration.value(),
              protobuf_parser::DurationToTimeDelta(
                  pageload_metrics.parse_blocked_on_script_load_duration()));
    EXPECT_EQ(timing().parse_stop.value(), protobuf_parser::DurationToTimeDelta(
                                               pageload_metrics.parse_stop()));

    EXPECT_EQ(kSessionKey, pageload_metrics.session_key());
    EXPECT_EQ(kFakeURL, pageload_metrics.first_request_url());
    EXPECT_EQ(kBytes, pageload_metrics.compressed_page_size_bytes());
    EXPECT_EQ(kBytesOriginal, pageload_metrics.original_page_size_bytes());
    EXPECT_EQ(kTotalPageSizeBytes, pageload_metrics.total_page_size_bytes());
    EXPECT_EQ(kCachedFraction, pageload_metrics.cached_fraction());
    EXPECT_EQ(page_ids.front(), pageload_metrics.page_id());
    page_ids.pop_front();
    EXPECT_EQ(
        PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_OFFLINE,
        pageload_metrics.effective_connection_type());
    EXPECT_EQ(PageloadMetrics_ConnectionType_CONNECTION_UNKNOWN,
              pageload_metrics.connection_type());
    EXPECT_EQ(kRendererMemory, pageload_metrics.renderer_memory_usage_kb());
  }

  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 3);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, SendTwoPingbacks) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 2);

  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 1);
  EXPECT_TRUE(factory()->GetFetcherByID(0));
  test_fetcher = factory()->GetFetcherByID(0);
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 2);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  histogram_tester().ExpectTotalCount(kHistogramAttempted, 2);
}

TEST_F(DataReductionProxyPingbackClientImplTest, NoPingbackSent) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(0.0f);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, false, 1);
  histogram_tester().ExpectTotalCount(kHistogramSucceeded, 0);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyReportingBehvaior) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));

  // Verify that if the random number is less than the reporting fraction, the
  // pingback is created.
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(0.5f);
  pingback_client()->OverrideRandom(true, 0.4f);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 1);

  // Verify that if the random number is greater than the reporting fraction,
  // the pingback is not created.
  pingback_client()->OverrideRandom(true, 0.6f);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectBucketCount(kHistogramAttempted, false, 1);
  test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_FALSE(test_fetcher);

  // Verify that if the random number is equal to the reporting fraction, the
  // pingback is not created. Specifically, if the reporting fraction is zero,
  // and the random number is zero, no pingback is sent.
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(0.0f);
  pingback_client()->OverrideRandom(true, 0.0f);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectBucketCount(kHistogramAttempted, false, 2);
  test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_FALSE(test_fetcher);

  // Verify that the command line flag forces a pingback.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      data_reduction_proxy::switches::kEnableDataReductionProxyForcePingback);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(0.0f);
  pingback_client()->OverrideRandom(true, 1.0f);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectBucketCount(kHistogramAttempted, true, 2);
  test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 2);
}

TEST_F(DataReductionProxyPingbackClientImplTest, FailedPingback) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  // Simulate a network error.
  test_fetcher->set_status(net::URLRequestStatus(
      net::URLRequestStatus::FAILED, net::ERR_INVALID_AUTH_CREDENTIALS));
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, false, 1);
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyLoFiContentNoOptOut) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  base::Time current_time = base::Time::UnixEpoch();
  pingback_client()->set_current_time(current_time);
  CreateAndSendPingback(
      true /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(PageloadMetrics_PreviewsType_LOFI,
            pageload_metrics.previews_type());
  EXPECT_EQ(PageloadMetrics_PreviewsOptOut_NON_OPT_OUT,
            pageload_metrics.previews_opt_out());

  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyLoFiContentOptOut) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  base::Time current_time = base::Time::UnixEpoch();
  pingback_client()->set_current_time(current_time);
  CreateAndSendPingback(
      true /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      true /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(PageloadMetrics_PreviewsType_LOFI,
            pageload_metrics.previews_type());
  EXPECT_EQ(PageloadMetrics_PreviewsOptOut_OPT_OUT,
            pageload_metrics.previews_opt_out());

  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest,
       VerifyClientLoFiContentOptOut) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  base::Time current_time = base::Time::UnixEpoch();
  pingback_client()->set_current_time(current_time);
  CreateAndSendPingback(
      false /* lofi_received */, true /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      true /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(PageloadMetrics_PreviewsType_LOFI,
            pageload_metrics.previews_type());
  EXPECT_EQ(PageloadMetrics_PreviewsOptOut_OPT_OUT,
            pageload_metrics.previews_opt_out());

  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyLoFiContentBackground) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  base::Time current_time = base::Time::UnixEpoch();
  pingback_client()->set_current_time(current_time);
  CreateAndSendPingback(
      true /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, true /* app_background_occurred */,
      true /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(PageloadMetrics_PreviewsType_LOFI,
            pageload_metrics.previews_type());
  EXPECT_EQ(PageloadMetrics_PreviewsOptOut_UNKNOWN,
            pageload_metrics.previews_opt_out());

  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyLitePageContent) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  base::Time current_time = base::Time::UnixEpoch();
  pingback_client()->set_current_time(current_time);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      true /* lite_page_received */, false /* app_background_occurred */,
      true /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 1);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(PageloadMetrics_PreviewsType_LITE_PAGE,
            pageload_metrics.previews_type());
  EXPECT_EQ(PageloadMetrics_PreviewsOptOut_OPT_OUT,
            pageload_metrics.previews_opt_out());

  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyTwoLitePagePingbacks) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);
  base::Time current_time = base::Time::UnixEpoch();
  pingback_client()->set_current_time(current_time);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      true /* lite_page_received */, false /* app_background_occurred */,
      true /* opt_out_occurred */, false /* renderer_crash */);
  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      true /* lite_page_received */, false /* app_background_occurred */,
      true /* opt_out_occurred */, false /* renderer_crash */);
  histogram_tester().ExpectUniqueSample(kHistogramAttempted, true, 2);
  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(PageloadMetrics_PreviewsType_LITE_PAGE,
            pageload_metrics.previews_type());
  EXPECT_EQ(PageloadMetrics_PreviewsOptOut_OPT_OUT,
            pageload_metrics.previews_opt_out());
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(PageloadMetrics_PreviewsType_LITE_PAGE,
            pageload_metrics.previews_type());
  EXPECT_EQ(PageloadMetrics_PreviewsOptOut_OPT_OUT,
            pageload_metrics.previews_opt_out());
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyCrashOomBehavior) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);

  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, true /* renderer_crash */);

  ReportCrash(true /* oom */);

  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
#if defined(OS_ANDROID)
  EXPECT_EQ(PageloadMetrics_RendererCrashType_ANDROID_FOREGROUND_OOM,
            pageload_metrics.renderer_crash_type());
#else
  EXPECT_EQ(PageloadMetrics_RendererCrashType_NOT_ANALYZED,
            pageload_metrics.renderer_crash_type());
#endif
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 1);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest, VerifyCrashNotOomBehavior) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);

  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, true /* renderer_crash */);

  ReportCrash(false /* oom */);

  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
#if defined(OS_ANDROID)
  EXPECT_EQ(PageloadMetrics_RendererCrashType_OTHER_CRASH,
            pageload_metrics.renderer_crash_type());
#else
  EXPECT_EQ(PageloadMetrics_RendererCrashType_NOT_ANALYZED,
            pageload_metrics.renderer_crash_type());
#endif
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 1);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

TEST_F(DataReductionProxyPingbackClientImplTest,
       VerifyCrashNotAnalyzedBehavior) {
  Init();
  EXPECT_FALSE(factory()->GetFetcherByID(0));
  pingback_client()->OverrideRandom(true, 0.5f);
  static_cast<DataReductionProxyPingbackClient*>(pingback_client())
      ->SetPingbackReportingFraction(1.0f);

  CreateAndSendPingback(
      false /* lofi_received */, false /* client_lofi_requested */,
      false /* lite_page_received */, false /* app_background_occurred */,
      false /* opt_out_occurred */, true /* renderer_crash */);

  // Don't report the crash dump details.
  scoped_task_environment_.FastForwardBy(base::TimeDelta::FromSeconds(5));

  net::TestURLFetcher* test_fetcher = factory()->GetFetcherByID(0);
  EXPECT_TRUE(test_fetcher);
  EXPECT_EQ(test_fetcher->upload_content_type(), "application/x-protobuf");
  RecordPageloadMetricsRequest batched_request;
  batched_request.ParseFromString(test_fetcher->upload_data());
  EXPECT_EQ(batched_request.pageloads_size(), 1);
  PageloadMetrics pageload_metrics = batched_request.pageloads(0);
  EXPECT_EQ(PageloadMetrics_RendererCrashType_NOT_ANALYZED,
            pageload_metrics.renderer_crash_type());
  test_fetcher->delegate()->OnURLFetchComplete(test_fetcher);
  histogram_tester().ExpectUniqueSample(kHistogramSucceeded, true, 1);
  EXPECT_FALSE(factory()->GetFetcherByID(0));
}

}  // namespace data_reduction_proxy
