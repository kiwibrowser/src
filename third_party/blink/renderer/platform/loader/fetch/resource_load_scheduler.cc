// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/resource_load_scheduler.h"

#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_number_conversions.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_context.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/renderer/frame_status.h"
#include "third_party/blink/renderer/platform/scheduler/util/aggregated_metric_reporter.h"

namespace blink {

namespace {

// Field trial name.
const char kResourceLoadSchedulerTrial[] = "ResourceLoadScheduler";

// Field trial parameter names.
// Note: bg_limit is supported on m61+, but bg_sub_limit is only on m63+.
// If bg_sub_limit param is not found, we should use bg_limit to make the
// study result statistically correct.
const char kOutstandingLimitForBackgroundMainFrameName[] = "bg_limit";
const char kOutstandingLimitForBackgroundSubFrameName[] = "bg_sub_limit";

// Field trial default parameters.
constexpr size_t kOutstandingLimitForBackgroundFrameDefault = 16u;

// Maximum request count that request count metrics assume.
constexpr base::HistogramBase::Sample kMaximumReportSize10K = 10000;

// Maximum traffic bytes that traffic metrics assume.
constexpr base::HistogramBase::Sample kMaximumReportSize1G =
    1 * 1000 * 1000 * 1000;

// Bucket count for metrics.
constexpr int32_t kReportBucketCount = 25;

constexpr char kRendererSideResourceScheduler[] =
    "RendererSideResourceScheduler";

// These values are copied from resource_scheduler.cc, but the meaning is a bit
// different because ResourceScheduler counts the running delayable requests
// while ResourceLoadScheduler counts all the running requests.
constexpr size_t kTightLimitForRendererSideResourceScheduler = 1u;
constexpr size_t kLimitForRendererSideResourceScheduler = 10u;

constexpr char kTightLimitForRendererSideResourceSchedulerName[] =
    "tight_limit";
constexpr char kLimitForRendererSideResourceSchedulerName[] = "limit";

// Represents a resource load circumstance, e.g. from main frame vs sub-frames,
// or on throttled state vs on not-throttled state.
// Used to report histograms. Do not reorder or insert new items.
enum class ReportCircumstance {
  kMainframeThrottled,
  kMainframeNotThrottled,
  kSubframeThrottled,
  kSubframeNotThrottled,
  // Append new items here.
  kNumOfCircumstances,
};

base::HistogramBase::Sample ToSample(ReportCircumstance circumstance) {
  return static_cast<base::HistogramBase::Sample>(circumstance);
}

uint32_t GetFieldTrialUint32Param(const char* trial_name,
                                  const char* parameter_name,
                                  uint32_t default_param) {
  std::map<std::string, std::string> trial_params;
  bool result = base::GetFieldTrialParams(trial_name, &trial_params);
  if (!result)
    return default_param;

  const auto& found = trial_params.find(parameter_name);
  if (found == trial_params.end())
    return default_param;

  uint32_t param;
  if (!base::StringToUint(found->second, &param))
    return default_param;

  return param;
}

size_t GetOutstandingThrottledLimit(FetchContext* context) {
  DCHECK(context);

  if (!RuntimeEnabledFeatures::ResourceLoadSchedulerEnabled())
    return ResourceLoadScheduler::kOutstandingUnlimited;

  uint32_t main_frame_limit = GetFieldTrialUint32Param(
      kResourceLoadSchedulerTrial, kOutstandingLimitForBackgroundMainFrameName,
      kOutstandingLimitForBackgroundFrameDefault);
  if (context->IsMainFrame())
    return main_frame_limit;

  // We do not have a fixed default limit for sub-frames, but use the limit for
  // the main frame so that it works as how previous versions that haven't
  // consider sub-frames' specific limit work.
  return GetFieldTrialUint32Param(kResourceLoadSchedulerTrial,
                                  kOutstandingLimitForBackgroundSubFrameName,
                                  main_frame_limit);
}

int TakeWholeKilobytes(int64_t& bytes) {
  int kilobytes = bytes / 1024;
  bytes %= 1024;
  return kilobytes;
}

}  // namespace

// A class to gather throttling and traffic information to report histograms.
class ResourceLoadScheduler::TrafficMonitor {
 public:
  explicit TrafficMonitor(FetchContext*);
  ~TrafficMonitor();

