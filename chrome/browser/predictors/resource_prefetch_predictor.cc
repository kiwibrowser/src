// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/resource_prefetch_predictor.h"

#include <map>
#include <set>
#include <utility>

#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/predictors/loading_data_collector.h"
#include "chrome/browser/predictors/predictor_database.h"
#include "chrome/browser/predictors/predictor_database_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_database.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/url_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;

namespace predictors {

namespace {

const float kMinOriginConfidenceToTriggerPreconnect = 0.75f;
const float kMinOriginConfidenceToTriggerPreresolve = 0.2f;

// For reporting events of interest that are not tied to any navigation.
enum ReportingEvent {
  REPORTING_EVENT_ALL_HISTORY_CLEARED = 0,
  REPORTING_EVENT_PARTIAL_HISTORY_CLEARED = 1,
  REPORTING_EVENT_COUNT = 2
};

float ComputeRedirectConfidence(const predictors::RedirectStat& redirect) {
  return (redirect.number_of_hits() + 0.0) /
         (redirect.number_of_hits() + redirect.number_of_misses());
}

void InitializeOriginStatFromOriginRequestSummary(
    OriginStat* origin,
    const OriginRequestSummary& summary) {
  origin->set_origin(summary.origin.spec());
  origin->set_number_of_hits(1);
  origin->set_average_position(summary.first_occurrence + 1);
  origin->set_always_access_network(summary.always_access_network);
  origin->set_accessed_network(summary.accessed_network);
}

void InitializeOnDBSequence(
    ResourcePrefetchPredictor::RedirectDataMap* host_redirect_data,
    ResourcePrefetchPredictor::OriginDataMap* origin_data) {
  host_redirect_data->InitializeOnDBSequence();
  origin_data->InitializeOnDBSequence();
}

}  // namespace

PreconnectRequest::PreconnectRequest(const GURL& origin, int num_sockets)
    : origin(origin), num_sockets(num_sockets) {
  DCHECK_GE(num_sockets, 0);
}

PreconnectPrediction::PreconnectPrediction() = default;
PreconnectPrediction::PreconnectPrediction(
    const PreconnectPrediction& prediction) = default;
PreconnectPrediction::~PreconnectPrediction() = default;

////////////////////////////////////////////////////////////////////////////////
// ResourcePrefetchPredictor static functions.

bool ResourcePrefetchPredictor::GetRedirectEndpoint(
    const std::string& entry_point,
    const RedirectDataMap& redirect_data,
    std::string* redirect_endpoint) const {
  DCHECK(redirect_endpoint);

  RedirectData data;
  bool exists = redirect_data.TryGetData(entry_point, &data);
  if (!exists) {
    // Fallback to fetching URLs based on the incoming URL/host. By default
    // the predictor is confident that there is no redirect.
    *redirect_endpoint = entry_point;
    return true;
  }

  DCHECK_GT(data.redirect_endpoints_size(), 0);
  if (data.redirect_endpoints_size() > 1) {
    // The predictor observed multiple redirect destinations recently. Redirect
    // endpoint is ambiguous. The predictor predicts a redirect only if it
    // believes that the redirect is "permanent", i.e. subsequent navigations
    // will lead to the same destination.
    return false;
  }

  // The threshold is higher than the threshold for resources because the
  // redirect misprediction causes the waste of whole prefetch.
  const float kMinRedirectConfidenceToTriggerPrefetch = 0.9f;
  const int kMinRedirectHitsToTriggerPrefetch = 2;

  // The predictor doesn't apply a minimum-number-of-hits threshold to
  // the no-redirect case because the no-redirect is a default assumption.
  const RedirectStat& redirect = data.redirect_endpoints(0);
  if (ComputeRedirectConfidence(redirect) <
          kMinRedirectConfidenceToTriggerPrefetch ||
      (redirect.number_of_hits() < kMinRedirectHitsToTriggerPrefetch &&
       redirect.url() != entry_point)) {
    return false;
  }

  *redirect_endpoint = redirect.url();
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// ResourcePrefetchPredictor.

ResourcePrefetchPredictor::ResourcePrefetchPredictor(
    const LoadingPredictorConfig& config,
    Profile* profile)
    : profile_(profile),
      observer_(nullptr),
      config_(config),
      initialization_state_(NOT_INITIALIZED),
      tables_(PredictorDatabaseFactory::GetForProfile(profile)
                  ->resource_prefetch_tables()),
      history_service_observer_(this),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Some form of learning has to be enabled.
  DCHECK(config_.IsLearningEnabled());
}

ResourcePrefetchPredictor::~ResourcePrefetchPredictor() {}

void ResourcePrefetchPredictor::StartInitialization() {
  TRACE_EVENT0("browser", "ResourcePrefetchPredictor::StartInitialization");

  if (initialization_state_ != NOT_INITIALIZED)
    return;
  initialization_state_ = INITIALIZING;

  // Create local caches using the database as loaded.
  auto host_redirect_data = std::make_unique<RedirectDataMap>(
      tables_, tables_->host_redirect_table(), config_.max_hosts_to_track,
      base::TimeDelta::FromSeconds(config_.flush_data_to_disk_delay_seconds));
  auto origin_data = std::make_unique<OriginDataMap>(
      tables_, tables_->origin_table(), config_.max_hosts_to_track,
      base::TimeDelta::FromSeconds(config_.flush_data_to_disk_delay_seconds));

  // Get raw pointers to pass to the first task. Ownership of the unique_ptrs
  // will be passed to the reply task.
  auto task = base::BindOnce(InitializeOnDBSequence, host_redirect_data.get(),
                             origin_data.get());
  auto reply = base::BindOnce(
      &ResourcePrefetchPredictor::CreateCaches, weak_factory_.GetWeakPtr(),
      std::move(host_redirect_data), std::move(origin_data));

  tables_->GetTaskRunner()->PostTaskAndReply(FROM_HERE, std::move(task),
                                             std::move(reply));
}

bool ResourcePrefetchPredictor::IsUrlPreconnectable(
    const GURL& main_frame_url) const {
  return PredictPreconnectOrigins(main_frame_url, nullptr);
}

void ResourcePrefetchPredictor::SetObserverForTesting(TestObserver* observer) {
  observer_ = observer;
}

void ResourcePrefetchPredictor::Shutdown() {
  history_service_observer_.RemoveAll();
}

void ResourcePrefetchPredictor::RecordPageRequestSummary(
    std::unique_ptr<PageRequestSummary> summary) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Make sure initialization is done or start initialization if necessary.
  if (initialization_state_ == NOT_INITIALIZED) {
    StartInitialization();
    return;
  } else if (initialization_state_ == INITIALIZING) {
    return;
  } else if (initialization_state_ != INITIALIZED) {
    NOTREACHED() << "Unexpected initialization_state_: "
                 << initialization_state_;
    return;
  }

