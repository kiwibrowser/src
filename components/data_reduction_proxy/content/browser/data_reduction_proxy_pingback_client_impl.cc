// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/content/browser/data_reduction_proxy_pingback_client_impl.h"

#include <stdint.h>
#include <string>

#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/optional.h"
#include "base/rand_util.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/time/time.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_util.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_page_load_timing.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/data_reduction_proxy/proto/client_config.pb.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/variations/net/variations_http_headers.h"
#include "content/public/common/child_process_host.h"
#include "net/base/load_flags.h"
#include "net/nqe/effective_connection_type.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace data_reduction_proxy {

namespace {

static const char kHistogramSucceeded[] =
    "DataReductionProxy.Pingback.Succeeded";
static const char kHistogramAttempted[] =
    "DataReductionProxy.Pingback.Attempted";
static const char kHistogramCrash[] = "DataReductionProxy.Pingback.CrashAction";

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class CrashAction {
  // A crash was detected.
  kDetected = 0,
  // The crash dump was analyzed and the information was queued to be sent.
  kAnalsisSucceeded = 1,
  // The crash dump was not analyzed successfully, but the information was
  // queued to be sent.
  kAnalysisFailed = 2,
  // The crash dump was not even attempted to be analyzed, but the information
  // was queued to be sent.
  kNotAnalyzed = 3,
  // The crash dump was successfully sent to the server.
  kSentSuccessfully = 4,
  // The crash dump request completed unsuccessfully.
  kSendUnuccessful = 5,
  kLast = kSendUnuccessful + 1,
};

// Adds the relevant information to |request| for this page load based on page
// timing and data reduction proxy state.
void AddDataToPageloadMetrics(const DataReductionProxyData& request_data,
                              const DataReductionProxyPageLoadTiming& timing,
                              PageloadMetrics_RendererCrashType crash_type,
                              PageloadMetrics* request) {
  request->set_session_key(request_data.session_key());
  request->set_holdback_group(params::HoldbackFieldTrialGroup());
  // For the timing events, any of them could be zero. Fill the message as a
  // best effort.
  request->set_allocated_first_request_time(
      protobuf_parser::CreateTimestampFromTime(timing.navigation_start)
          .release());
  if (request_data.request_url().is_valid())
    request->set_first_request_url(request_data.request_url().spec());
  if (timing.first_contentful_paint) {
    request->set_allocated_time_to_first_contentful_paint(
        protobuf_parser::CreateDurationFromTimeDelta(
            timing.first_contentful_paint.value())
            .release());
  }
  if (timing.experimental_first_meaningful_paint) {
    request->set_allocated_experimental_time_to_first_meaningful_paint(
        protobuf_parser::CreateDurationFromTimeDelta(
            timing.experimental_first_meaningful_paint.value())
            .release());
  }
  if (timing.first_image_paint) {
    request->set_allocated_time_to_first_image_paint(
        protobuf_parser::CreateDurationFromTimeDelta(
            timing.first_image_paint.value())
            .release());
  }
  if (timing.response_start) {
    request->set_allocated_time_to_first_byte(
        protobuf_parser::CreateDurationFromTimeDelta(
            timing.response_start.value())
            .release());
  }
  if (timing.load_event_start) {
    request->set_allocated_page_load_time(
        protobuf_parser::CreateDurationFromTimeDelta(
            timing.load_event_start.value())
            .release());
  }
  if (timing.first_input_delay) {
    request->set_allocated_first_input_delay(
        protobuf_parser::CreateDurationFromTimeDelta(
            timing.first_input_delay.value())
            .release());
  }
  if (timing.parse_blocked_on_script_load_duration) {
    request->set_allocated_parse_blocked_on_script_load_duration(
        protobuf_parser::CreateDurationFromTimeDelta(
            timing.parse_blocked_on_script_load_duration.value())
            .release());
  }
  if (timing.parse_stop) {
    request->set_allocated_parse_stop(
        protobuf_parser::CreateDurationFromTimeDelta(timing.parse_stop.value())
            .release());
  }

  request->set_effective_connection_type(
      protobuf_parser::ProtoEffectiveConnectionTypeFromEffectiveConnectionType(
          request_data.effective_connection_type()));
  request->set_connection_type(
      protobuf_parser::ProtoConnectionTypeFromConnectionType(
          request_data.connection_type()));
  request->set_compressed_page_size_bytes(timing.network_bytes);
  request->set_original_page_size_bytes(timing.original_network_bytes);
  request->set_total_page_size_bytes(timing.total_page_size_bytes);
  request->set_cached_fraction(timing.cached_fraction);
  request->set_renderer_memory_usage_kb(timing.renderer_memory_usage_kb);

  request->set_renderer_crash_type(crash_type);

  if (request_data.page_id()) {
    request->set_page_id(request_data.page_id().value());
  }

  bool was_preview_shown = false;
  if (request_data.lofi_received() || request_data.client_lofi_requested()) {
    request->set_previews_type(PageloadMetrics_PreviewsType_LOFI);
    was_preview_shown = true;
  } else if (request_data.lite_page_received()) {
    request->set_previews_type(PageloadMetrics_PreviewsType_LITE_PAGE);
    was_preview_shown = true;
  } else {
    request->set_previews_type(PageloadMetrics_PreviewsType_NONE);
  }

  // Only report opt out information if a server preview was shown (otherwise,
  // report opt out unknown). Similarly, if app background (Android) caused this
  // report to be sent before the page load is terminated, do not report opt out
  // information as the user could reload the original preview after this report
  // is sent.
  if (!was_preview_shown || timing.app_background_occurred) {
    request->set_previews_opt_out(PageloadMetrics_PreviewsOptOut_UNKNOWN);
    return;
  }

  if (timing.opt_out_occurred) {
    request->set_previews_opt_out(PageloadMetrics_PreviewsOptOut_OPT_OUT);
    return;
  }
  request->set_previews_opt_out(PageloadMetrics_PreviewsOptOut_NON_OPT_OUT);
}

// Adds |current_time| as the metrics sent time to |request_data|, and returns
// the serialized request.
std::string AddBatchInfoAndSerializeRequest(
    RecordPageloadMetricsRequest* request_data,
    base::Time current_time) {
  request_data->set_allocated_metrics_sent_time(
      protobuf_parser::CreateTimestampFromTime(current_time).release());
  data_reduction_proxy::PageloadDeviceInfo* device_info =
      request_data->mutable_device_info();
  device_info->set_total_device_memory_kb(
      base::SysInfo::AmountOfPhysicalMemory() / 1024);
  std::string serialized_request;
  request_data->SerializeToString(&serialized_request);
  return serialized_request;
}

}  // namespace

