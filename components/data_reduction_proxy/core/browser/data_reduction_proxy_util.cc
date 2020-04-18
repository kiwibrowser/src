// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_util.h"

#include <stdint.h>

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/version.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "components/data_reduction_proxy/core/common/lofi_decider.h"
#include "components/data_reduction_proxy/core/common/version.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/url_request/url_request.h"

#if defined(USE_GOOGLE_API_KEYS)
#include "google_apis/google_api_keys.h"
#endif

namespace data_reduction_proxy {

namespace {

#if defined(USE_GOOGLE_API_KEYS)
// Used in all Data Reduction Proxy URLs to specify API Key.
const char kApiKeyName[] = "key";
#endif

// Scales |byte_count| by the ratio of |numerator|:|denomenator|.
int64_t ScaleByteCountByRatio(int64_t byte_count,
                              int64_t numerator,
                              int64_t denomenator) {
  DCHECK_LE(0, byte_count);
  DCHECK_LE(0, numerator);
  DCHECK_LT(0, denomenator);

  // As an optimization, use integer arithmetic if it won't overflow.
  if (byte_count <= std::numeric_limits<int32_t>::max() &&
      numerator <= std::numeric_limits<int32_t>::max()) {
    return byte_count * numerator / denomenator;
  }

  double scaled_byte_count = static_cast<double>(byte_count) *
                             static_cast<double>(numerator) /
                             static_cast<double>(denomenator);
  if (scaled_byte_count >
      static_cast<double>(std::numeric_limits<int64_t>::max())) {
    // If this ever triggers, then byte counts can no longer be safely stored in
    // 64-bit ints.
    NOTREACHED();
    return byte_count;
  }
  return static_cast<int64_t>(scaled_byte_count);
}

// Estimate the size of the original headers of |request|. If |used_drp| is
// true, then it's assumed that the original request would have used HTTP/1.1,
// otherwise it assumes that the original request would have used the same
// protocol as |request| did. This is to account for stuff like HTTP/2 header
// compression.
int64_t EstimateOriginalHeaderBytes(const net::URLRequest& request,
                                    const LoFiDecider* lofi_decider) {
  // If this is an auto-reload of an image, then this request would ordinarily
  // not be issued, so return 0.
  if (lofi_decider && lofi_decider->IsClientLoFiAutoReloadRequest(request))
    return 0;
  data_reduction_proxy::DataReductionProxyData* data =
      data_reduction_proxy::DataReductionProxyData::GetData(request);
  if (data && data->used_data_reduction_proxy()) {
    // TODO(sclittle): Remove headers added by Data Reduction Proxy when
    // computing original size. https://crbug.com/535701.
    return request.response_headers()->raw_headers().size();
  }
  return std::max<int64_t>(0, request.GetTotalReceivedBytes() -
                                  request.received_response_content_length());
}

}  // namespace

namespace util {

const char* ChromiumVersion() {
  // Assert at compile time that the Chromium version is at least somewhat
  // properly formed, e.g. the version string is at least as long as "0.0.0.0",
  // and starts and ends with numeric digits. This is to prevent another
  // regression like http://crbug.com/595471.
  static_assert(arraysize(PRODUCT_VERSION) >= arraysize("0.0.0.0") &&
                    '0' <= PRODUCT_VERSION[0] && PRODUCT_VERSION[0] <= '9' &&
                    '0' <= PRODUCT_VERSION[arraysize(PRODUCT_VERSION) - 2] &&
                    PRODUCT_VERSION[arraysize(PRODUCT_VERSION) - 2] <= '9',
                "PRODUCT_VERSION must be a string of the form "
                "'MAJOR.MINOR.BUILD.PATCH', e.g. '1.2.3.4'. "
                "PRODUCT_VERSION='" PRODUCT_VERSION "' is badly formed.");

  return PRODUCT_VERSION;
}

void GetChromiumBuildAndPatch(const std::string& version_string,
                              std::string* build,
                              std::string* patch) {
  uint32_t build_number;
  uint32_t patch_number;
  GetChromiumBuildAndPatchAsInts(version_string, &build_number, &patch_number);
  *build = base::NumberToString(build_number);
  *patch = base::NumberToString(patch_number);
}

void GetChromiumBuildAndPatchAsInts(const std::string& version_string,
                                    uint32_t* build,
                                    uint32_t* patch) {
  base::Version version(version_string);
  DCHECK(version.IsValid());
  DCHECK_EQ(4U, version.components().size());
  *build = version.components()[2];
  *patch = version.components()[3];
}

const char* GetStringForClient(Client client) {
  switch (client) {
    case Client::UNKNOWN:
      return "";
    case Client::CRONET_ANDROID:
      return "cronet";
    case Client::WEBVIEW_ANDROID:
      return "webview";
    case Client::CHROME_ANDROID:
      return "android";
    case Client::CHROME_IOS:
      return "ios";
    case Client::CHROME_MAC:
      return "mac";
    case Client::CHROME_CHROMEOS:
      return "chromeos";
    case Client::CHROME_LINUX:
      return "linux";
    case Client::CHROME_WINDOWS:
      return "win";
    case Client::CHROME_FREEBSD:
      return "freebsd";
    case Client::CHROME_OPENBSD:
      return "openbsd";
    case Client::CHROME_SOLARIS:
      return "solaris";
    case Client::CHROME_QNX:
      return "qnx";
    default:
      NOTREACHED();
      return "";
  }
}

GURL AddApiKeyToUrl(const GURL& url) {
  GURL new_url = url;
#if defined(USE_GOOGLE_API_KEYS)
  std::string api_key = google_apis::GetAPIKey();
  if (google_apis::HasKeysConfigured() && !api_key.empty()) {
    new_url = net::AppendOrReplaceQueryParameter(url, kApiKeyName, api_key);
  }
#endif
  return net::AppendOrReplaceQueryParameter(new_url, "alt", "proto");
}

bool EligibleForDataReductionProxy(const net::ProxyInfo& proxy_info,
                                   const GURL& url,
                                   const std::string& method) {
  return proxy_info.is_direct() && proxy_info.proxy_list().size() == 1 &&
         !url.SchemeIsWSOrWSS() && net::HttpUtil::IsMethodIdempotent(method);
}

bool ApplyProxyConfigToProxyInfo(const net::ProxyConfig& proxy_config,
                                 const net::ProxyRetryInfoMap& proxy_retry_info,
                                 const GURL& url,
                                 net::ProxyInfo* data_reduction_proxy_info) {
  DCHECK(data_reduction_proxy_info);
  if (proxy_config.proxy_rules().empty())
    return false;
  proxy_config.proxy_rules().Apply(url, data_reduction_proxy_info);
  data_reduction_proxy_info->DeprioritizeBadProxies(proxy_retry_info);
  return !data_reduction_proxy_info->proxy_server().is_direct();
}

int64_t CalculateOCLFromOFCL(const net::URLRequest& request) {
  const net::HttpResponseHeaders* response_headers = request.response_headers();
  if (!response_headers)
    return request.received_response_content_length();

  int64_t original_content_length = GetDataReductionProxyOFCL(response_headers);

  if (response_headers->response_code() == net::HTTP_PARTIAL_CONTENT) {
    int64_t first, last, range_content_length;
    if (response_headers->GetContentRangeFor206(&first, &last,
                                                &range_content_length) &&
        range_content_length > 0 && original_content_length > 0) {
      // For a range request, OFCL indicates the original content length of the
      // entire resource. The received response content length should be scaled
      // by the compression ratio given by OFCL / range_content_length.
      original_content_length =
          ScaleByteCountByRatio(request.received_response_content_length(),
                                original_content_length, range_content_length);
    }
  }
  return original_content_length;
}

int64_t EstimateOriginalBodySize(const net::URLRequest& request,
                                 const LoFiDecider* lofi_decider) {
  if (lofi_decider) {
    // If this is an auto-reload of an image, then this request would ordinarily
    // not be issued, so return 0.
    if (lofi_decider->IsClientLoFiAutoReloadRequest(request))
      return 0;

    int64_t first, last, length;
    if (!request.was_cached() &&
        lofi_decider->IsClientLoFiImageRequest(request) &&
        request.response_headers() &&
        request.response_headers()->GetContentRangeFor206(&first, &last,
                                                          &length) &&
        length > request.received_response_content_length()) {
      return length;
    }
  }

  data_reduction_proxy::DataReductionProxyData* data =
      data_reduction_proxy::DataReductionProxyData::GetData(request);
  if (!data || !data->used_data_reduction_proxy() || request.was_cached() ||
      !request.response_headers()) {
    return std::min<int64_t>(request.GetTotalReceivedBytes(),
                             request.received_response_content_length());
  }

  int64_t original_content_length_from_header = CalculateOCLFromOFCL(request);

  if (original_content_length_from_header < 0)
    return request.received_response_content_length();
  if (request.status().error() == net::OK)
    return original_content_length_from_header;

  int64_t content_length_from_header =
      request.response_headers()->GetContentLength();

  if (content_length_from_header < 0)
    return request.received_response_content_length();
  if (content_length_from_header == 0)
    return original_content_length_from_header;

  return ScaleByteCountByRatio(request.received_response_content_length(),
                               original_content_length_from_header,
                               content_length_from_header);
}

int64_t EstimateOriginalReceivedBytes(const net::URLRequest& request,
                                      const LoFiDecider* lofi_decider) {
  if (request.was_cached() || !request.response_headers())
    return request.GetTotalReceivedBytes();

  return EstimateOriginalHeaderBytes(request, lofi_decider) +
         EstimateOriginalBodySize(request, lofi_decider);
}

ProxyScheme ConvertNetProxySchemeToProxyScheme(
    net::ProxyServer::Scheme scheme) {
  switch (scheme) {
    case net::ProxyServer::SCHEME_HTTP:
      return PROXY_SCHEME_HTTP;
    case net::ProxyServer::SCHEME_HTTPS:
      return PROXY_SCHEME_HTTPS;
    case net::ProxyServer::SCHEME_QUIC:
      return PROXY_SCHEME_QUIC;
    case net::ProxyServer::SCHEME_DIRECT:
      return PROXY_SCHEME_DIRECT;
    default:
      NOTREACHED() << scheme;
      return PROXY_SCHEME_UNKNOWN;
  }
}

}  // namespace util

namespace protobuf_parser {

PageloadMetrics_EffectiveConnectionType
ProtoEffectiveConnectionTypeFromEffectiveConnectionType(
    net::EffectiveConnectionType effective_connection_type) {
  switch (effective_connection_type) {
    case net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN:
      return PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_UNKNOWN;
    case net::EFFECTIVE_CONNECTION_TYPE_OFFLINE:
      return PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_OFFLINE;
    case net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G:
      return PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_SLOW_2G;
    case net::EFFECTIVE_CONNECTION_TYPE_2G:
      return PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_2G;
    case net::EFFECTIVE_CONNECTION_TYPE_3G:
      return PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_3G;
    case net::EFFECTIVE_CONNECTION_TYPE_4G:
      return PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_4G;
    default:
      NOTREACHED();
      return PageloadMetrics_EffectiveConnectionType_EFFECTIVE_CONNECTION_TYPE_UNKNOWN;
  }
}

PageloadMetrics_ConnectionType ProtoConnectionTypeFromConnectionType(
    net::NetworkChangeNotifier::ConnectionType connection_type) {
  switch (connection_type) {
    case net::NetworkChangeNotifier::CONNECTION_UNKNOWN:
      return PageloadMetrics_ConnectionType_CONNECTION_UNKNOWN;
    case net::NetworkChangeNotifier::CONNECTION_ETHERNET:
      return PageloadMetrics_ConnectionType_CONNECTION_ETHERNET;
    case net::NetworkChangeNotifier::CONNECTION_WIFI:
      return PageloadMetrics_ConnectionType_CONNECTION_WIFI;
    case net::NetworkChangeNotifier::CONNECTION_2G:
      return PageloadMetrics_ConnectionType_CONNECTION_2G;
    case net::NetworkChangeNotifier::CONNECTION_3G:
      return PageloadMetrics_ConnectionType_CONNECTION_3G;
    case net::NetworkChangeNotifier::CONNECTION_4G:
      return PageloadMetrics_ConnectionType_CONNECTION_4G;
    case net::NetworkChangeNotifier::CONNECTION_NONE:
      return PageloadMetrics_ConnectionType_CONNECTION_NONE;
    case net::NetworkChangeNotifier::CONNECTION_BLUETOOTH:
      return PageloadMetrics_ConnectionType_CONNECTION_BLUETOOTH;
  }
}

net::ProxyServer::Scheme SchemeFromProxyScheme(
    ProxyServer_ProxyScheme proxy_scheme) {
  switch (proxy_scheme) {
    case ProxyServer_ProxyScheme_HTTP:
      return net::ProxyServer::SCHEME_HTTP;
    case ProxyServer_ProxyScheme_HTTPS:
      return net::ProxyServer::SCHEME_HTTPS;
    default:
      return net::ProxyServer::SCHEME_INVALID;
  }
}

ProxyServer_ProxyScheme ProxySchemeFromScheme(net::ProxyServer::Scheme scheme) {
  switch (scheme) {
    case net::ProxyServer::SCHEME_HTTP:
      return ProxyServer_ProxyScheme_HTTP;
    case net::ProxyServer::SCHEME_HTTPS:
      return ProxyServer_ProxyScheme_HTTPS;
    default:
      return ProxyServer_ProxyScheme_UNSPECIFIED;
  }
}

void TimeDeltaToDuration(const base::TimeDelta& time_delta,
                         Duration* duration) {
  duration->set_seconds(time_delta.InSeconds());
  base::TimeDelta partial_seconds =
      time_delta - base::TimeDelta::FromSeconds(time_delta.InSeconds());
  duration->set_nanos(partial_seconds.InMicroseconds() *
                      base::Time::kNanosecondsPerMicrosecond);
}

base::TimeDelta DurationToTimeDelta(const Duration& duration) {
  return base::TimeDelta::FromSeconds(duration.seconds()) +
         base::TimeDelta::FromMicroseconds(
             duration.nanos() / base::Time::kNanosecondsPerMicrosecond);
}

void TimeToTimestamp(const base::Time& time, Timestamp* timestamp) {
  timestamp->set_seconds((time - base::Time::UnixEpoch()).InSeconds());
  timestamp->set_nanos(((time - base::Time::UnixEpoch()).InMicroseconds() %
                        base::Time::kMicrosecondsPerSecond) *
                       base::Time::kNanosecondsPerMicrosecond);
}

base::Time TimestampToTime(const Timestamp& timestamp) {
  base::Time t = base::Time::UnixEpoch();
  t += base::TimeDelta::FromSeconds(timestamp.seconds());
  t += base::TimeDelta::FromMicroseconds(
      timestamp.nanos() / base::Time::kNanosecondsPerMicrosecond);
  return t;
}

std::unique_ptr<Duration> CreateDurationFromTimeDelta(
    const base::TimeDelta& time_delta) {
  std::unique_ptr<Duration> duration(new Duration);
  TimeDeltaToDuration(time_delta, duration.get());
  return duration;
}

std::unique_ptr<Timestamp> CreateTimestampFromTime(const base::Time& time) {
  std::unique_ptr<Timestamp> timestamp(new Timestamp);
  TimeToTimestamp(time, timestamp.get());
  return timestamp;
}

}  // namespace protobuf_parser

}  // namespace data_reduction_proxy