  const std::string& host = summary->main_frame_url.host();
  LearnRedirect(summary->initial_url.host(), host, host_redirect_data_.get());

  if (config_.is_origin_learning_enabled)
    LearnOrigins(host, summary->main_frame_url.GetOrigin(), summary->origins);

  if (observer_)
    observer_->OnNavigationLearned(*summary);
}

bool ResourcePrefetchPredictor::PredictPreconnectOrigins(
    const GURL& url,
    PreconnectPrediction* prediction) const {
  DCHECK(!prediction || prediction->requests.empty());
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (initialization_state_ != INITIALIZED)
    return false;

  std::string host = url.host();
  std::string redirect_endpoint;
  if (!GetRedirectEndpoint(host, *host_redirect_data_, &redirect_endpoint))
    return false;

  OriginData data;
  if (!origin_data_->TryGetData(redirect_endpoint, &data))
    return false;

  if (prediction) {
    prediction->host = redirect_endpoint;
    prediction->is_redirected = (host != redirect_endpoint);
  }

  bool has_any_prediction = false;
  for (const OriginStat& origin : data.origins()) {
    float confidence = static_cast<float>(origin.number_of_hits()) /
                       (origin.number_of_hits() + origin.number_of_misses());
    if (confidence < kMinOriginConfidenceToTriggerPreresolve)
      continue;

    has_any_prediction = true;
    if (prediction) {
      if (confidence > kMinOriginConfidenceToTriggerPreconnect)
        prediction->requests.emplace_back(GURL(origin.origin()), 1);
      else
        prediction->requests.emplace_back(GURL(origin.origin()), 0);
    }
  }

  return has_any_prediction;
}

