// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/external_data_use_reporter.h"

#include <utility>

#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "components/data_usage/core/data_use.h"
#include "components/variations/variations_associated_data.h"

namespace android {

namespace {

// Default duration after which a pending data use report is considered timed
// out. May be overridden by the field trial.
const int kDefaultDataUseReportSubmitTimeoutMsec = 60 * 2 * 1000;  // 2 minutes.

// Default value of the minimum number of bytes that should be buffered before
// a data use report is submitted. May be overridden by the field trial.
const int64_t kDefaultDataUseReportMinBytes = 100 * 1024;  // 100 KB.

// Record the result of data use report submission. |bytes| is the sum of send
// and received bytes in the report.
void RecordDataUsageReportSubmission(
    ExternalDataUseReporter::DataUsageReportSubmissionResult result,
    int64_t bytes) {
  DCHECK_LE(0, bytes);
  UMA_HISTOGRAM_ENUMERATION(
      "DataUsage.ReportSubmissionResult", result,
      ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_MAX);
  // Cap to the maximum sample value.
  const int32_t bytes_capped = bytes <= base::HistogramBase::kSampleType_MAX - 1
                                   ? bytes
                                   : base::HistogramBase::kSampleType_MAX - 1;
  switch (result) {
    case ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_SUCCESSFUL:
      UMA_HISTOGRAM_COUNTS("DataUsage.ReportSubmission.Bytes.Successful",
                           bytes_capped);
      break;
    case ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_FAILED:
      UMA_HISTOGRAM_COUNTS("DataUsage.ReportSubmission.Bytes.Failed",
                           bytes_capped);
      break;
    case ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_TIMED_OUT:
      UMA_HISTOGRAM_COUNTS("DataUsage.ReportSubmission.Bytes.TimedOut",
                           bytes_capped);
      break;
    case ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_LOST:
      UMA_HISTOGRAM_COUNTS("DataUsage.ReportSubmission.Bytes.Lost",
                           bytes_capped);
      break;
    default:
      NOTIMPLEMENTED();
      break;
  }
}

// Populates various parameters from the values specified in the field trial
// |field_trial|.
int32_t GetDataReportSubmitTimeoutMsec(const char* field_trial) {
  int32_t duration_seconds = -1;
  const std::string variation_value = variations::GetVariationParamValue(
      field_trial, "data_report_submit_timeout_msec");
  if (!variation_value.empty() &&
      base::StringToInt(variation_value, &duration_seconds)) {
    DCHECK_LE(0, duration_seconds);
    return duration_seconds;
  }
  return kDefaultDataUseReportSubmitTimeoutMsec;
}

// Populates various parameters from the values specified in the field trial
// |field_trial|.
int64_t GetMinBytes(const char* field_trial) {
  int64_t min_bytes = -1;
  const std::string variation_value = variations::GetVariationParamValue(
      field_trial, "data_use_report_min_bytes");
  if (!variation_value.empty() &&
      base::StringToInt64(variation_value, &min_bytes)) {
    DCHECK_LE(0, min_bytes);
    return min_bytes;
  }
  return kDefaultDataUseReportMinBytes;
}

}  // namespace

// static
const size_t ExternalDataUseReporter::kMaxBufferSize = 100;

ExternalDataUseReporter::ExternalDataUseReporter(
    const char* field_trial,
    const GetTrackingInfoCallback& get_tracking_info_callback,
    const ReportDataUseCallback& report_data_use_callback)
    : get_tracking_info_callback_(get_tracking_info_callback),
      report_data_use_callback_(report_data_use_callback),
      last_data_report_submitted_ticks_(base::TimeTicks()),
      pending_report_bytes_(0),
      previous_report_time_(base::Time::Now()),
      total_bytes_buffered_(0),
      data_use_report_min_bytes_(GetMinBytes(field_trial)),
      data_report_submit_timeout_(base::TimeDelta::FromMilliseconds(
          GetDataReportSubmitTimeoutMsec(field_trial))) {
  DCHECK(get_tracking_info_callback_);
  DCHECK(report_data_use_callback_);
  DCHECK(last_data_report_submitted_ticks_.is_null());
  // Detach from current thread since rest of ExternalDataUseReporter lives on
  // the UI thread and the current thread may not be UI thread..
  thread_checker_.DetachFromThread();
}

void ExternalDataUseReporter::InitOnUIThread() {
  DCHECK(thread_checker_.CalledOnValidThread());
#if defined(OS_ANDROID)
  app_state_listener_.reset(new base::android::ApplicationStatusListener(
      base::Bind(&ExternalDataUseReporter::OnApplicationStateChange,
                 base::Unretained(this))));
#endif
}

ExternalDataUseReporter::~ExternalDataUseReporter() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void ExternalDataUseReporter::OnDataUse(
    std::unique_ptr<const std::vector<const data_usage::DataUse>>
        data_use_list) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(data_use_list);