DataReductionProxyPingbackClientImpl::DataReductionProxyPingbackClientImpl(
    net::URLRequestContextGetter* url_request_context,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner)
    : url_request_context_(url_request_context),
      pingback_url_(util::AddApiKeyToUrl(params::GetPingbackURL())),
      pingback_reporting_fraction_(0.0),
      current_fetcher_message_count_(0u),
      current_fetcher_crash_count_(0u),
      ui_task_runner_(std::move(ui_task_runner)),
#if defined(OS_ANDROID)
      scoped_observer_(this),
      weak_factory_(this) {
  auto* crash_manager = breakpad::CrashDumpManager::GetInstance();
  DCHECK(crash_manager);
  scoped_observer_.Add(crash_manager);
#else
      weak_factory_(this){
#endif
}

DataReductionProxyPingbackClientImpl::~DataReductionProxyPingbackClientImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void DataReductionProxyPingbackClientImpl::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(source == current_fetcher_.get());
  // For each message in the batched message, we should report UMA.
  // Historically, batched requests are not common, so this loop usually only
  // has 1 iteration.
  for (size_t message = 0u; message < current_fetcher_message_count_;
       ++message) {
    UMA_HISTOGRAM_BOOLEAN(kHistogramSucceeded,
                          source->GetStatus().is_success());
  }

  // For each crash we should report UMA.
  for (size_t crash = 0u; crash < current_fetcher_crash_count_; ++crash) {
    UMA_HISTOGRAM_ENUMERATION(
        kHistogramCrash,
        (source->GetStatus().is_success() ? CrashAction::kSentSuccessfully
                                          : CrashAction::kSendUnuccessful),
        CrashAction::kLast);
  }

  current_fetcher_.reset();
  if (metrics_request_.pageloads_size() > 0) {
    CreateFetcherForDataAndStart();
  }
}