void ResourcePrefetchPredictor::CreateCaches(
    std::unique_ptr<RedirectDataMap> host_redirect_data,
    std::unique_ptr<OriginDataMap> origin_data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(INITIALIZING, initialization_state_);

  DCHECK(host_redirect_data);
  DCHECK(origin_data);

  host_redirect_data_ = std::move(host_redirect_data);
  origin_data_ = std::move(origin_data);

  ConnectToHistoryService();
}

void ResourcePrefetchPredictor::OnHistoryAndCacheLoaded() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(INITIALIZING, initialization_state_);

  initialization_state_ = INITIALIZED;
  if (observer_)
    observer_->OnPredictorInitialized();
}

void ResourcePrefetchPredictor::DeleteAllUrls() {
  host_redirect_data_->DeleteAllData();
  origin_data_->DeleteAllData();
}

void ResourcePrefetchPredictor::DeleteUrls(const history::URLRows& urls) {
  std::vector<std::string> hosts_to_delete;

  for (const auto& it : urls)
    hosts_to_delete.emplace_back(it.url().host());

  host_redirect_data_->DeleteData(hosts_to_delete);
  origin_data_->DeleteData(hosts_to_delete);
}

void ResourcePrefetchPredictor::LearnRedirect(const std::string& key,
                                              const std::string& final_redirect,
                                              RedirectDataMap* redirect_data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // If the primary key is too long reject it.
  if (key.length() > ResourcePrefetchPredictorTables::kMaxStringLength)
    return;

  RedirectData data;
  bool exists = redirect_data->TryGetData(key, &data);
  if (!exists) {
    data.set_primary_key(key);
    data.set_last_visit_time(base::Time::Now().ToInternalValue());
    RedirectStat* redirect_to_add = data.add_redirect_endpoints();
    redirect_to_add->set_url(final_redirect);
    redirect_to_add->set_number_of_hits(1);
  } else {
    data.set_last_visit_time(base::Time::Now().ToInternalValue());

    bool need_to_add = true;
    for (RedirectStat& redirect : *(data.mutable_redirect_endpoints())) {
      if (redirect.url() == final_redirect) {
        need_to_add = false;
        redirect.set_number_of_hits(redirect.number_of_hits() + 1);
        redirect.set_consecutive_misses(0);
      } else {
        redirect.set_number_of_misses(redirect.number_of_misses() + 1);
        redirect.set_consecutive_misses(redirect.consecutive_misses() + 1);
      }
    }

    if (need_to_add) {
      RedirectStat* redirect_to_add = data.add_redirect_endpoints();
      redirect_to_add->set_url(final_redirect);
      redirect_to_add->set_number_of_hits(1);
    }
  }

  // Trim the redirects after the update.
  ResourcePrefetchPredictorTables::TrimRedirects(
      &data, config_.max_redirect_consecutive_misses);

  if (data.redirect_endpoints_size() == 0)
    redirect_data->DeleteData({key});
  else
    redirect_data->UpdateData(key, data);
}

