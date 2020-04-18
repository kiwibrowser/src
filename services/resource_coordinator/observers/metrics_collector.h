// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_

#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "services/resource_coordinator/observers/background_metrics_reporter.h"
#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"

namespace resource_coordinator {

class CoordinationUnitBase;
class FrameCoordinationUnitImpl;
class PageCoordinationUnitImpl;

extern const char kTabFromBackgroundedToFirstAlertFiredUMA[];
extern const char kTabFromBackgroundedToFirstAudioStartsUMA[];
extern const char kTabFromBackgroundedToFirstFaviconUpdatedUMA[];
extern const char kTabFromBackgroundedToFirstTitleUpdatedUMA[];
extern const char
    kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA[];
extern const base::TimeDelta kMaxAudioSlientTimeout;
extern const base::TimeDelta kMetricsReportDelayTimeout;
extern const int kDefaultFrequencyUkmEQTReported;

// A MetricsCollector observes changes happened inside CoordinationUnit Graph,
// and reports UMA/UKM.
class MetricsCollector : public CoordinationUnitGraphObserver {
 public:
  MetricsCollector();
  ~MetricsCollector() override;

  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitBase* coordination_unit) override;
  void OnCoordinationUnitCreated(
      const CoordinationUnitBase* coordination_unit) override;
  void OnBeforeCoordinationUnitDestroyed(
      const CoordinationUnitBase* coordination_unit) override;
  void OnFramePropertyChanged(const FrameCoordinationUnitImpl* frame_cu,
                              const mojom::PropertyType property_type,
                              int64_t value) override;
  void OnPagePropertyChanged(const PageCoordinationUnitImpl* page_cu,
                             const mojom::PropertyType property_type,
                             int64_t value) override;
  void OnProcessPropertyChanged(const ProcessCoordinationUnitImpl* process_cu,
                                const mojom::PropertyType property_type,
                                int64_t value) override;
  void OnFrameEventReceived(const FrameCoordinationUnitImpl* frame_cu,
                            const mojom::Event event) override;
  void OnPageEventReceived(const PageCoordinationUnitImpl* page_cu,
                           const mojom::Event event) override;

 private:
  struct MetricsReportRecord {
    MetricsReportRecord();
    MetricsReportRecord(const MetricsReportRecord& other);
    void UpdateUKMSourceID(int64_t ukm_source_id);
    void Reset();
    BackgroundMetricsReporter<
        ukm::builders::TabManager_Background_FirstAlertFired,
        kTabFromBackgroundedToFirstAlertFiredUMA,
        internal::UKMFrameReportType::kMainFrameAndChildFrame>
        first_alert_fired;
    BackgroundMetricsReporter<
        ukm::builders::TabManager_Background_FirstAudioStarts,
        kTabFromBackgroundedToFirstAudioStartsUMA,
        internal::UKMFrameReportType::kMainFrameAndChildFrame>
        first_audible;
    BackgroundMetricsReporter<
        ukm::builders::TabManager_Background_FirstFaviconUpdated,
        kTabFromBackgroundedToFirstFaviconUpdatedUMA,
        internal::UKMFrameReportType::kMainFrameOnly>
        first_favicon_updated;
    BackgroundMetricsReporter<
        ukm::builders::
            TabManager_Background_FirstNonPersistentNotificationCreated,
        kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA,
        internal::UKMFrameReportType::kMainFrameAndChildFrame>
        first_non_persistent_notification_created;
    BackgroundMetricsReporter<
        ukm::builders::TabManager_Background_FirstTitleUpdated,
        kTabFromBackgroundedToFirstTitleUpdatedUMA,
        internal::UKMFrameReportType::kMainFrameOnly>
        first_title_updated;
  };

  struct UkmCollectionState {
    size_t num_cpu_usage_measurements = 0u;
    int num_unreported_eqt_measurements = 0u;
    ukm::SourceId ukm_source_id = ukm::kInvalidSourceId;
  };

  bool ShouldReportMetrics(const PageCoordinationUnitImpl* page_cu);
  bool IsCollectingCPUUsageForUkm(const CoordinationUnitID& page_cu_id);
  bool IsCollectingExpectedQueueingTimeForUkm(
      const CoordinationUnitID& page_cu_id);
  void RecordCPUUsageForUkm(const CoordinationUnitID& page_cu_id,
                            double cpu_usage,
                            size_t num_coresident_tabs);
  void RecordExpectedQueueingTimeForUkm(const CoordinationUnitID& page_cu_id,
                                        int64_t expected_queueing_time);
  void UpdateUkmSourceIdForPage(const CoordinationUnitID& page_cu_id,
                                ukm::SourceId ukm_source_id);
  void UpdateWithFieldTrialParams();
  void ResetMetricsReportRecord(CoordinationUnitID cu_id);

  // The metrics_report_record_map_ is used to record whether a metric was
  // already reported to avoid reporting multiple metrics.
  std::map<CoordinationUnitID, MetricsReportRecord> metrics_report_record_map_;
  std::map<CoordinationUnitID, UkmCollectionState> ukm_collection_state_map_;
  size_t max_ukm_cpu_usage_measurements_ = 0u;
  // The number of reports to wait before reporting ExpectedQueueingTime. For
  // example, if |frequency_ukm_eqt_reported_| is 2, then the first value is not
  // reported, the second one is, the third one isn't, etc.
  int frequency_ukm_eqt_reported_;
  DISALLOW_COPY_AND_ASSIGN(MetricsCollector);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_METRICS_COLLECTOR_H_