  const base::Time now_time = base::Time::Now();
  DataUseTabModel::TrackingInfo tracking_info;

  for (const auto& data_use : *data_use_list) {
    if (!get_tracking_info_callback_.Run(
            data_use.tab_id, data_use.request_start, &tracking_info)) {
      continue;
    }

    BufferDataUseReport(data_use, tracking_info.label, tracking_info.tag,
                        previous_report_time_, now_time);
    SubmitBufferedDataUseReport(false);
  }
  previous_report_time_ = now_time;
}

void ExternalDataUseReporter::OnReportDataUseDone(bool success) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!last_data_report_submitted_ticks_.is_null());

  if (success) {
    RecordDataUsageReportSubmission(DATAUSAGE_REPORT_SUBMISSION_SUCCESSFUL,
                                    pending_report_bytes_);
  } else {
    RecordDataUsageReportSubmission(DATAUSAGE_REPORT_SUBMISSION_FAILED,
                                    pending_report_bytes_);
  }
  UMA_HISTOGRAM_TIMES(
      "DataUsage.Perf.ReportSubmissionDuration",
      base::TimeTicks::Now() - last_data_report_submitted_ticks_);

  last_data_report_submitted_ticks_ = base::TimeTicks();
  pending_report_bytes_ = 0;

  SubmitBufferedDataUseReport(false);
}

#if defined(OS_ANDROID)
void ExternalDataUseReporter::OnApplicationStateChange(
    base::android::ApplicationState new_state) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // TODO(rajendrant): When Chromium is backgrounded, only one data use report
  // is submitted since the external API only supports one pending report
  // submission. Once the external API supports submitting multiple reports
  // in one call, all reports should be submitted immediately.
  if (new_state == base::android::APPLICATION_STATE_HAS_PAUSED_ACTIVITIES)
    SubmitBufferedDataUseReport(true);
}
#endif

void ExternalDataUseReporter::BufferDataUseReport(
    const data_usage::DataUse& data_use,
    const std::string& label,
    const std::string& tag,
    const base::Time& start_time,
    const base::Time& end_time) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!label.empty());
  DCHECK_LE(0, data_use.rx_bytes);
  DCHECK_LE(0, data_use.tx_bytes);
  // Skip if the |data_use| does not report any network traffic.
  if (data_use.rx_bytes == 0 && data_use.tx_bytes == 0)
    return;

  DataUseReportKey data_use_report_key =
      DataUseReportKey(label, tag, data_use.connection_type, data_use.mcc_mnc);

  DataUseReport report =
      DataUseReport(start_time, end_time, data_use.rx_bytes, data_use.tx_bytes);

  // Check if the |data_use_report_key| is already in the buffered reports.
  DataUseReports::iterator it =
      buffered_data_reports_.find(data_use_report_key);
  if (it == buffered_data_reports_.end()) {
    // Limit the buffer size.
    if (buffered_data_reports_.size() ==
        ExternalDataUseReporter::kMaxBufferSize) {
      RecordDataUsageReportSubmission(DATAUSAGE_REPORT_SUBMISSION_LOST,
                                      data_use.rx_bytes + data_use.tx_bytes);
      return;
    }
    buffered_data_reports_.insert(std::make_pair(data_use_report_key, report));
  } else {
    DataUseReport existing_report = DataUseReport(it->second);
    DataUseReport merged_report = DataUseReport(
        std::min(existing_report.start_time, report.start_time),
        std::max(existing_report.end_time, report.end_time),
        existing_report.bytes_downloaded + report.bytes_downloaded,
        existing_report.bytes_uploaded + report.bytes_uploaded);
    buffered_data_reports_.erase(it);
    buffered_data_reports_.insert(
        std::make_pair(data_use_report_key, merged_report));
  }
  total_bytes_buffered_ += (data_use.rx_bytes + data_use.tx_bytes);

  DCHECK_LT(0U, buffered_data_reports_.size());
  DCHECK_LE(buffered_data_reports_.size(),
            ExternalDataUseReporter::kMaxBufferSize);
}

