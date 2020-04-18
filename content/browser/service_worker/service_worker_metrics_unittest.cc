// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_metrics.h"

#include "base/test/histogram_tester.h"
#include "content/browser/service_worker/embedded_worker_status.h"
#include "content/test/test_content_browser_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {
const std::string kNavigationPreloadSuffix = "_NavigationPreloadEnabled";
const std::string kStartWorkerDuringStartupSuffix = "_StartWorkerDuringStartup";
const std::string kWorkerStartOccurred = "_WorkerStartOccurred";
const std::string kStartWorkerExistingReadyProcessSuffix =
    "_StartWorkerExistingReadyProcess";
const std::string kPreparationTime =
    "ServiceWorker.ActivatedWorkerPreparationForMainFrame.Time";
const std::string kPreparationType =
    "ServiceWorker.ActivatedWorkerPreparationForMainFrame.Type";
const std::string kPreparationTypeSearch =
    "ServiceWorker.ActivatedWorkerPreparationForMainFrame.Type.search";

void ExpectNoNavPreloadMainFrameUMA(
    const base::HistogramTester& histogram_tester) {
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerPreparationType_MainFrame", 0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame", 0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame", 0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame", 0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame", 0);

  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame_WorkerStartOccurred", 0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame_WorkerStartOccurred",
      0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame_WorkerStartOccurred",
      0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame_WorkerStartOccurred",
      0);
}

void ExpectNoNavPreloadWorkerStartOccurredUMA(
    const base::HistogramTester& histogram_tester) {
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame_WorkerStartOccurred", 0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame_WorkerStartOccurred",
      0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame_WorkerStartOccurred",
      0);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame_WorkerStartOccurred",
      0);
}

}  // namespace

using CrossProcessTimeDelta = ServiceWorkerMetrics::CrossProcessTimeDelta;
using StartSituation = ServiceWorkerMetrics::StartSituation;
using WorkerPreparationType = ServiceWorkerMetrics::WorkerPreparationType;

class MetricTestContentBrowserClient : public TestContentBrowserClient {
 public:
  std::string GetMetricSuffixForURL(const GURL& url) override {
    if (url.host() == "search.example.com")
      return "search";
    return std::string();
  }
};

