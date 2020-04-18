// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/rappor_service_impl.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/metrics/metrics_hashes.h"
#include "base/time/time.h"
#include "components/rappor/log_uploader.h"
#include "components/rappor/proto/rappor_metric.pb.h"
#include "components/rappor/rappor_metric.h"
#include "components/rappor/rappor_pref_names.h"
#include "components/rappor/rappor_prefs.h"
#include "components/variations/variations_associated_data.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace rappor {

namespace {

// Seconds before the initial log is generated.
const int kInitialLogIntervalSeconds = 15;
// Interval between ongoing logs.
const int kLogIntervalSeconds = 30 * 60;

const char kMimeType[] = "application/vnd.chrome.rappor";

const char kRapporDailyEventHistogram[] = "Rappor.DailyEvent.IntervalType";

// The rappor server's URL.
const char kDefaultServerUrl[] = "https://clients4.google.com/rappor";

}  // namespace

RapporServiceImpl::RapporServiceImpl(
    PrefService* pref_service,
    const base::Callback<bool(void)> is_incognito_callback)
    : pref_service_(pref_service),
      is_incognito_callback_(is_incognito_callback),
      cohort_(-1),
      daily_event_(pref_service,
                   prefs::kRapporLastDailySample,
                   kRapporDailyEventHistogram),
      recording_enabled_(false) {}

RapporServiceImpl::~RapporServiceImpl() {}

void RapporServiceImpl::AddDailyObserver(
    std::unique_ptr<metrics::DailyEvent::Observer> observer) {
  daily_event_.AddObserver(std::move(observer));
}

void RapporServiceImpl::Initialize(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!IsInitialized());
  const GURL server_url = GURL(kDefaultServerUrl);
  if (!server_url.is_valid()) {
    DVLOG(1) << server_url.spec() << " is invalid. "
             << "RapporServiceImpl not started.";
    return;
  }
  DVLOG(1) << "RapporServiceImpl reporting to " << server_url.spec();
  InitializeInternal(
      std::make_unique<LogUploader>(server_url, kMimeType, url_loader_factory),
      internal::LoadCohort(pref_service_), internal::LoadSecret(pref_service_));
}

void RapporServiceImpl::Update(bool may_record, bool may_upload) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsInitialized());
  if (recording_enabled_ != may_record) {
    recording_enabled_ = may_record;
    if (!may_record) {
      DVLOG(1) << "Rappor service stopped because all groups were disabled.";
      CancelNextLogRotation();
    } else {
      DVLOG(1) << "RapporServiceImpl started.";
      ScheduleNextLogRotation(
          base::TimeDelta::FromSeconds(kInitialLogIntervalSeconds));
    }
  }

  DVLOG(1) << "RapporServiceImpl recording_groups=" << recording_enabled_
           << " may_upload=" << may_upload;
  if (may_upload) {
    uploader_->Start();
  } else {
    uploader_->Stop();
  }
}

// static
void RapporServiceImpl::RegisterPrefs(PrefRegistrySimple* registry) {
  internal::RegisterPrefs(registry);
}

void RapporServiceImpl::InitializeInternal(
    std::unique_ptr<LogUploaderInterface> uploader,
    int32_t cohort,
    const std::string& secret) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!IsInitialized());
  DCHECK(secret_.empty());
  uploader_.swap(uploader);
  cohort_ = cohort;
  secret_ = secret;
}

void RapporServiceImpl::CancelNextLogRotation() {
  DCHECK(thread_checker_.CalledOnValidThread());
  metrics_map_.clear();
  log_rotation_timer_.Stop();
}

void RapporServiceImpl::ScheduleNextLogRotation(base::TimeDelta interval) {
  log_rotation_timer_.Start(FROM_HERE, interval, this,
                            &RapporServiceImpl::OnLogInterval);
}