void DataReductionProxyPingbackClientImpl::SendPingback(
    const DataReductionProxyData& request_data,
    const DataReductionProxyPageLoadTiming& timing) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool send_pingback = ShouldSendPingback();
  UMA_HISTOGRAM_BOOLEAN(kHistogramAttempted, send_pingback);
  if (!send_pingback)
    return;

  if (timing.host_id != content::ChildProcessHost::kInvalidUniqueID) {
    UMA_HISTOGRAM_ENUMERATION(kHistogramCrash, CrashAction::kDetected,
                              CrashAction::kLast);
#if defined(OS_ANDROID)
    // Defer sending the report until the crash is processed.
    AddRequestToCrashMap(request_data, timing);
#else
    // Don't analyze non-Android crashes.
    UMA_HISTOGRAM_ENUMERATION(kHistogramCrash, CrashAction::kNotAnalyzed,
                              CrashAction::kLast);
    CreateReport(request_data, timing,
                 PageloadMetrics_RendererCrashType_NOT_ANALYZED);
#endif
    return;
  }
  CreateReport(request_data, timing,
               PageloadMetrics_RendererCrashType_NO_CRASH);
}

#if defined(OS_ANDROID)
void DataReductionProxyPingbackClientImpl::OnCrashDumpProcessed(
    const breakpad::CrashDumpManager::CrashDumpDetails& details) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto iter = crash_map_.find(details.process_host_id);
  if (iter == crash_map_.end())
    return;
  const CrashPageLoadInformation& crash_page_load_information = iter->second;

  UMA_HISTOGRAM_ENUMERATION(kHistogramCrash, CrashAction::kAnalsisSucceeded,
                            CrashAction::kLast);

  bool renderer_foreground_oom =
      breakpad::CrashDumpManager::IsForegroundOom(details);
  CreateReport(std::get<0>(crash_page_load_information),
               std::get<1>(crash_page_load_information),
               renderer_foreground_oom
                   ? PageloadMetrics_RendererCrashType_ANDROID_FOREGROUND_OOM
                   : PageloadMetrics_RendererCrashType_OTHER_CRASH);
  crash_map_.erase(iter);
}

void DataReductionProxyPingbackClientImpl::AddRequestToCrashMap(
    const DataReductionProxyData& request_data,
    const DataReductionProxyPageLoadTiming& timing) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // It is guaranteed that |AddRequestToCrashMap| is called before
  // |OnCrashDumpProcessed| due to the nature of both events being triggered
  // from the channel closing, and SendPingback being called on the same stack,
  // while OnCrashDumpProcessed is called from a PostTask.
  crash_map_.insert(
      std::make_pair(timing.host_id, std::make_tuple(request_data, timing)));
  // If the crash hasn't been processed in 5 seconds, send the report without it
  // being analyzed. 5 seconds should be enough time for breakpad to process the
  // dump, while being short enough that the user will probably not shutdown the
  // app.
  ui_task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&DataReductionProxyPingbackClientImpl::RemoveFromCrashMap,
                     weak_factory_.GetWeakPtr(), timing.host_id),
      base::TimeDelta::FromSeconds(5));
}

void DataReductionProxyPingbackClientImpl::RemoveFromCrashMap(
    int process_host_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto iter = crash_map_.find(process_host_id);
  if (iter == crash_map_.end())
    return;
  const CrashPageLoadInformation& crash_page_load_information = iter->second;

  UMA_HISTOGRAM_ENUMERATION(kHistogramCrash, CrashAction::kAnalysisFailed,
                            CrashAction::kLast);

  CreateReport(std::get<0>(crash_page_load_information),
               std::get<1>(crash_page_load_information),
               PageloadMetrics_RendererCrashType_NOT_ANALYZED);
  crash_map_.erase(iter);
}