  // Notified when the ThrottlingState is changed.
  void OnThrottlingStateChanged(FrameScheduler::ThrottlingState);

  // Reports resource request completion.
  void Report(const ResourceLoadScheduler::TrafficReportHints&);

  // Reports per-frame reports.
  void ReportAll();

 private:
  const bool is_main_frame_;

  const WeakPersistent<FetchContext> context_;  // NOT OWNED

  FrameScheduler::ThrottlingState current_state_ =
      FrameScheduler::ThrottlingState::kStopped;

  size_t total_throttled_request_count_ = 0;
  size_t total_throttled_traffic_bytes_ = 0;
  size_t total_throttled_decoded_bytes_ = 0;
  size_t total_not_throttled_request_count_ = 0;
  size_t total_not_throttled_traffic_bytes_ = 0;
  size_t total_not_throttled_decoded_bytes_ = 0;
  size_t throttling_state_change_count_ = 0;
  bool report_all_is_called_ = false;

  scheduler::AggregatedMetricReporter<scheduler::FrameStatus, int64_t>
      traffic_kilobytes_per_frame_status_;
  scheduler::AggregatedMetricReporter<scheduler::FrameStatus, int64_t>
      decoded_kilobytes_per_frame_status_;
};

ResourceLoadScheduler::TrafficMonitor::TrafficMonitor(FetchContext* context)
    : is_main_frame_(context->IsMainFrame()),
      context_(context),
      traffic_kilobytes_per_frame_status_(
          "Blink.ResourceLoadScheduler.TrafficBytes.KBPerFrameStatus",
          &TakeWholeKilobytes),
      decoded_kilobytes_per_frame_status_(
          "Blink.ResourceLoadScheduler.DecodedBytes.KBPerFrameStatus",
          &TakeWholeKilobytes) {
  DCHECK(context_);
}

ResourceLoadScheduler::TrafficMonitor::~TrafficMonitor() {
  ReportAll();
}

void ResourceLoadScheduler::TrafficMonitor::OnThrottlingStateChanged(
    FrameScheduler::ThrottlingState state) {
  current_state_ = state;
  throttling_state_change_count_++;
}

void ResourceLoadScheduler::TrafficMonitor::Report(
    const ResourceLoadScheduler::TrafficReportHints& hints) {
  // Currently we only care about stats from frames.
  if (!IsMainThread())
    return;
  if (!hints.IsValid())
    return;

  DEFINE_STATIC_LOCAL(EnumerationHistogram, request_count_by_circumstance,
                      ("Blink.ResourceLoadScheduler.RequestCount",
                       ToSample(ReportCircumstance::kNumOfCircumstances)));

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TrafficBytes.MainframeThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_not_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TrafficBytes.MainframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TrafficBytes.SubframeThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_not_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TrafficBytes.SubframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_throttled_decoded_bytes,
      ("Blink.ResourceLoadScheduler.DecodedBytes.MainframeThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_not_throttled_decoded_bytes,
      ("Blink.ResourceLoadScheduler.DecodedBytes.MainframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_throttled_decoded_bytes,
      ("Blink.ResourceLoadScheduler.DecodedBytes.SubframeThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_not_throttled_decoded_bytes,
      ("Blink.ResourceLoadScheduler.DecodedBytes.SubframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));

  switch (current_state_) {
    case FrameScheduler::ThrottlingState::kThrottled:
    case FrameScheduler::ThrottlingState::kHidden:
      if (is_main_frame_) {
        request_count_by_circumstance.Count(
            ToSample(ReportCircumstance::kMainframeThrottled));
        main_frame_throttled_traffic_bytes.Count(hints.encoded_data_length());
        main_frame_throttled_decoded_bytes.Count(hints.decoded_body_length());
      } else {
        request_count_by_circumstance.Count(
            ToSample(ReportCircumstance::kSubframeThrottled));
        sub_frame_throttled_traffic_bytes.Count(hints.encoded_data_length());
        sub_frame_throttled_decoded_bytes.Count(hints.decoded_body_length());
      }
      total_throttled_request_count_++;
      total_throttled_traffic_bytes_ += hints.encoded_data_length();
      total_throttled_decoded_bytes_ += hints.decoded_body_length();
      break;
    case FrameScheduler::ThrottlingState::kNotThrottled:
      if (is_main_frame_) {
        request_count_by_circumstance.Count(
            ToSample(ReportCircumstance::kMainframeNotThrottled));
        main_frame_not_throttled_traffic_bytes.Count(
            hints.encoded_data_length());
        main_frame_not_throttled_decoded_bytes.Count(
            hints.decoded_body_length());
      } else {
        request_count_by_circumstance.Count(
            ToSample(ReportCircumstance::kSubframeNotThrottled));
        sub_frame_not_throttled_traffic_bytes.Count(
            hints.encoded_data_length());
        sub_frame_not_throttled_decoded_bytes.Count(
            hints.decoded_body_length());
      }
      total_not_throttled_request_count_++;
      total_not_throttled_traffic_bytes_ += hints.encoded_data_length();
      total_not_throttled_decoded_bytes_ += hints.decoded_body_length();
      break;
    case FrameScheduler::ThrottlingState::kStopped:
      break;
  }

  // Report kilobytes instead of bytes to avoid overflows.
  size_t encoded_kilobytes = hints.encoded_data_length() / 1024;
  size_t decoded_kilobytes = hints.decoded_body_length() / 1024;

  if (encoded_kilobytes) {
    traffic_kilobytes_per_frame_status_.RecordTask(
        scheduler::GetFrameStatus(context_->GetFrameScheduler()),
        encoded_kilobytes);
  }
  if (decoded_kilobytes) {
    decoded_kilobytes_per_frame_status_.RecordTask(
        scheduler::GetFrameStatus(context_->GetFrameScheduler()),
        decoded_kilobytes);
  }
}

void ResourceLoadScheduler::TrafficMonitor::ReportAll() {
  // Currently we only care about stats from frames.
  if (!IsMainThread())
    return;

  // Blink has several cases to create DocumentLoader not for an actual page
  // load use. I.e., per a XMLHttpRequest in "document" type response.
  // We just ignore such uninteresting cases in following metrics.
  if (!total_throttled_request_count_ && !total_not_throttled_request_count_)
    return;

  if (report_all_is_called_)
    return;
  report_all_is_called_ = true;

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_throttled_request_count,
      ("Blink.ResourceLoadScheduler.TotalRequestCount.MainframeThrottled", 0,
       kMaximumReportSize10K, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_not_throttled_request_count,
      ("Blink.ResourceLoadScheduler.TotalRequestCount.MainframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_throttled_request_count,
      ("Blink.ResourceLoadScheduler.TotalRequestCount.SubframeThrottled", 0,
       kMaximumReportSize10K, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_not_throttled_request_count,
      ("Blink.ResourceLoadScheduler.TotalRequestCount.SubframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucketCount));

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TotalTrafficBytes.MainframeThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_not_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TotalTrafficBytes.MainframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TotalTrafficBytes.SubframeThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_not_throttled_traffic_bytes,
      ("Blink.ResourceLoadScheduler.TotalTrafficBytes.SubframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_throttled_decoded_bytes,
      ("Blink.ResourceLoadScheduler.TotalDecodedBytes.MainframeThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_total_not_throttled_decoded_bytes,
      ("Blink.ResourceLoadScheduler.TotalDecodedBytes.MainframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_throttled_decoded_bytes,
      ("Blink.ResourceLoadScheduler.TotalDecodedBytes.SubframeThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_total_not_throttled_decoded_bytes,
      ("Blink.ResourceLoadScheduler.TotalDecodedBytes.SubframeNotThrottled", 0,
       kMaximumReportSize1G, kReportBucketCount));

  DEFINE_STATIC_LOCAL(CustomCountHistogram, throttling_state_change_count,
                      ("Blink.ResourceLoadScheduler.ThrottlingStateChangeCount",
                       0, 100, kReportBucketCount));

  if (is_main_frame_) {
    main_frame_total_throttled_request_count.Count(
        total_throttled_request_count_);
    main_frame_total_not_throttled_request_count.Count(
        total_not_throttled_request_count_);
    main_frame_total_throttled_traffic_bytes.Count(
        total_throttled_traffic_bytes_);
    main_frame_total_not_throttled_traffic_bytes.Count(
        total_not_throttled_traffic_bytes_);
    main_frame_total_throttled_decoded_bytes.Count(
        total_throttled_decoded_bytes_);
    main_frame_total_not_throttled_decoded_bytes.Count(
        total_not_throttled_decoded_bytes_);
  } else {
    sub_frame_total_throttled_request_count.Count(
        total_throttled_request_count_);
    sub_frame_total_not_throttled_request_count.Count(
        total_not_throttled_request_count_);
    sub_frame_total_throttled_traffic_bytes.Count(
        total_throttled_traffic_bytes_);
    sub_frame_total_not_throttled_traffic_bytes.Count(
        total_not_throttled_traffic_bytes_);
    sub_frame_total_throttled_decoded_bytes.Count(
        total_throttled_decoded_bytes_);
    sub_frame_total_not_throttled_decoded_bytes.Count(
        total_not_throttled_decoded_bytes_);
  }

  throttling_state_change_count.Count(throttling_state_change_count_);
}

constexpr ResourceLoadScheduler::ClientId
    ResourceLoadScheduler::kInvalidClientId;

ResourceLoadScheduler::ResourceLoadScheduler(FetchContext* context)
    : outstanding_limit_for_throttled_frame_scheduler_(
          GetOutstandingThrottledLimit(context)),
      context_(context) {
  traffic_monitor_ =
      std::make_unique<ResourceLoadScheduler::TrafficMonitor>(context_);

  if (!RuntimeEnabledFeatures::ResourceLoadSchedulerEnabled() &&
      !Platform::Current()->IsRendererSideResourceSchedulerEnabled()) {
    // Initialize TrafficMonitor's state to be |kNotThrottled| so that it
    // reports metrics in a reasonable state group.
    traffic_monitor_->OnThrottlingStateChanged(
        FrameScheduler::ThrottlingState::kNotThrottled);
    return;
  }

  auto* scheduler = context->GetFrameScheduler();
  if (!scheduler)
    return;

  if (Platform::Current()->IsRendererSideResourceSchedulerEnabled()) {
    policy_ = context->InitialLoadThrottlingPolicy();
    normal_outstanding_limit_ =
        GetFieldTrialUint32Param(kRendererSideResourceScheduler,
                                 kLimitForRendererSideResourceSchedulerName,
                                 kLimitForRendererSideResourceScheduler);
    tight_outstanding_limit_ = GetFieldTrialUint32Param(
        kRendererSideResourceScheduler,
        kTightLimitForRendererSideResourceSchedulerName,
        kTightLimitForRendererSideResourceScheduler);
  }

  is_enabled_ = true;
  scheduler_observer_handle_ = scheduler->AddThrottlingObserver(
      FrameScheduler::ObserverType::kLoader, this);
}

ResourceLoadScheduler* ResourceLoadScheduler::Create(FetchContext* context) {
  return new ResourceLoadScheduler(context ? context
                                           : &FetchContext::NullInstance());
}

ResourceLoadScheduler::~ResourceLoadScheduler() = default;

void ResourceLoadScheduler::Trace(blink::Visitor* visitor) {
  visitor->Trace(pending_request_map_);
  visitor->Trace(context_);
}

void ResourceLoadScheduler::LoosenThrottlingPolicy() {
  switch (policy_) {
    case ThrottlingPolicy::kTight:
      break;
    case ThrottlingPolicy::kNormal:
      return;
  }
  policy_ = ThrottlingPolicy::kNormal;
  MaybeRun();
}

void ResourceLoadScheduler::Shutdown() {
  // Do nothing if the feature is not enabled, or Shutdown() was already called.
  if (is_shutdown_)
    return;
  is_shutdown_ = true;

  if (traffic_monitor_)
    traffic_monitor_.reset();

  scheduler_observer_handle_.reset();
}

void ResourceLoadScheduler::Request(ResourceLoadSchedulerClient* client,
                                    ThrottleOption option,
                                    ResourceLoadPriority priority,
                                    int intra_priority,
                                    ResourceLoadScheduler::ClientId* id) {
  *id = GenerateClientId();
  if (is_shutdown_)
    return;

  if (!Platform::Current()->IsRendererSideResourceSchedulerEnabled()) {
    // Prioritization is effectively disabled as we use the constant priority.
    priority = ResourceLoadPriority::kMedium;
    intra_priority = 0;
  }

  if (!is_enabled_ || option == ThrottleOption::kCanNotBeThrottled ||
      !IsThrottablePriority(priority)) {
    Run(*id, client, false);
    return;
  }

  pending_requests_.emplace(*id, priority, intra_priority);
  pending_request_map_.insert(
      *id, new ClientWithPriority(client, priority, intra_priority));
  MaybeRun();
}

void ResourceLoadScheduler::SetPriority(ClientId client_id,
                                        ResourceLoadPriority priority,
                                        int intra_priority) {
  if (!Platform::Current()->IsRendererSideResourceSchedulerEnabled())
    return;

  auto client_it = pending_request_map_.find(client_id);
  if (client_it == pending_request_map_.end())
    return;

  auto it = pending_requests_.find(ClientIdWithPriority(
      client_id, client_it->value->priority, client_it->value->intra_priority));

  DCHECK(it != pending_requests_.end());
  pending_requests_.erase(it);

  client_it->value->priority = priority;
  client_it->value->intra_priority = intra_priority;

  pending_requests_.emplace(client_id, priority, intra_priority);
  MaybeRun();
}

bool ResourceLoadScheduler::Release(
    ResourceLoadScheduler::ClientId id,
    ResourceLoadScheduler::ReleaseOption option,
    const ResourceLoadScheduler::TrafficReportHints& hints) {
  // Check kInvalidClientId that can not be passed to the HashSet.
  if (id == kInvalidClientId)
    return false;

  if (running_requests_.find(id) != running_requests_.end()) {
    running_requests_.erase(id);
    running_throttlable_requests_.erase(id);

    if (traffic_monitor_)
      traffic_monitor_->Report(hints);

    if (option == ReleaseOption::kReleaseAndSchedule)
      MaybeRun();
    return true;
  }
  auto found = pending_request_map_.find(id);
  if (found != pending_request_map_.end()) {
    pending_request_map_.erase(found);
    // Intentionally does not remove it from |pending_requests_|.

    // Didn't release any running requests, but the outstanding limit might be
    // changed to allow another request.
    if (option == ReleaseOption::kReleaseAndSchedule)
      MaybeRun();
    return true;
  }
  return false;
}

void ResourceLoadScheduler::SetOutstandingLimitForTesting(size_t tight_limit,
                                                          size_t normal_limit) {
  tight_outstanding_limit_ = tight_limit;
  normal_outstanding_limit_ = normal_limit;
  MaybeRun();
}

void ResourceLoadScheduler::OnNetworkQuiet() {
  DCHECK(IsMainThread());

  // Flush out all traffic reports here for safety.
  traffic_monitor_->ReportAll();

  if (maximum_running_requests_seen_ == 0)
    return;

  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.MainframeThrottled", 0,
       kMaximumReportSize10K, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, main_frame_not_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.MainframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.SubframeThrottled", 0,
       kMaximumReportSize10K, kReportBucketCount));
  DEFINE_STATIC_LOCAL(
      CustomCountHistogram, sub_frame_not_throttled,
      ("Blink.ResourceLoadScheduler.PeakRequests.SubframeNotThrottled", 0,
       kMaximumReportSize10K, kReportBucketCount));

  switch (throttling_history_) {
    case ThrottlingHistory::kInitial:
    case ThrottlingHistory::kNotThrottled:
      if (context_->IsMainFrame())
        main_frame_not_throttled.Count(maximum_running_requests_seen_);
      else
        sub_frame_not_throttled.Count(maximum_running_requests_seen_);
      break;
    case ThrottlingHistory::kThrottled:
      if (context_->IsMainFrame())
        main_frame_throttled.Count(maximum_running_requests_seen_);
      else
        sub_frame_throttled.Count(maximum_running_requests_seen_);
      break;
    case ThrottlingHistory::kPartiallyThrottled:
      break;
    case ThrottlingHistory::kStopped:
      break;
  }
}

bool ResourceLoadScheduler::IsThrottablePriority(
    ResourceLoadPriority priority) const {
  if (!Platform::Current()->IsRendererSideResourceSchedulerEnabled())
    return true;

  if (RuntimeEnabledFeatures::ResourceLoadSchedulerEnabled()) {
    // If this scheduler is throttled by the associated FrameScheduler,
    // consider every prioritiy as throttlable.
    const auto state = frame_scheduler_throttling_state_;
    if (state == FrameScheduler::ThrottlingState::kHidden ||
        state == FrameScheduler::ThrottlingState::kThrottled ||
        state == FrameScheduler::ThrottlingState::kStopped) {
      return true;
    }
  }

  return priority < ResourceLoadPriority::kHigh;
}

void ResourceLoadScheduler::OnThrottlingStateChanged(
    FrameScheduler::ThrottlingState state) {
  if (frame_scheduler_throttling_state_ == state)
    return;

  if (traffic_monitor_)
    traffic_monitor_->OnThrottlingStateChanged(state);

  frame_scheduler_throttling_state_ = state;

  switch (state) {
    case FrameScheduler::ThrottlingState::kHidden:
    case FrameScheduler::ThrottlingState::kThrottled:
      if (throttling_history_ == ThrottlingHistory::kInitial)
        throttling_history_ = ThrottlingHistory::kThrottled;
      else if (throttling_history_ == ThrottlingHistory::kNotThrottled)
        throttling_history_ = ThrottlingHistory::kPartiallyThrottled;
      break;
    case FrameScheduler::ThrottlingState::kNotThrottled:
      if (throttling_history_ == ThrottlingHistory::kInitial)
        throttling_history_ = ThrottlingHistory::kNotThrottled;
      else if (throttling_history_ == ThrottlingHistory::kThrottled)
        throttling_history_ = ThrottlingHistory::kPartiallyThrottled;
      break;
    case FrameScheduler::ThrottlingState::kStopped:
      throttling_history_ = ThrottlingHistory::kStopped;
      break;
  }
  MaybeRun();
}

ResourceLoadScheduler::ClientId ResourceLoadScheduler::GenerateClientId() {
  ClientId id = ++current_id_;
  CHECK_NE(0u, id);
  return id;
}

void ResourceLoadScheduler::MaybeRun() {
  // Requests for keep-alive loaders could be remained in the pending queue,
  // but ignore them once Shutdown() is called.
  if (is_shutdown_)
    return;

  while (!pending_requests_.empty()) {
    if (IsThrottablePriority(pending_requests_.begin()->priority) &&
        running_throttlable_requests_.size() >= GetOutstandingLimit()) {
      break;
    }

    ClientId id = pending_requests_.begin()->client_id;
    pending_requests_.erase(pending_requests_.begin());
    auto found = pending_request_map_.find(id);
    if (found == pending_request_map_.end())
      continue;  // Already released.
    ResourceLoadSchedulerClient* client = found->value->client;
    pending_request_map_.erase(found);
    Run(id, client, true);
  }
}

void ResourceLoadScheduler::Run(ResourceLoadScheduler::ClientId id,
                                ResourceLoadSchedulerClient* client,
                                bool throttlable) {
  running_requests_.insert(id);
  if (throttlable)
    running_throttlable_requests_.insert(id);
  if (running_requests_.size() > maximum_running_requests_seen_) {
    maximum_running_requests_seen_ = running_requests_.size();
  }
  client->Run();
}

size_t ResourceLoadScheduler::GetOutstandingLimit() const {
  size_t limit = kOutstandingUnlimited;

  switch (frame_scheduler_throttling_state_) {
    case FrameScheduler::ThrottlingState::kHidden:
    case FrameScheduler::ThrottlingState::kThrottled:
      limit = std::min(limit, outstanding_limit_for_throttled_frame_scheduler_);
      break;
    case FrameScheduler::ThrottlingState::kNotThrottled:
      break;
    case FrameScheduler::ThrottlingState::kStopped:
      if (RuntimeEnabledFeatures::ResourceLoadSchedulerEnabled())
        limit = 0;
      break;
  }

  switch (policy_) {
    case ThrottlingPolicy::kTight:
      limit = std::min(limit, tight_outstanding_limit_);
      break;
    case ThrottlingPolicy::kNormal:
      limit = std::min(limit, normal_outstanding_limit_);
      break;
  }
  return limit;
}

}  // namespace blink