void RapporServiceImpl::OnLogInterval() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(uploader_);
  DVLOG(2) << "RapporServiceImpl::OnLogInterval";
  daily_event_.CheckInterval();
  RapporReports reports;
  if (ExportMetrics(&reports)) {
    std::string log_text;
    bool success = reports.SerializeToString(&log_text);
    DCHECK(success);
    DVLOG(1) << "RapporServiceImpl sending a report of "
             << reports.report_size() << " value(s).";
    uploader_->QueueLog(log_text);
  }
  ScheduleNextLogRotation(base::TimeDelta::FromSeconds(kLogIntervalSeconds));
}

bool RapporServiceImpl::ExportMetrics(RapporReports* reports) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_GE(cohort_, 0);
  reports->set_cohort(cohort_);

  for (const auto& kv : metrics_map_) {
    const RapporMetric* metric = kv.second.get();
    RapporReports::Report* report = reports->add_report();
    report->set_name_hash(base::HashMetricName(kv.first));
    ByteVector bytes = metric->GetReport(secret_);
    report->set_bits(std::string(bytes.begin(), bytes.end()));
    DVLOG(2) << "Exporting metric " << kv.first;
  }
  metrics_map_.clear();

  sampler_.ExportMetrics(secret_, reports);

  DVLOG(2) << "Generated a report with " << reports->report_size()
           << " metrics.";
  return reports->report_size() > 0;
}

bool RapporServiceImpl::IsInitialized() const {
  return cohort_ >= 0;
}

bool RapporServiceImpl::RecordingAllowed(const RapporParameters& parameters) {
  // Skip recording in incognito mode.
  if (is_incognito_callback_.Run()) {
    DVLOG(2) << "Metric not logged due to incognito mode.";
    return false;
  }
  // Skip this metric if its recording_group is not enabled.
  if (!recording_enabled_) {
    DVLOG(2) << "Metric not logged due to recording_enabled_ = false.";
    return false;
  }
  return true;
}

void RapporServiceImpl::RecordSampleString(const std::string& metric_name,
                                           RapporType type,
                                           const std::string& sample) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Ignore the sample if the service hasn't started yet.
  if (!IsInitialized())
    return;
  DCHECK_LT(type, NUM_RAPPOR_TYPES);
  const RapporParameters& parameters = internal::kRapporParametersForType[type];
  DVLOG(2) << "Recording sample \"" << sample << "\" for metric \""
           << metric_name << "\" of type: " << type;
  RecordSampleInternal(metric_name, parameters, sample);
}

void RapporServiceImpl::RecordSampleInternal(const std::string& metric_name,
                                             const RapporParameters& parameters,
                                             const std::string& sample) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsInitialized());
  if (!RecordingAllowed(parameters))
    return;
  RapporMetric* metric = LookUpMetric(metric_name, parameters);
  metric->AddSample(sample);
}

RapporMetric* RapporServiceImpl::LookUpMetric(
    const std::string& metric_name,
    const RapporParameters& parameters) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsInitialized());
  auto it = metrics_map_.find(metric_name);
  if (it != metrics_map_.end()) {
    RapporMetric* metric = it->second.get();
    DCHECK_EQ(parameters.ToString(), metric->parameters().ToString());
    return metric;
  }

  RapporMetric* new_metric = new RapporMetric(metric_name, parameters, cohort_);
  metrics_map_[metric_name] = base::WrapUnique(new_metric);
  return new_metric;
}

std::unique_ptr<Sample> RapporServiceImpl::CreateSample(RapporType type) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(IsInitialized());
  return base::WrapUnique(
      new Sample(cohort_, internal::kRapporParametersForType[type]));
}

void RapporServiceImpl::RecordSample(const std::string& metric_name,
                                     std::unique_ptr<Sample> sample) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!RecordingAllowed(sample->parameters()))
    return;
  DVLOG(1) << "Recording sample of metric \"" << metric_name << "\"";
  sampler_.AddSample(metric_name, std::move(sample));
}

}  // namespace rappor