TEST(ServiceWorkerMetricsTest, ActivatedWorkerPreparation) {
  base::TimeDelta time = base::TimeDelta::FromMilliseconds(123);
  {
    // Test preparation when the worker was STARTING.
    base::HistogramTester histogram_tester;
    ServiceWorkerMetrics::RecordActivatedWorkerPreparationForMainFrame(
        time, EmbeddedWorkerStatus::STARTING,
        ServiceWorkerMetrics::StartSituation::UNKNOWN,
        false /* did_navigation_preload */, GURL("https://example.com"));
    histogram_tester.ExpectUniqueSample(
        kPreparationType, static_cast<int>(WorkerPreparationType::STARTING), 1);
    histogram_tester.ExpectTotalCount(
        kPreparationType + kNavigationPreloadSuffix, 0);
    histogram_tester.ExpectTimeBucketCount(kPreparationTime, time, 1);
    histogram_tester.ExpectTimeBucketCount(kPreparationTime + "_StartingWorker",
                                           time, 1);
    histogram_tester.ExpectTotalCount(
        kPreparationTime + kNavigationPreloadSuffix, 0);
  }

  {
    // Test preparation when the worker started up during startup.
    base::HistogramTester histogram_tester;
    ServiceWorkerMetrics::RecordActivatedWorkerPreparationForMainFrame(
        time, EmbeddedWorkerStatus::STOPPED, StartSituation::DURING_STARTUP,
        true /* did_navigation_preload */, GURL("https://example.com"));
    histogram_tester.ExpectUniqueSample(
        kPreparationType,
        static_cast<int>(WorkerPreparationType::START_DURING_STARTUP), 1);
    histogram_tester.ExpectUniqueSample(
        kPreparationType + kNavigationPreloadSuffix,
        static_cast<int>(WorkerPreparationType::START_DURING_STARTUP), 1);
    histogram_tester.ExpectTimeBucketCount(kPreparationTime, time, 1);
    histogram_tester.ExpectTimeBucketCount(
        kPreparationTime + kNavigationPreloadSuffix, time, 1);
    histogram_tester.ExpectTotalCount(
        kPreparationTime + kWorkerStartOccurred + kNavigationPreloadSuffix, 1);
  }

  {
    // Test preparation when the worker started up in an existing process.
    base::HistogramTester histogram_tester;
    ServiceWorkerMetrics::RecordActivatedWorkerPreparationForMainFrame(
        time, EmbeddedWorkerStatus::STOPPED,
        StartSituation::EXISTING_READY_PROCESS,
        true /* did_navigation_preload */, GURL("https://example.com"));
    histogram_tester.ExpectUniqueSample(
        kPreparationType,
        static_cast<int>(
            WorkerPreparationType::START_IN_EXISTING_READY_PROCESS),
        1);
    histogram_tester.ExpectUniqueSample(
        kPreparationType + kNavigationPreloadSuffix,
        static_cast<int>(
            WorkerPreparationType::START_IN_EXISTING_READY_PROCESS),
        1);
    histogram_tester.ExpectTimeBucketCount(kPreparationTime, time, 1);
    histogram_tester.ExpectTimeBucketCount(
        kPreparationTime + kNavigationPreloadSuffix, time, 1);
    histogram_tester.ExpectTimeBucketCount(
        kPreparationTime + kWorkerStartOccurred + kNavigationPreloadSuffix,
        time, 1);
  }

  // Suffixed metric test.
  MetricTestContentBrowserClient test_browser_client;
  ContentBrowserClient* old_browser_client =
      SetBrowserClientForTesting(&test_browser_client);
  {
    base::HistogramTester histogram_tester;
    ServiceWorkerMetrics::RecordActivatedWorkerPreparationForMainFrame(
        time, EmbeddedWorkerStatus::STOPPED,
        StartSituation::EXISTING_READY_PROCESS,
        true /* did_navigation_preload */, GURL("https://search.example.com"));
    histogram_tester.ExpectUniqueSample(
        kPreparationType,
        static_cast<int>(
            WorkerPreparationType::START_IN_EXISTING_READY_PROCESS),
        1);
    histogram_tester.ExpectUniqueSample(
        kPreparationTypeSearch,
        static_cast<int>(
            WorkerPreparationType::START_IN_EXISTING_READY_PROCESS),
        1);
  }
  {
    base::HistogramTester histogram_tester;
    ServiceWorkerMetrics::RecordActivatedWorkerPreparationForMainFrame(
        time, EmbeddedWorkerStatus::STOPPED,
        StartSituation::EXISTING_READY_PROCESS,
        true /* did_navigation_preload */,
        GURL("https://notsearch.example.com"));
    histogram_tester.ExpectUniqueSample(
        kPreparationType,
        static_cast<int>(
            WorkerPreparationType::START_IN_EXISTING_READY_PROCESS),
        1);
    histogram_tester.ExpectTotalCount(kPreparationTypeSearch, 0);
  }
  SetBrowserClientForTesting(old_browser_client);
}

// ===========================================================================
// Navigation preload tests
// ===========================================================================
// The worker was already running.
TEST(ServiceWorkerMetricsTest,
     NavigationPreloadResponse_WorkerAlreadyRunning_MainFrame) {
  base::TimeDelta worker_start = base::TimeDelta::FromMilliseconds(123);
  base::TimeDelta response_start = base::TimeDelta::FromMilliseconds(789);
  EmbeddedWorkerStatus initial_worker_status = EmbeddedWorkerStatus::RUNNING;
  StartSituation start_situation = StartSituation::EXISTING_READY_PROCESS;
  base::HistogramTester histogram_tester;
  ServiceWorkerMetrics::RecordNavigationPreloadResponse(
      worker_start, response_start, initial_worker_status, start_situation,
      RESOURCE_TYPE_MAIN_FRAME);

  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.WorkerPreparationType_MainFrame",
      static_cast<int>(WorkerPreparationType::RUNNING), 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame", response_start, 1);
  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame", false, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame", worker_start, 1);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame", 0);

  ExpectNoNavPreloadWorkerStartOccurredUMA(histogram_tester);
}

// The worker was already running (subframe).
TEST(ServiceWorkerMetricsTest,
     NavigationPreloadResponse_WorkerAlreadyRunning_SubFrame) {
  base::TimeDelta worker_start = base::TimeDelta::FromMilliseconds(123);
  base::TimeDelta response_start = base::TimeDelta::FromMilliseconds(789);
  EmbeddedWorkerStatus initial_worker_status = EmbeddedWorkerStatus::RUNNING;
  StartSituation start_situation = StartSituation::EXISTING_READY_PROCESS;
  base::HistogramTester histogram_tester;
  ServiceWorkerMetrics::RecordNavigationPreloadResponse(
      worker_start, response_start, initial_worker_status, start_situation,
      RESOURCE_TYPE_SUB_FRAME);

  ExpectNoNavPreloadMainFrameUMA(histogram_tester);
}