#endif

void DataReductionProxyPingbackClientImpl::CreateReport(
    const DataReductionProxyData& request_data,
    const DataReductionProxyPageLoadTiming& timing,
    PageloadMetrics_RendererCrashType crash_type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  PageloadMetrics* pageload_metrics = metrics_request_.add_pageloads();
  AddDataToPageloadMetrics(request_data, timing, crash_type, pageload_metrics);
  if (current_fetcher_)
    return;
  DCHECK_EQ(1, metrics_request_.pageloads_size());
  CreateFetcherForDataAndStart();
}

void DataReductionProxyPingbackClientImpl::CreateFetcherForDataAndStart() {
  DCHECK(!current_fetcher_);
  DCHECK_GE(metrics_request_.pageloads_size(), 1);
  std::string serialized_request =
      AddBatchInfoAndSerializeRequest(&metrics_request_, CurrentTime());

  current_fetcher_message_count_ = metrics_request_.pageloads_size();
  current_fetcher_crash_count_ = 0u;
  for (const auto& iter : metrics_request_.pageloads()) {
    if (iter.renderer_crash_type() !=
        PageloadMetrics_RendererCrashType_NO_CRASH) {
      ++current_fetcher_crash_count_;
    }
  }

  metrics_request_.Clear();
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("data_reduction_proxy_pingback", R"(
        semantics {
          sender: "Data Reduction Proxy"
          description:
            "Sends page performance and data efficiency metrics to the data "
            "reduction proxy."
          trigger:
            "Sent after a page load, if the page was loaded via the data "
            "reduction proxy."
          data:
            "URL, request time, response time, page size, connection type, and "
            "performance measures. See the following for details: "
            "components/data_reduction_proxy/proto/pageload_metrics.proto"
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can control Data Saver on Android via 'Data Saver' setting. "
            "Data Saver is not available on iOS, and on desktop it is enabled "
            "by insalling the Data Saver extension. While Data Saver is "
            "enabled, this feature cannot be disabled by settings."
          policy_exception_justification: "Not implemented."
        })");
  current_fetcher_ = net::URLFetcher::Create(
      pingback_url_, net::URLFetcher::POST, this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      current_fetcher_.get(),
      data_use_measurement::DataUseUserData::DATA_REDUCTION_PROXY);
  current_fetcher_->SetLoadFlags(net::LOAD_BYPASS_PROXY |
                                 net::LOAD_DO_NOT_SEND_COOKIES |
                                 net::LOAD_DO_NOT_SAVE_COOKIES);
  current_fetcher_->SetUploadData("application/x-protobuf", serialized_request);
  current_fetcher_->SetRequestContext(url_request_context_);
  // |current_fetcher_| should not retry on 5xx errors since the server may
  // already be overloaded.
  static const int kMaxRetries = 5;
  current_fetcher_->SetAutomaticallyRetryOnNetworkChanges(kMaxRetries);

  // Attach variations headers.
  net::HttpRequestHeaders headers;
  variations::AppendVariationHeaders(pingback_url_,
                                     variations::InIncognito::kNo,
                                     variations::SignedIn::kNo, &headers);
  if (!headers.IsEmpty())
    current_fetcher_->SetExtraRequestHeaders(headers.ToString());

  current_fetcher_->Start();
}

bool DataReductionProxyPingbackClientImpl::ShouldSendPingback() const {
  return params::IsForcePingbackEnabledViaFlags() ||
         GenerateRandomFloat() < pingback_reporting_fraction_;
}

base::Time DataReductionProxyPingbackClientImpl::CurrentTime() const {
  return base::Time::Now();
}

float DataReductionProxyPingbackClientImpl::GenerateRandomFloat() const {
  return static_cast<float>(base::RandDouble());
}

void DataReductionProxyPingbackClientImpl::SetPingbackReportingFraction(
    float pingback_reporting_fraction) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_LE(0.0f, pingback_reporting_fraction);
  DCHECK_GE(1.0f, pingback_reporting_fraction);
  pingback_reporting_fraction_ = pingback_reporting_fraction;
}

}  // namespace data_reduction_proxy
