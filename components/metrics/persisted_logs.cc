// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/persisted_logs.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/md5.h"
#include "base/metrics/histogram_macros.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/timer/elapsed_timer.h"
#include "components/metrics/persisted_logs_metrics.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/zlib/google/compression_utils.h"

namespace metrics {

namespace {

const char kLogHashKey[] = "hash";
const char kLogTimestampKey[] = "timestamp";
const char kLogDataKey[] = "data";

std::string EncodeToBase64(const std::string& to_convert) {
  // CHECK to diagnose crbug.com/695433
  CHECK(to_convert.data());
  std::string base64_result;
  base::Base64Encode(to_convert, &base64_result);
  return base64_result;
}

std::string DecodeFromBase64(const std::string& to_convert) {
  std::string result;
  base::Base64Decode(to_convert, &result);
  return result;
}

}  // namespace

void PersistedLogs::LogInfo::Init(PersistedLogsMetrics* metrics,
                                  const std::string& log_data,
                                  const std::string& log_timestamp) {
  DCHECK(!log_data.empty());

  if (!compression::GzipCompress(log_data, &compressed_log_data)) {
    NOTREACHED();
    return;
  }

  metrics->RecordCompressionRatio(compressed_log_data.size(), log_data.size());

  hash = base::SHA1HashString(log_data);
  timestamp = log_timestamp;
}

PersistedLogs::PersistedLogs(std::unique_ptr<PersistedLogsMetrics> metrics,
                             PrefService* local_state,
                             const char* pref_name,
                             size_t min_log_count,
                             size_t min_log_bytes,
                             size_t max_log_size)
    : metrics_(std::move(metrics)),
      local_state_(local_state),
      pref_name_(pref_name),
      min_log_count_(min_log_count),
      min_log_bytes_(min_log_bytes),
      max_log_size_(max_log_size != 0 ? max_log_size : static_cast<size_t>(-1)),
      staged_log_index_(-1) {
  DCHECK(local_state_);
  // One of the limit arguments must be non-zero.
  DCHECK(min_log_count_ > 0 || min_log_bytes_ > 0);
}

PersistedLogs::~PersistedLogs() {}

bool PersistedLogs::has_unsent_logs() const {
  return !!size();
}

// True if a log has been staged.
bool PersistedLogs::has_staged_log() const {
  return staged_log_index_ != -1;
}

// Returns the element in the front of the list.
const std::string& PersistedLogs::staged_log() const {
  DCHECK(has_staged_log());
  return list_[staged_log_index_].compressed_log_data;
}

// Returns the element in the front of the list.
const std::string& PersistedLogs::staged_log_hash() const {
  DCHECK(has_staged_log());
  return list_[staged_log_index_].hash;
}

// Returns the timestamp of the element in the front of the list.
const std::string& PersistedLogs::staged_log_timestamp() const {
  DCHECK(has_staged_log());
  return list_[staged_log_index_].timestamp;
}

void PersistedLogs::StageNextLog() {
  // CHECK, rather than DCHECK, because swap()ing with an empty list causes
  // hard-to-identify crashes much later.
  CHECK(!list_.empty());
  DCHECK(!has_staged_log());
  staged_log_index_ = list_.size() - 1;
  DCHECK(has_staged_log());
}

void PersistedLogs::DiscardStagedLog() {
  // CHECK, rather than DCHECK, to diagnose cause of crashes from the field,
  // for crbug.com/695433.
  CHECK(has_staged_log());
  DCHECK_LT(static_cast<size_t>(staged_log_index_), list_.size());
  list_.erase(list_.begin() + staged_log_index_);
  staged_log_index_ = -1;
}

void PersistedLogs::PersistUnsentLogs() const {
  ListPrefUpdate update(local_state_, pref_name_);
  WriteLogsToPrefList(update.Get());
}

void PersistedLogs::LoadPersistedUnsentLogs() {
  ReadLogsFromPrefList(*local_state_->GetList(pref_name_));
}

void PersistedLogs::StoreLog(const std::string& log_data) {
  list_.push_back(LogInfo());
  list_.back().Init(metrics_.get(), log_data,
                    base::Int64ToString(base::Time::Now().ToTimeT()));
}

void PersistedLogs::Purge() {
  if (has_staged_log()) {
    DiscardStagedLog();
  }
  list_.clear();
  local_state_->ClearPref(pref_name_);
}

void PersistedLogs::ReadLogsFromPrefList(const base::ListValue& list_value) {
  if (list_value.empty()) {
    metrics_->RecordLogReadStatus(PersistedLogsMetrics::LIST_EMPTY);
    return;
  }

  const size_t log_count = list_value.GetSize();

  DCHECK(list_.empty());
  list_.resize(log_count);

  for (size_t i = 0; i < log_count; ++i) {
    const base::DictionaryValue* dict;
    if (!list_value.GetDictionary(i, &dict) ||
        !dict->GetString(kLogDataKey, &list_[i].compressed_log_data) ||
        !dict->GetString(kLogHashKey, &list_[i].hash)) {
      list_.clear();
      metrics_->RecordLogReadStatus(
          PersistedLogsMetrics::LOG_STRING_CORRUPTION);
      return;
    }

    list_[i].compressed_log_data =
        DecodeFromBase64(list_[i].compressed_log_data);
    list_[i].hash = DecodeFromBase64(list_[i].hash);
    // Ignoring the success of this step as timestamp might not be there for
    // older logs.
    // NOTE: Should be added to the check with other fields once migration is
    // over.
    dict->GetString(kLogTimestampKey, &list_[i].timestamp);
  }

  metrics_->RecordLogReadStatus(PersistedLogsMetrics::RECALL_SUCCESS);
}

void PersistedLogs::WriteLogsToPrefList(base::ListValue* list_value) const {
  list_value->Clear();

  // Keep the most recent logs which are smaller than |max_log_size_|.
  // We keep at least |min_log_bytes_| and |min_log_count_| of logs before
  // discarding older logs.
  size_t start = list_.size();
  size_t saved_log_count = 0;
  size_t bytes_used = 0;
  for (; start > 0; --start) {
    size_t log_size = list_[start - 1].compressed_log_data.length();
    if (bytes_used >= min_log_bytes_ &&
        saved_log_count >= min_log_count_) {
      break;
    }
    // Oversized logs won't be persisted, so don't count them.
    if (log_size > max_log_size_)
      continue;
    bytes_used += log_size;
    ++saved_log_count;
  }
  int dropped_logs_num = start - 1;

  for (size_t i = start; i < list_.size(); ++i) {
    size_t log_size = list_[i].compressed_log_data.length();
    if (log_size > max_log_size_) {
      metrics_->RecordDroppedLogSize(log_size);
      dropped_logs_num++;
      continue;
    }
    std::unique_ptr<base::DictionaryValue> dict_value(
        new base::DictionaryValue);
    dict_value->SetString(kLogHashKey, EncodeToBase64(list_[i].hash));
    dict_value->SetString(kLogDataKey,
                          EncodeToBase64(list_[i].compressed_log_data));
    dict_value->SetString(kLogTimestampKey, list_[i].timestamp);
    list_value->Append(std::move(dict_value));
  }
  if (dropped_logs_num > 0)
    metrics_->RecordDroppedLogsNum(dropped_logs_num);
}

}  // namespace metrics