// The worker started up.
TEST(ServiceWorkerMetricsTest,
     NavigationPreloadResponse_WorkerStart_MainFrame) {
  base::TimeDelta worker_start = base::TimeDelta::FromMilliseconds(234);
  base::TimeDelta response_start = base::TimeDelta::FromMilliseconds(789);
  EmbeddedWorkerStatus initial_worker_status = EmbeddedWorkerStatus::STOPPED;
  StartSituation start_situation = StartSituation::NEW_PROCESS;
  base::HistogramTester histogram_tester;
  ServiceWorkerMetrics::RecordNavigationPreloadResponse(
      worker_start, response_start, initial_worker_status, start_situation,
      RESOURCE_TYPE_MAIN_FRAME);

  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.WorkerPreparationType_MainFrame",
      static_cast<int>(WorkerPreparationType::START_IN_NEW_PROCESS), 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame", response_start, 1);
  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame", false, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame", worker_start, 1);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame", 0);

  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame_WorkerStartOccurred",
      response_start, 1);
  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame_WorkerStartOccurred",
      false, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame_WorkerStartOccurred",
      worker_start, 1);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame_WorkerStartOccurred",
      0);
}

// The worker started up (subframe).
TEST(ServiceWorkerMetricsTest, NavigationPreloadResponse_WorkerStart_SubFrame) {
  base::TimeDelta worker_start = base::TimeDelta::FromMilliseconds(234);
  base::TimeDelta response_start = base::TimeDelta::FromMilliseconds(789);
  EmbeddedWorkerStatus initial_worker_status = EmbeddedWorkerStatus::STOPPED;
  StartSituation start_situation = StartSituation::EXISTING_READY_PROCESS;
  base::HistogramTester histogram_tester;
  ServiceWorkerMetrics::RecordNavigationPreloadResponse(
      worker_start, response_start, initial_worker_status, start_situation,
      RESOURCE_TYPE_SUB_FRAME);

  ExpectNoNavPreloadMainFrameUMA(histogram_tester);
}

// The worker was already starting up (STARTING). This gets logged to the
// _WorkerStartOccurred suffix as well.
TEST(ServiceWorkerMetricsTest,
     NavigationPreloadResponse_WorkerStart_WorkerStarting) {
  base::TimeDelta worker_start = base::TimeDelta::FromMilliseconds(234);
  base::TimeDelta response_start = base::TimeDelta::FromMilliseconds(789);
  EmbeddedWorkerStatus initial_worker_status = EmbeddedWorkerStatus::STARTING;
  StartSituation start_situation = StartSituation::NEW_PROCESS;
  base::HistogramTester histogram_tester;
  ServiceWorkerMetrics::RecordNavigationPreloadResponse(
      worker_start, response_start, initial_worker_status, start_situation,
      RESOURCE_TYPE_MAIN_FRAME);

  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.WorkerPreparationType_MainFrame",
      static_cast<int>(WorkerPreparationType::STARTING), 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame", response_start, 1);
  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame", false, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame", worker_start, 1);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame", 0);

  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame_"
      "WorkerStartOccurred",
      response_start, 1);
  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame_"
      "WorkerStartOccurred",
      false, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame_"
      "WorkerStartOccurred",
      worker_start, 1);
  histogram_tester.ExpectTotalCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame_"
      "WorkerStartOccurred",
      0);
}

// The worker started up, but navigation preload arrived first.
TEST(ServiceWorkerMetricsTest,
     NavigationPreloadResponse_NavPreloadFinishedFirst) {
  base::TimeDelta worker_start = base::TimeDelta::FromMilliseconds(2345);
  base::TimeDelta response_start = base::TimeDelta::FromMilliseconds(456);
  base::TimeDelta wait_time = worker_start - response_start;
  EmbeddedWorkerStatus initial_worker_status = EmbeddedWorkerStatus::STOPPING;
  StartSituation start_situation = StartSituation::NEW_PROCESS;
  base::HistogramTester histogram_tester;
  ServiceWorkerMetrics::RecordNavigationPreloadResponse(
      worker_start, response_start, initial_worker_status, start_situation,
      RESOURCE_TYPE_MAIN_FRAME);
  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.WorkerPreparationType_MainFrame",
      static_cast<int>(WorkerPreparationType::STOPPING), 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ResponseTime_MainFrame", response_start, 1);
  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame", true, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame", response_start, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame", wait_time, 1);

  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.FinishedFirst_MainFrame_"
      "WorkerStartOccurred",
      true, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.ConcurrentTime_MainFrame_"
      "WorkerStartOccurred",
      response_start, 1);
  histogram_tester.ExpectTimeBucketCount(
      "ServiceWorker.NavPreload.WorkerWaitTime_MainFrame_"
      "WorkerStartOccurred",
      wait_time, 1);
}