void ExternalDataUseReporter::SubmitBufferedDataUseReport(bool immediate) {
  DCHECK(thread_checker_.CalledOnValidThread());

  const base::TimeTicks ticks_now = base::TimeTicks::Now();

  // Return if a data use report has been pending for less than
  // |data_report_submit_timeout_| duration.
  if (!last_data_report_submitted_ticks_.is_null() &&
      ticks_now - last_data_report_submitted_ticks_ <
          data_report_submit_timeout_) {
    return;
  }

  if (buffered_data_reports_.empty())
    return;

  if (!immediate && total_bytes_buffered_ < data_use_report_min_bytes_)
    return;

  if (!last_data_report_submitted_ticks_.is_null()) {
    // Mark the pending DataUsage report as timed out.
    RecordDataUsageReportSubmission(DATAUSAGE_REPORT_SUBMISSION_TIMED_OUT,
                                    pending_report_bytes_);
    pending_report_bytes_ = 0;
    last_data_report_submitted_ticks_ = base::TimeTicks();
  }

  // Send one data use report.
  DataUseReports::iterator it = buffered_data_reports_.begin();
  DataUseReportKey key = it->first;
  DataUseReport report = it->second;

  DCHECK_EQ(0, pending_report_bytes_);
  DCHECK(last_data_report_submitted_ticks_.is_null());
  pending_report_bytes_ = report.bytes_downloaded + report.bytes_uploaded;
  last_data_report_submitted_ticks_ = ticks_now;

  // Remove the entry from the map.
  buffered_data_reports_.erase(it);
  total_bytes_buffered_ -= (report.bytes_downloaded + report.bytes_uploaded);

  report_data_use_callback_.Run(key.label, key.tag, key.connection_type,
                                key.mcc_mnc, report.start_time, report.end_time,
                                report.bytes_downloaded, report.bytes_uploaded);
}

ExternalDataUseReporter::DataUseReportKey::DataUseReportKey(
    const std::string& label,
    const std::string& tag,
    net::NetworkChangeNotifier::ConnectionType connection_type,
    const std::string& mcc_mnc)
    : label(label),
      tag(tag),
      connection_type(connection_type),
      mcc_mnc(mcc_mnc) {}

ExternalDataUseReporter::DataUseReportKey::DataUseReportKey(
    const ExternalDataUseReporter::DataUseReportKey& other) = default;

bool ExternalDataUseReporter::DataUseReportKey::operator==(
    const DataUseReportKey& other) const {
  return label == other.label && tag == other.tag &&
         connection_type == other.connection_type && mcc_mnc == other.mcc_mnc;
}

ExternalDataUseReporter::DataUseReport::DataUseReport(
    const base::Time& start_time,
    const base::Time& end_time,
    int64_t bytes_downloaded,
    int64_t bytes_uploaded)
    : start_time(start_time),
      end_time(end_time),
      bytes_downloaded(bytes_downloaded),
      bytes_uploaded(bytes_uploaded) {}

size_t ExternalDataUseReporter::DataUseReportKeyHash::operator()(
    const DataUseReportKey& k) const {
  //  The hash is computed by hashing individual variables and combining them
  //  using prime numbers. Prime numbers are used for multiplication because the
  //  number of buckets used by map is always an even number. Using a prime
  //  number ensures that for two different DataUseReportKey objects (say |j|
  //  and |k|), if the hash value of |k.label| is equal to hash value of
  //  |j.mcc_mnc|, then |j| and |k| map to different buckets. Large prime
  //  numbers are used so that hash value is spread over a larger range.
  std::hash<std::string> hash_function;
  size_t hash = 1;
  hash = hash * 23 + hash_function(k.label);
  hash = hash * 31 + hash_function(k.tag);
  hash = hash * 43 + k.connection_type;
  hash = hash * 83 + hash_function(k.mcc_mnc);
  return hash;
}

}  // namespace android
