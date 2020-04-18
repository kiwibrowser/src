// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_bypass_protocol.h"

#include <vector>

#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_bypass_stats.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_server.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_type_info.h"
#include "net/base/load_flags.h"
#include "net/base/proxy_server.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/proxy_resolution/proxy_list.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/proxy_resolution/proxy_retry_info.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace data_reduction_proxy {

namespace {

// Adds the Data Reduction Proxy servers in |proxy_type_info| that should be
// marked bad according to |data_reduction_proxy_info| to the retry map
// maintained by the proxy resolution service of the |request|.
void MarkProxiesAsBad(const net::URLRequest& request,
                      const DataReductionProxyInfo& data_reduction_proxy_info,
                      const DataReductionProxyTypeInfo& proxy_type_info) {
  DCHECK_GT(proxy_type_info.proxy_servers.size(), proxy_type_info.proxy_index);

  // Synthesize a suitable |ProxyInfo| to add the proxies to the
  // |ProxyRetryInfoMap| of the proxy service.
  net::ProxyList proxy_list;

  const size_t bad_proxy_end_index = data_reduction_proxy_info.bypass_all
                                         ? proxy_type_info.proxy_servers.size()
                                         : proxy_type_info.proxy_index + 1U;

  for (size_t i = proxy_type_info.proxy_index; i < bad_proxy_end_index; ++i) {
    const net::ProxyServer& bad_proxy =
        proxy_type_info.proxy_servers[i].proxy_server();
    DCHECK(bad_proxy.is_valid());
    DCHECK(!bad_proxy.is_direct());
    proxy_list.AddProxyServer(bad_proxy);
  }
  std::vector<net::ProxyServer> bad_proxies = proxy_list.GetAll();
  proxy_list.AddProxyServer(net::ProxyServer::Direct());

  net::ProxyInfo proxy_info;
  proxy_info.UseProxyList(proxy_list);

  request.context()->proxy_resolution_service()->MarkProxiesAsBadUntil(
      proxy_info, data_reduction_proxy_info.bypass_duration, bad_proxies,
      request.net_log());
}

}  // namespace

DataReductionProxyBypassProtocol::DataReductionProxyBypassProtocol(
    DataReductionProxyConfig* config)
    : config_(config) {
  DCHECK(config_);
}

DataReductionProxyBypassProtocol::~DataReductionProxyBypassProtocol() {
}

bool DataReductionProxyBypassProtocol::MaybeBypassProxyAndPrepareToRetry(
    net::URLRequest* request,
    DataReductionProxyBypassType* proxy_bypass_type,
    DataReductionProxyInfo* data_reduction_proxy_info) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(request);

  DataReductionProxyBypassType bypass_type;

  const net::HttpResponseHeaders* response_headers =
      request->response_info().headers.get();
  bool retry;
  if (!response_headers) {
    retry = HandleInvalidResponseHeadersCase(
        *request, data_reduction_proxy_info, &bypass_type);
  } else {
    retry = HandleValidResponseHeadersCase(
        *request, proxy_bypass_type, data_reduction_proxy_info, &bypass_type);
  }
  if (!retry)
    return false;

  if (data_reduction_proxy_info->mark_proxies_as_bad) {
    base::Optional<DataReductionProxyTypeInfo> proxy_type_info =
        config_->FindConfiguredDataReductionProxy(request->proxy_server());
    DCHECK(proxy_type_info);
    MarkProxiesAsBad(*request, *data_reduction_proxy_info, *proxy_type_info);
  } else {
    request->SetLoadFlags(request->load_flags() | net::LOAD_BYPASS_CACHE |
                          net::LOAD_BYPASS_PROXY);
  }

  return bypass_type == BYPASS_EVENT_TYPE_CURRENT || !response_headers ||
         net::HttpUtil::IsMethodIdempotent(request->method());
}