void ResourcePrefetchPredictor::LearnOrigins(
    const std::string& host,
    const GURL& main_frame_origin,
    const std::map<GURL, OriginRequestSummary>& summaries) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (host.size() > ResourcePrefetchPredictorTables::kMaxStringLength)
    return;

  OriginData data;
  bool exists = origin_data_->TryGetData(host, &data);
  if (!exists) {
    data.set_host(host);
    data.set_last_visit_time(base::Time::Now().ToInternalValue());
    size_t origins_size = summaries.size();
    auto ordered_origins =
        std::vector<const OriginRequestSummary*>(origins_size);
    for (const auto& kv : summaries) {
      size_t index = kv.second.first_occurrence;
      DCHECK_LT(index, origins_size);
      ordered_origins[index] = &kv.second;
    }

    for (const OriginRequestSummary* summary : ordered_origins) {
      auto* origin_to_add = data.add_origins();
      InitializeOriginStatFromOriginRequestSummary(origin_to_add, *summary);
    }
  } else {
    data.set_last_visit_time(base::Time::Now().ToInternalValue());

    std::map<GURL, int> old_index;
    int old_size = static_cast<int>(data.origins_size());
    for (int i = 0; i < old_size; ++i) {
      bool is_new =
          old_index.insert({GURL(data.origins(i).origin()), i}).second;
      DCHECK(is_new);
    }

    // Update the old origins.
    for (int i = 0; i < old_size; ++i) {
      auto* old_origin = data.mutable_origins(i);
      GURL origin(old_origin->origin());
      auto it = summaries.find(origin);
      if (it == summaries.end()) {
        // miss
        old_origin->set_number_of_misses(old_origin->number_of_misses() + 1);
        old_origin->set_consecutive_misses(old_origin->consecutive_misses() +
                                           1);
      } else {
        // hit: update.
        const auto& new_origin = it->second;
        old_origin->set_always_access_network(new_origin.always_access_network);
        old_origin->set_accessed_network(new_origin.accessed_network);

        int position = new_origin.first_occurrence + 1;
        int total =
            old_origin->number_of_hits() + old_origin->number_of_misses();
        old_origin->set_average_position(
            ((old_origin->average_position() * total) + position) /
            (total + 1));
        old_origin->set_number_of_hits(old_origin->number_of_hits() + 1);
        old_origin->set_consecutive_misses(0);
      }
    }

    // Add new origins.
    for (const auto& kv : summaries) {
      if (old_index.find(kv.first) != old_index.end())
        continue;

      auto* origin_to_add = data.add_origins();
      InitializeOriginStatFromOriginRequestSummary(origin_to_add, kv.second);
    }
  }

  // Trim and Sort.
  ResourcePrefetchPredictorTables::TrimOrigins(&data,
                                               config_.max_consecutive_misses);
  ResourcePrefetchPredictorTables::SortOrigins(&data, main_frame_origin.spec());
  if (data.origins_size() > static_cast<int>(config_.max_origins_per_entry)) {
    data.mutable_origins()->DeleteSubrange(
        config_.max_origins_per_entry,
        data.origins_size() - config_.max_origins_per_entry);
  }

  // Update the database.
  if (data.origins_size() == 0)
    origin_data_->DeleteData({host});
  else
    origin_data_->UpdateData(host, data);
}

void ResourcePrefetchPredictor::OnURLsDeleted(
    history::HistoryService* history_service,
    const history::DeletionInfo& deletion_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(initialization_state_ == INITIALIZED);

  if (deletion_info.IsAllHistory()) {
    DeleteAllUrls();
    UMA_HISTOGRAM_ENUMERATION("ResourcePrefetchPredictor.ReportingEvent",
                              REPORTING_EVENT_ALL_HISTORY_CLEARED,
                              REPORTING_EVENT_COUNT);
  } else {
    DeleteUrls(deletion_info.deleted_rows());
    UMA_HISTOGRAM_ENUMERATION("ResourcePrefetchPredictor.ReportingEvent",
                              REPORTING_EVENT_PARTIAL_HISTORY_CLEARED,
                              REPORTING_EVENT_COUNT);
  }
}

void ResourcePrefetchPredictor::OnHistoryServiceLoaded(
    history::HistoryService* history_service) {
  if (initialization_state_ == INITIALIZING) {
    OnHistoryAndCacheLoaded();
  }
}

void ResourcePrefetchPredictor::ConnectToHistoryService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(INITIALIZING, initialization_state_);

  // Register for HistoryServiceLoading if it is not ready.
  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfile(profile_,
                                           ServiceAccessType::EXPLICIT_ACCESS);
  if (!history_service)
    return;
  DCHECK(!history_service_observer_.IsObserving(history_service));
  history_service_observer_.Add(history_service);
  if (history_service->BackendLoaded()) {
    // HistoryService is already loaded. Continue with Initialization.
    OnHistoryAndCacheLoaded();
  }
}

////////////////////////////////////////////////////////////////////////////////
// TestObserver.

TestObserver::~TestObserver() {
  predictor_->SetObserverForTesting(nullptr);
}

TestObserver::TestObserver(ResourcePrefetchPredictor* predictor)
    : predictor_(predictor) {
  predictor_->SetObserverForTesting(this);
}

}  // namespace predictors