// The worker started up, but navigation preload arrived first (subframe).
TEST(ServiceWorkerMetricsTest,
     NavigationPreloadResponse_NavPreloadFinishedFirst_SubFrame) {
  base::TimeDelta worker_start = base::TimeDelta::FromMilliseconds(2345);
  base::TimeDelta response_start = base::TimeDelta::FromMilliseconds(456);
  EmbeddedWorkerStatus initial_worker_status = EmbeddedWorkerStatus::STOPPED;
  StartSituation start_situation = StartSituation::NEW_PROCESS;
  base::HistogramTester histogram_tester;
  ServiceWorkerMetrics::RecordNavigationPreloadResponse(
      worker_start, response_start, initial_worker_status, start_situation,
      RESOURCE_TYPE_SUB_FRAME);

  ExpectNoNavPreloadMainFrameUMA(histogram_tester);
}

// The worker started up during browser startup.
TEST(ServiceWorkerMetricsTest, NavigationPreloadResponse_BrowserStartup) {
  base::TimeDelta worker_start = base::TimeDelta::FromMilliseconds(2345);
  base::TimeDelta response_start = base::TimeDelta::FromMilliseconds(456);
  EmbeddedWorkerStatus initial_worker_status = EmbeddedWorkerStatus::STOPPED;
  StartSituation start_situation = StartSituation::DURING_STARTUP;
  base::HistogramTester histogram_tester;
  ServiceWorkerMetrics::RecordNavigationPreloadResponse(
      worker_start, response_start, initial_worker_status, start_situation,
      RESOURCE_TYPE_MAIN_FRAME);
  histogram_tester.ExpectUniqueSample(
      "ServiceWorker.NavPreload.WorkerPreparationType_MainFrame",
      static_cast<int>(WorkerPreparationType::START_DURING_STARTUP), 1);
}

TEST(ServiceWorkerMetricsTest, EmbeddedWorkerStartTiming) {
  base::TimeTicks start_worker_sent_time = base::TimeTicks::Now();
  const base::TimeDelta latency = base::TimeDelta::FromMilliseconds(33);
  auto start_timing = mojom::EmbeddedWorkerStartTiming::New();
  start_timing->blink_initialized_time = start_worker_sent_time;
  start_timing->start_worker_received_time = start_worker_sent_time + latency;
  StartSituation start_situation = StartSituation::EXISTING_READY_PROCESS;
  base::HistogramTester histogram_tester;

  ServiceWorkerMetrics::RecordEmbeddedWorkerStartTiming(
      std::move(start_timing), start_worker_sent_time, start_situation);
  histogram_tester.ExpectUniqueSample(
      "EmbeddedWorkerInstance.Start.StartMessageLatency.Type",
      static_cast<int>(CrossProcessTimeDelta::NORMAL), 1);
  histogram_tester.ExpectTimeBucketCount(
      "EmbeddedWorkerInstance.Start.StartMessageLatency", latency, 1);
  histogram_tester.ExpectTimeBucketCount(
      "EmbeddedWorkerInstance.Start.StartMessageLatency_ExistingReadyProcess",
      latency, 1);
  histogram_tester.ExpectUniqueSample(
      "EmbeddedWorkerInstance.Start.WaitedForRendererSetup", false, 1);
  histogram_tester.ExpectTotalCount(
      "EmbeddedWorkerInstance.Start.WaitedForRendererSetup.Time", 0);
}

