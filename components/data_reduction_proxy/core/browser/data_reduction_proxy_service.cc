// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_service.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_compression_stats.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_io_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_pingback_client.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_service_observer.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_settings.h"
#include "components/data_reduction_proxy/core/browser/data_store.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_store.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"
#include "components/data_reduction_proxy/proto/data_store.pb.h"
#include "components/prefs/pref_service.h"

namespace data_reduction_proxy {

DataReductionProxyService::DataReductionProxyService(
    DataReductionProxySettings* settings,
    PrefService* prefs,
    net::URLRequestContextGetter* request_context_getter,
    std::unique_ptr<DataStore> store,
    std::unique_ptr<DataReductionProxyPingbackClient> pingback_client,
    const scoped_refptr<base::SequencedTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    const scoped_refptr<base::SequencedTaskRunner>& db_task_runner,
    const base::TimeDelta& commit_delay)
    : url_request_context_getter_(request_context_getter),
      pingback_client_(std::move(pingback_client)),
      settings_(settings),
      prefs_(prefs),
      db_data_owner_(new DBDataOwner(std::move(store))),
      io_task_runner_(io_task_runner),
      db_task_runner_(db_task_runner),
      initialized_(false),
      weak_factory_(this) {
  DCHECK(settings);
  db_task_runner_->PostTask(FROM_HERE,
                            base::Bind(&DBDataOwner::InitializeOnDBThread,
                                       db_data_owner_->GetWeakPtr()));
  if (prefs_) {
    compression_stats_.reset(
        new DataReductionProxyCompressionStats(this, prefs_, commit_delay));
  }
  event_store_.reset(new DataReductionProxyEventStore());
}

DataReductionProxyService::~DataReductionProxyService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  compression_stats_.reset();
  db_task_runner_->DeleteSoon(FROM_HERE, db_data_owner_.release());
}

void DataReductionProxyService::SetIOData(
    base::WeakPtr<DataReductionProxyIOData> io_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  io_data_ = io_data;
  initialized_ = true;
  for (DataReductionProxyServiceObserver& observer : observer_list_)
    observer.OnServiceInitialized();

  ReadPersistedClientConfig();
}

void DataReductionProxyService::ReadPersistedClientConfig() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!prefs_)
    return;

  base::Time last_config_retrieval_time =
      base::Time() + base::TimeDelta::FromMicroseconds(prefs_->GetInt64(
                         prefs::kDataReductionProxyLastConfigRetrievalTime));
  base::TimeDelta time_since_last_config_retrieval =
      base::Time::Now() - last_config_retrieval_time;

  // A config older than 24 hours should not be used.
  bool persisted_config_is_expired =
      GetFieldTrialParamByFeatureAsBool(
          features::kDataReductionProxyRobustConnection,
          "use_24h_config_expiration_time", true) &&
      !last_config_retrieval_time.is_null() &&
      time_since_last_config_retrieval > base::TimeDelta::FromHours(24);

  if (persisted_config_is_expired)
    return;

  const std::string config_value =
      prefs_->GetString(prefs::kDataReductionProxyConfig);

  if (config_value.empty())
    return;

  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&DataReductionProxyIOData::SetDataReductionProxyConfiguration,
                 io_data_, config_value));
}

void DataReductionProxyService::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  weak_factory_.InvalidateWeakPtrs();
}

void DataReductionProxyService::UpdateDataUseForHost(int64_t network_bytes,
                                                     int64_t original_bytes,
                                                     const std::string& host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (compression_stats_) {
    compression_stats_->RecordDataUseByHost(host, network_bytes, original_bytes,
                                            base::Time::Now());
  }
}

void DataReductionProxyService::UpdateContentLengths(
    int64_t data_used,
    int64_t original_size,
    bool data_reduction_proxy_enabled,
    DataReductionProxyRequestType request_type,
    const std::string& mime_type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (compression_stats_) {
    compression_stats_->RecordDataUseWithMimeType(data_used, original_size,
                                                  data_reduction_proxy_enabled,
                                                  request_type, mime_type);
  }
}

void DataReductionProxyService::AddEvent(std::unique_ptr<base::Value> event) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  event_store_->AddEvent(std::move(event));
}

void DataReductionProxyService::AddEnabledEvent(
    std::unique_ptr<base::Value> event,
    bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  event_store_->AddEnabledEvent(std::move(event), enabled);
}