bool DataReductionProxyBypassProtocol::HandleInvalidResponseHeadersCase(
    const net::URLRequest& request,
    DataReductionProxyInfo* data_reduction_proxy_info,
    DataReductionProxyBypassType* bypass_type) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(nullptr, request.response_info().headers.get());

  // Handling of invalid response headers is enabled by default.
  if (!GetFieldTrialParamByFeatureAsBool(
          features::kDataReductionProxyRobustConnection,
          "handle_invalid_respnse_headers", true)) {
    return false;
  }

  if (!config_->FindConfiguredDataReductionProxy(request.proxy_server()))
    return false;

  DCHECK(request.url().SchemeIs(url::kHttpScheme));

  net::URLRequestStatus status = request.status();
  DCHECK(!status.is_success());
  DCHECK_NE(net::OK, status.error());
  if (status.error() == net::ERR_IO_PENDING ||
      status.error() == net::ERR_NETWORK_CHANGED ||
      status.error() == net::ERR_INTERNET_DISCONNECTED ||
      status.error() == net::ERR_NETWORK_IO_SUSPENDED ||
      status.error() == net::ERR_ABORTED ||
      status.error() == net::ERR_INSUFFICIENT_RESOURCES ||
      status.error() == net::ERR_OUT_OF_MEMORY ||
      status.error() == net::ERR_NAME_NOT_RESOLVED ||
      status.error() == net::ERR_NAME_RESOLUTION_FAILED ||
      status.error() == net::ERR_ADDRESS_UNREACHABLE ||
      std::abs(status.error()) >= 400) {
    // No need to retry the request or mark the proxy as bad. Only bypass on
    // System related errors, connection related errors and certificate errors.
    return false;
  }

  static_assert(
      net::ERR_CONNECTION_RESET > -400 && net::ERR_SSL_PROTOCOL_ERROR > -400,
      "net error is not handled");

  base::UmaHistogramSparse(
      "DataReductionProxy.InvalidResponseHeadersReceived.NetError",
      -status.error());

  data_reduction_proxy_info->bypass_all = false;
  data_reduction_proxy_info->mark_proxies_as_bad = true;
  data_reduction_proxy_info->bypass_duration = base::TimeDelta::FromMinutes(5);
  data_reduction_proxy_info->bypass_action = BYPASS_ACTION_TYPE_BYPASS;
  *bypass_type = BYPASS_EVENT_TYPE_MEDIUM;

  return true;
}

bool DataReductionProxyBypassProtocol::HandleValidResponseHeadersCase(
    const net::URLRequest& request,
    DataReductionProxyBypassType* proxy_bypass_type,
    DataReductionProxyInfo* data_reduction_proxy_info,
    DataReductionProxyBypassType* bypass_type) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Empty implies either that the request was served from cache or that
  // request was served directly from the origin.
  const net::HttpResponseHeaders* response_headers =
      request.response_info().headers.get();

  DCHECK(response_headers);

  base::Optional<DataReductionProxyTypeInfo> data_reduction_proxy_type_info =
      config_->FindConfiguredDataReductionProxy(request.proxy_server());
  if (!data_reduction_proxy_type_info)
    return false;

  // At this point, the response is expected to have the data reduction proxy
  // via header, so detect and report cases where the via header is missing.
  DataReductionProxyBypassStats::DetectAndRecordMissingViaHeaderResponseCode(
      data_reduction_proxy_type_info->proxy_index == 0U, *response_headers);

  // GetDataReductionProxyBypassType will only log a net_log event if a bypass
  // command was sent via the data reduction proxy headers.
  *bypass_type = GetDataReductionProxyBypassType(
      request.url_chain(), *response_headers, data_reduction_proxy_info);

  if (proxy_bypass_type)
    *proxy_bypass_type = *bypass_type;
  if (*bypass_type == BYPASS_EVENT_TYPE_MAX)
    return false;

  DCHECK(request.context());
  DCHECK(request.context()->proxy_resolution_service());
  DCHECK_GT(data_reduction_proxy_type_info->proxy_servers.size(),
            data_reduction_proxy_type_info->proxy_index);

  const net::ProxyServer& proxy_server =
      data_reduction_proxy_type_info
          ->proxy_servers[data_reduction_proxy_type_info->proxy_index]
          .proxy_server();

  // Only record UMA if the proxy isn't already on the retry list.
  if (!config_->IsProxyBypassed(
          request.context()->proxy_resolution_service()->proxy_retry_info(),
          proxy_server, nullptr)) {
    DataReductionProxyBypassStats::RecordDataReductionProxyBypassInfo(
        data_reduction_proxy_type_info->proxy_index == 0U,
        data_reduction_proxy_info->bypass_all, proxy_server, *bypass_type);
  }
  return true;
}

}  // namespace data_reduction_proxy