TEST(ServiceWorkerMetricsTest, EmbeddedWorkerStartTiming_BrowserStartup) {
  base::TimeTicks start_worker_sent_time = base::TimeTicks::Now();
  const base::TimeDelta latency = base::TimeDelta::FromMilliseconds(66);
  auto start_timing = mojom::EmbeddedWorkerStartTiming::New();
  start_timing->blink_initialized_time = start_worker_sent_time;
  start_timing->start_worker_received_time = start_worker_sent_time + latency;
  StartSituation start_situation = StartSituation::DURING_STARTUP;
  base::HistogramTester histogram_tester;

  ServiceWorkerMetrics::RecordEmbeddedWorkerStartTiming(
      std::move(start_timing), start_worker_sent_time, start_situation);
  histogram_tester.ExpectUniqueSample(
      "EmbeddedWorkerInstance.Start.StartMessageLatency.Type",
      static_cast<int>(CrossProcessTimeDelta::NORMAL), 1);
  histogram_tester.ExpectTimeBucketCount(
      "EmbeddedWorkerInstance.Start.StartMessageLatency", latency, 1);
  histogram_tester.ExpectTimeBucketCount(
      "EmbeddedWorkerInstance.Start.StartMessageLatency_DuringStartup", latency,
      1);
  histogram_tester.ExpectUniqueSample(
      "EmbeddedWorkerInstance.Start.WaitedForRendererSetup", false, 1);
  histogram_tester.ExpectTotalCount(
      "EmbeddedWorkerInstance.Start.WaitedForRendererSetup.Time", 0);
}

TEST(ServiceWorkerMetricsTest, EmbeddedWorkerStartTiming_WaitForRenderer) {
  base::TimeTicks start_worker_sent_time = base::TimeTicks::Now();
  const base::TimeDelta latency = base::TimeDelta::FromMilliseconds(777);
  const base::TimeDelta renderer_setup = base::TimeDelta::FromMilliseconds(333);
  auto start_timing = mojom::EmbeddedWorkerStartTiming::New();
  start_timing->blink_initialized_time =
      start_worker_sent_time + renderer_setup;
  start_timing->start_worker_received_time = start_worker_sent_time + latency;
  StartSituation start_situation = StartSituation::NEW_PROCESS;
  base::HistogramTester histogram_tester;

  ServiceWorkerMetrics::RecordEmbeddedWorkerStartTiming(
      std::move(start_timing), start_worker_sent_time, start_situation);
  histogram_tester.ExpectUniqueSample(
      "EmbeddedWorkerInstance.Start.StartMessageLatency.Type",
      static_cast<int>(CrossProcessTimeDelta::NORMAL), 1);
  histogram_tester.ExpectTimeBucketCount(
      "EmbeddedWorkerInstance.Start.StartMessageLatency", latency, 1);
  histogram_tester.ExpectTimeBucketCount(
      "EmbeddedWorkerInstance.Start.StartMessageLatency_NewProcess", latency,
      1);
  histogram_tester.ExpectUniqueSample(
      "EmbeddedWorkerInstance.Start.WaitedForRendererSetup", true, 1);
  histogram_tester.ExpectTimeBucketCount(
      "EmbeddedWorkerInstance.Start.WaitedForRendererSetup.Time",
      renderer_setup, 1);
}

TEST(ServiceWorkerMetricsTest, EmbeddedWorkerStartTiming_NegativeLatency) {
  base::TimeTicks start_worker_sent_time = base::TimeTicks::Now();
  const base::TimeDelta latency = base::TimeDelta::FromMilliseconds(777);
  auto start_timing = mojom::EmbeddedWorkerStartTiming::New();
  start_timing->blink_initialized_time = start_worker_sent_time;
  start_timing->start_worker_received_time = start_worker_sent_time - latency;
  StartSituation start_situation = StartSituation::EXISTING_READY_PROCESS;
  base::HistogramTester histogram_tester;

  ServiceWorkerMetrics::RecordEmbeddedWorkerStartTiming(
      std::move(start_timing), start_worker_sent_time, start_situation);
  histogram_tester.ExpectUniqueSample(
      "EmbeddedWorkerInstance.Start.StartMessageLatency.Type",
      static_cast<int>(CrossProcessTimeDelta::NEGATIVE), 1);
  histogram_tester.ExpectTotalCount(
      "EmbeddedWorkerInstance.Start.StartMessageLatency", 0);
  histogram_tester.ExpectTotalCount(
      "EmbeddedWorkerInstance.Start.StartMessageLatency_ExistingReadyProcess",
      0);
  histogram_tester.ExpectTotalCount(
      "EmbeddedWorkerInstance.Start.WaitedForRendererSetup", 0);
  histogram_tester.ExpectTotalCount(
      "EmbeddedWorkerInstance.Start.WaitedForRendererSetup.Time", 0);
}

}  // namespace content