void DataReductionProxyService::AddEventAndSecureProxyCheckState(
    std::unique_ptr<base::Value> event,
    SecureProxyCheckState state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  event_store_->AddEventAndSecureProxyCheckState(std::move(event), state);
}

void DataReductionProxyService::AddAndSetLastBypassEvent(
    std::unique_ptr<base::Value> event,
    int64_t expiration_ticks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  event_store_->AddAndSetLastBypassEvent(std::move(event), expiration_ticks);
}

void DataReductionProxyService::SetUnreachable(bool unreachable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  settings_->SetUnreachable(unreachable);
}

void DataReductionProxyService::SetInt64Pref(const std::string& pref_path,
                                             int64_t value) {
  if (prefs_)
    prefs_->SetInt64(pref_path, value);
}

void DataReductionProxyService::SetStringPref(const std::string& pref_path,
                                              const std::string& value) {
  if (prefs_)
    prefs_->SetString(pref_path, value);
}

void DataReductionProxyService::SetProxyPrefs(bool enabled, bool at_startup) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (io_task_runner_->BelongsToCurrentThread()) {
    io_data_->SetProxyPrefs(enabled, at_startup);
    return;
  }
  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DataReductionProxyIOData::SetProxyPrefs, io_data_,
                            enabled, at_startup));
}

void DataReductionProxyService::SetPingbackReportingFraction(
    float pingback_reporting_fraction) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  pingback_client_->SetPingbackReportingFraction(pingback_reporting_fraction);
}

void DataReductionProxyService::OnCacheCleared(const base::Time start,
                                               const base::Time end) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DataReductionProxyIOData::OnCacheCleared,
                                io_data_, start, end));
}

void DataReductionProxyService::LoadHistoricalDataUsage(
    const HistoricalDataUsageCallback& load_data_usage_callback) {
  std::unique_ptr<std::vector<DataUsageBucket>> data_usage(
      new std::vector<DataUsageBucket>());
  std::vector<DataUsageBucket>* data_usage_ptr = data_usage.get();
  db_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&DBDataOwner::LoadHistoricalDataUsage,
                     db_data_owner_->GetWeakPtr(),
                     base::Unretained(data_usage_ptr)),
      base::BindOnce(load_data_usage_callback, std::move(data_usage)));
}

void DataReductionProxyService::LoadCurrentDataUsageBucket(
    const LoadCurrentDataUsageCallback& load_current_data_usage_callback) {
  std::unique_ptr<DataUsageBucket> bucket(new DataUsageBucket());
  DataUsageBucket* bucket_ptr = bucket.get();
  db_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&DBDataOwner::LoadCurrentDataUsageBucket,
                     db_data_owner_->GetWeakPtr(),
                     base::Unretained(bucket_ptr)),
      base::BindOnce(load_current_data_usage_callback, std::move(bucket)));
}

void DataReductionProxyService::StoreCurrentDataUsageBucket(
    std::unique_ptr<DataUsageBucket> current) {
  db_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&DBDataOwner::StoreCurrentDataUsageBucket,
                     db_data_owner_->GetWeakPtr(), std::move(current)));
}

void DataReductionProxyService::DeleteHistoricalDataUsage() {
  db_task_runner_->PostTask(FROM_HERE,
                            base::Bind(&DBDataOwner::DeleteHistoricalDataUsage,
                                       db_data_owner_->GetWeakPtr()));
}

void DataReductionProxyService::DeleteBrowsingHistory(const base::Time& start,
                                                      const base::Time& end) {
  DCHECK_LE(start, end);
  db_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DBDataOwner::DeleteBrowsingHistory,
                            db_data_owner_->GetWeakPtr(), start, end));

  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DataReductionProxyIOData::DeleteBrowsingHistory,
                            io_data_, start, end));
}

void DataReductionProxyService::AddObserver(
    DataReductionProxyServiceObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observer_list_.AddObserver(observer);
}

void DataReductionProxyService::RemoveObserver(
    DataReductionProxyServiceObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observer_list_.RemoveObserver(observer);
}

bool DataReductionProxyService::Initialized() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return initialized_;
}

base::WeakPtr<DataReductionProxyService>
DataReductionProxyService::GetWeakPtr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return weak_factory_.GetWeakPtr();
}

}  // namespace data_reduction_proxy
