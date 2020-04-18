// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_use_measurement/core/data_use_measurement.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "components/data_use_measurement/core/data_use_ascriber.h"
#include "components/data_use_measurement/core/data_use_recorder.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/data_use_measurement/core/url_request_classifier.h"
#include "components/domain_reliability/uploader.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "net/base/network_change_notifier.h"
#include "net/base/upload_data_stream.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request.h"

#if defined(OS_ANDROID)
#include "net/android/traffic_stats.h"
#endif

namespace data_use_measurement {

namespace {

// Records the occurrence of |sample| in |name| histogram. Conventional UMA
// histograms are not used because the |name| is not static.
void RecordUMAHistogramCount(const std::string& name, int64_t sample) {
  base::HistogramBase* histogram_pointer = base::Histogram::FactoryGet(
      name,
      1,        // Minimum sample size in bytes.
      1000000,  // Maximum sample size in bytes. Should cover most of the
                // requests by services.
      50,       // Bucket count.
      base::HistogramBase::kUmaTargetedHistogramFlag);
  histogram_pointer->Add(sample);
}

// This function increases the value of |sample| bucket in |name| sparse
// histogram by |value|. Conventional UMA histograms are not used because |name|
// is not static.
void IncreaseSparseHistogramByValue(const std::string& name,
                                    int64_t sample,
                                    int64_t value) {
  base::HistogramBase* histogram = base::SparseHistogram::FactoryGet(
      name + "KB", base::HistogramBase::kUmaTargetedHistogramFlag);
  histogram->AddKiB(sample, value);
}

#if defined(OS_ANDROID)
void IncrementLatencyHistogramByCount(const std::string& name,
                                      const base::TimeDelta& latency,
                                      int64_t count) {
  base::HistogramBase* histogram_pointer = base::Histogram::FactoryTimeGet(
      name,
      base::TimeDelta::FromMilliseconds(1),  // Minimum sample
      base::TimeDelta::FromHours(1),         // Maximum sample
      50,                                    // Bucket count.
      base::HistogramBase::kUmaTargetedHistogramFlag);
  histogram_pointer->AddCount(latency.InMilliseconds(), count);
}
#endif

void RecordFavIconDataUse(const net::URLRequest& request) {
  UMA_HISTOGRAM_COUNTS_100000(
      "DataUse.FavIcon.Downstream",
      request.was_cached() ? 0 : request.GetTotalReceivedBytes());
  if (request.status().is_success() &&
      request.GetResponseCode() != net::HTTP_OK) {
    UMA_HISTOGRAM_COUNTS_100000("DataUse.FavIcon.Downstream.Non200Response",
                                request.GetTotalReceivedBytes());
  }
}

}  // namespace

DataUseMeasurement::DataUseMeasurement(
    std::unique_ptr<URLRequestClassifier> url_request_classifier,
    const metrics::UpdateUsagePrefCallbackType& metrics_data_use_forwarder,
    DataUseAscriber* ascriber)
    : url_request_classifier_(std::move(url_request_classifier)),
      metrics_data_use_forwarder_(metrics_data_use_forwarder),
      ascriber_(ascriber)
#if defined(OS_ANDROID)
      ,
      app_state_(base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES),
      app_listener_(new base::android::ApplicationStatusListener(
          base::Bind(&DataUseMeasurement::OnApplicationStateChange,
                     base::Unretained(this)))),
      rx_bytes_os_(0),
      tx_bytes_os_(0),
      bytes_transferred_since_last_traffic_stats_query_(0),
      no_reads_since_background_(false)
#endif
{
  DCHECK(ascriber_);
  DCHECK(url_request_classifier_);

#if defined(OS_ANDROID)
  int64_t bytes = 0;
  // Query Android traffic stats.
  if (net::android::traffic_stats::GetCurrentUidRxBytes(&bytes))
    rx_bytes_os_ = bytes;

  if (net::android::traffic_stats::GetCurrentUidTxBytes(&bytes))
    tx_bytes_os_ = bytes;
#endif
}

DataUseMeasurement::~DataUseMeasurement(){};

void DataUseMeasurement::OnBeforeURLRequest(net::URLRequest* request) {
  DataUseUserData* data_use_user_data = reinterpret_cast<DataUseUserData*>(
      request->GetUserData(DataUseUserData::kUserDataKey));
  if (!data_use_user_data) {
    DataUseUserData::ServiceName service_name =
        DataUseUserData::ServiceName::NOT_TAGGED;
    if (!url_request_classifier_->IsUserRequest(*request) &&
        domain_reliability::DomainReliabilityUploader::
            OriginatedFromDomainReliability(*request)) {
      // Detect if the request originated from DomainReliability.
      // DataUseUserData::AttachToFetcher() cannot be called from domain
      // reliability, since it sets userdata on URLFetcher for its purposes.
      service_name = DataUseUserData::ServiceName::DOMAIN_RELIABILITY;
    } else if (gaia::RequestOriginatedFromGaia(*request)) {
      service_name = DataUseUserData::ServiceName::GAIA;
    }

    data_use_user_data = new DataUseUserData(service_name, CurrentAppState());
    request->SetUserData(DataUseUserData::kUserDataKey,
                         base::WrapUnique(data_use_user_data));
  } else {
    data_use_user_data->set_app_state(CurrentAppState());
  }
}

void DataUseMeasurement::OnBeforeRedirect(const net::URLRequest& request,
                                          const GURL& new_location) {
  // Recording data use of request on redirects.
  // TODO(rajendrant): May not be needed when http://crbug/651957 is fixed.
  UpdateDataUsePrefs(request);
  ReportServicesMessageSizeUMA(request);
  if (url_request_classifier_->IsFavIconRequest(request))
    RecordFavIconDataUse(request);
}

void DataUseMeasurement::OnHeadersReceived(
    net::URLRequest* request,
    const net::HttpResponseHeaders* response_headers) {
  DataUseUserData* data_use_user_data = reinterpret_cast<DataUseUserData*>(
      request->GetUserData(DataUseUserData::kUserDataKey));
  if (data_use_user_data) {
    data_use_user_data->set_content_type(
        url_request_classifier_->GetContentType(*request, *response_headers));
  }
}

void DataUseMeasurement::OnNetworkBytesReceived(const net::URLRequest& request,
                                                int64_t bytes_received) {
  UMA_HISTOGRAM_COUNTS("DataUse.BytesReceived.Delegate", bytes_received);
  ReportDataUseUMA(request, DOWNSTREAM, bytes_received);
#if defined(OS_ANDROID)
  bytes_transferred_since_last_traffic_stats_query_ += bytes_received;
#endif
}

void DataUseMeasurement::OnNetworkBytesSent(const net::URLRequest& request,
                                            int64_t bytes_sent) {
  UMA_HISTOGRAM_COUNTS("DataUse.BytesSent.Delegate", bytes_sent);
  ReportDataUseUMA(request, UPSTREAM, bytes_sent);
#if defined(OS_ANDROID)
  bytes_transferred_since_last_traffic_stats_query_ += bytes_sent;
#endif
}

void DataUseMeasurement::OnCompleted(const net::URLRequest& request,
                                     bool started) {
  // TODO(amohammadkhan): Verify that there is no double recording in data use
  // of redirected requests.
  UpdateDataUsePrefs(request);
  ReportServicesMessageSizeUMA(request);
  RecordPageTransitionUMA(request);
#if defined(OS_ANDROID)
  MaybeRecordNetworkBytesOS();
#endif
  if (url_request_classifier_->IsFavIconRequest(request))
    RecordFavIconDataUse(request);
}

void DataUseMeasurement::ReportDataUseUMA(const net::URLRequest& request,
                                          TrafficDirection dir,
                                          int64_t bytes) {
  bool is_user_traffic = url_request_classifier_->IsUserRequest(request);
  bool is_connection_cellular =
      net::NetworkChangeNotifier::IsConnectionCellular(
          net::NetworkChangeNotifier::GetConnectionType());

  DataUseUserData* attached_service_data = static_cast<DataUseUserData*>(
      request.GetUserData(DataUseUserData::kUserDataKey));
  DataUseUserData::AppState old_app_state = DataUseUserData::FOREGROUND;
  DataUseUserData::AppState new_app_state = DataUseUserData::UNKNOWN;

  if (attached_service_data)
    old_app_state = attached_service_data->app_state();

  if (old_app_state == CurrentAppState())
    new_app_state = old_app_state;

  if (attached_service_data && old_app_state != new_app_state)
    attached_service_data->set_app_state(CurrentAppState());

  RecordUMAHistogramCount(
      GetHistogramName(is_user_traffic ? "DataUse.TrafficSize.User"
                                       : "DataUse.TrafficSize.System",
                       dir, new_app_state, is_connection_cellular),
      bytes);

#if defined(OS_ANDROID)
  if (dir == DOWNSTREAM && CurrentAppState() == DataUseUserData::BACKGROUND) {
    DCHECK(!last_app_background_time_.is_null());

    const base::TimeDelta time_since_background =
        base::TimeTicks::Now() - last_app_background_time_;
    IncrementLatencyHistogramByCount(
        is_user_traffic ? "DataUse.BackgroundToDataRecievedPerByte.User"
                        : "DataUse.BackgroundToDataRecievedPerByte.System",
        time_since_background, bytes);
    if (no_reads_since_background_) {
      no_reads_since_background_ = false;
      IncrementLatencyHistogramByCount(
          is_user_traffic ? "DataUse.BackgroundToFirstDownstream.User"
                          : "DataUse.BackgroundToFirstDownstream.System",
          time_since_background, 1);
    }
  }
#endif

  bool is_tab_visible = false;

  if (is_user_traffic) {
    const DataUseRecorder* recorder = ascriber_->GetDataUseRecorder(request);
    if (recorder) {
      is_tab_visible = recorder->is_visible();
      RecordTabStateHistogram(dir, new_app_state, recorder->is_visible(),
                              bytes);
    }
  }
  if (attached_service_data && dir == DOWNSTREAM &&
      new_app_state != DataUseUserData::UNKNOWN) {
    RecordContentTypeHistogram(attached_service_data->content_type(),
                               is_user_traffic, new_app_state, is_tab_visible,
                               bytes);
  }
}

void DataUseMeasurement::UpdateDataUsePrefs(
    const net::URLRequest& request) const {
  bool is_connection_cellular =
      net::NetworkChangeNotifier::IsConnectionCellular(
          net::NetworkChangeNotifier::GetConnectionType());

  DataUseUserData* attached_service_data = static_cast<DataUseUserData*>(
      request.GetUserData(DataUseUserData::kUserDataKey));
  DataUseUserData::ServiceName service_name =
      attached_service_data ? attached_service_data->service_name()
                            : DataUseUserData::NOT_TAGGED;

  // Update data use prefs for cellular connections.
  if (!metrics_data_use_forwarder_.is_null()) {
    metrics_data_use_forwarder_.Run(
        DataUseUserData::GetServiceNameAsString(service_name),
        request.GetTotalSentBytes() + request.GetTotalReceivedBytes(),
        is_connection_cellular);
  }
}

#if defined(OS_ANDROID)
void DataUseMeasurement::OnApplicationStateChangeForTesting(
    base::android::ApplicationState application_state) {
  OnApplicationStateChange(application_state);
}
#endif

DataUseUserData::AppState DataUseMeasurement::CurrentAppState() const {
#if defined(OS_ANDROID)
  if (app_state_ != base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES)
    return DataUseUserData::BACKGROUND;
#endif
  // If the OS is not Android, all the requests are considered Foreground.
  return DataUseUserData::FOREGROUND;
}

std::string DataUseMeasurement::GetHistogramName(
    const char* prefix,
    TrafficDirection dir,
    DataUseUserData::AppState app_state,
    bool is_connection_cellular) const {
  return base::StringPrintf(
      "%s.%s.%s.%s", prefix, dir == UPSTREAM ? "Upstream" : "Downstream",
      app_state == DataUseUserData::UNKNOWN
          ? "Unknown"
          : (app_state == DataUseUserData::FOREGROUND ? "Foreground"
                                                      : "Background"),
      is_connection_cellular ? "Cellular" : "NotCellular");
}

#if defined(OS_ANDROID)
void DataUseMeasurement::OnApplicationStateChange(
    base::android::ApplicationState application_state) {
  app_state_ = application_state;
  if (app_state_ != base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    last_app_background_time_ = base::TimeTicks::Now();
    no_reads_since_background_ = true;
    MaybeRecordNetworkBytesOS();
  } else {
    last_app_background_time_ = base::TimeTicks();
  }
}

void DataUseMeasurement::MaybeRecordNetworkBytesOS() {
  // Minimum number of bytes that should be reported by the network delegate
  // before Android's TrafficStats API is queried (if Chrome is not in
  // background). This reduces the overhead of repeatedly calling the API.
  static const int64_t kMinDelegateBytes = 25000;

  if (bytes_transferred_since_last_traffic_stats_query_ < kMinDelegateBytes &&
      CurrentAppState() == DataUseUserData::FOREGROUND) {
    return;
  }
  bytes_transferred_since_last_traffic_stats_query_ = 0;
  int64_t bytes = 0;
  // Query Android traffic stats directly instead of registering with the
  // DataUseAggregator since the latter does not provide notifications for
  // the incognito traffic.
  if (net::android::traffic_stats::GetCurrentUidRxBytes(&bytes)) {
    if (rx_bytes_os_ != 0) {
      DCHECK_GE(bytes, rx_bytes_os_);
      if (bytes > rx_bytes_os_) {
        // Do not record samples with value 0.
        UMA_HISTOGRAM_COUNTS("DataUse.BytesReceived.OS", bytes - rx_bytes_os_);
      }
    }
    rx_bytes_os_ = bytes;
  }

  if (net::android::traffic_stats::GetCurrentUidTxBytes(&bytes)) {
    if (tx_bytes_os_ != 0) {
      DCHECK_GE(bytes, tx_bytes_os_);
      if (bytes > tx_bytes_os_) {
        // Do not record samples with value 0.
        UMA_HISTOGRAM_COUNTS("DataUse.BytesSent.OS", bytes - tx_bytes_os_);
      }
    }
    tx_bytes_os_ = bytes;
  }
}
#endif

void DataUseMeasurement::ReportServicesMessageSizeUMA(
    const net::URLRequest& request) {
  bool is_user_traffic = url_request_classifier_->IsUserRequest(request);
  bool is_connection_cellular =
      net::NetworkChangeNotifier::IsConnectionCellular(
          net::NetworkChangeNotifier::GetConnectionType());

  DataUseUserData* attached_service_data = static_cast<DataUseUserData*>(
      request.GetUserData(DataUseUserData::kUserDataKey));
  DataUseUserData::ServiceName service_name = DataUseUserData::NOT_TAGGED;

  if (attached_service_data)
    service_name = attached_service_data->service_name();

  if (!is_user_traffic) {
    ReportDataUsageServices(service_name, UPSTREAM, CurrentAppState(),
                            is_connection_cellular,
                            request.GetTotalSentBytes());
    ReportDataUsageServices(service_name, DOWNSTREAM, CurrentAppState(),
                            is_connection_cellular,
                            request.GetTotalReceivedBytes());
  }
}

void DataUseMeasurement::ReportDataUsageServices(
    DataUseUserData::ServiceName service,
    TrafficDirection dir,
    DataUseUserData::AppState app_state,
    bool is_connection_cellular,
    int64_t message_size) const {
  if (message_size > 0) {
    IncreaseSparseHistogramByValue(
        GetHistogramName("DataUse.MessageSize.AllServices", dir, app_state,
                         is_connection_cellular),
        service, message_size);
    if (app_state == DataUseUserData::BACKGROUND) {
      IncreaseSparseHistogramByValue("DataUse.AllServices.Background", service,
                                     message_size);
    }
  }
}

void DataUseMeasurement::RecordTabStateHistogram(
    TrafficDirection dir,
    DataUseUserData::AppState app_state,
    bool is_tab_visible,
    int64_t bytes) const {
  if (app_state == DataUseUserData::UNKNOWN)
    return;

  std::string histogram_name = "DataUse.AppTabState.";
  histogram_name.append(dir == UPSTREAM ? "Upstream." : "Downstream.");
  if (app_state == DataUseUserData::BACKGROUND) {
    histogram_name.append("AppBackground");
  } else if (is_tab_visible) {
    histogram_name.append("AppForeground.TabForeground");
  } else {
    histogram_name.append("AppForeground.TabBackground");
  }
  RecordUMAHistogramCount(histogram_name, bytes);
}

void DataUseMeasurement::RecordContentTypeHistogram(
    DataUseUserData::DataUseContentType content_type,
    bool is_user_traffic,
    DataUseUserData::AppState app_state,
    bool is_tab_visible,
    int64_t bytes) {
  if (content_type == DataUseUserData::AUDIO) {
    content_type = app_state != DataUseUserData::FOREGROUND
                       ? DataUseUserData::AUDIO_APPBACKGROUND
                       : (!is_tab_visible ? DataUseUserData::AUDIO_TABBACKGROUND
                                          : DataUseUserData::AUDIO);
  } else if (content_type == DataUseUserData::VIDEO) {
    content_type = app_state != DataUseUserData::FOREGROUND
                       ? DataUseUserData::VIDEO_APPBACKGROUND
                       : (!is_tab_visible ? DataUseUserData::VIDEO_TABBACKGROUND
                                          : DataUseUserData::VIDEO);
  }
  // Use the more primitive STATIC_HISTOGRAM_POINTER_BLOCK macro because the
  // simple UMA_HISTOGRAM_ENUMERATION macros don't expose 'AddKiB'.
  if (is_user_traffic) {
    STATIC_HISTOGRAM_POINTER_BLOCK(
        "DataUse.ContentType.UserTrafficKB", AddKiB(content_type, bytes),
        base::LinearHistogram::FactoryGet(
            "DataUse.ContentType.UserTrafficKB", 1, DataUseUserData::TYPE_MAX,
            DataUseUserData::TYPE_MAX + 1,
            base::HistogramBase::kUmaTargetedHistogramFlag));
  } else {
    STATIC_HISTOGRAM_POINTER_BLOCK(
        "DataUse.ContentType.ServicesKB", AddKiB(content_type, bytes),
        base::LinearHistogram::FactoryGet(
            "DataUse.ContentType.ServicesKB", 1, DataUseUserData::TYPE_MAX,
            DataUseUserData::TYPE_MAX + 1,
            base::HistogramBase::kUmaTargetedHistogramFlag));
  }
}

void DataUseMeasurement::RecordPageTransitionUMA(
    const net::URLRequest& request) const {
  if (!url_request_classifier_->IsUserRequest(request))
    return;

  const DataUseRecorder* recorder = ascriber_->GetDataUseRecorder(request);
  if (recorder) {
    url_request_classifier_->RecordPageTransitionUMA(
        recorder->page_transition(), request.GetTotalReceivedBytes());
  }
}

}  // namespace data_use_measurement
