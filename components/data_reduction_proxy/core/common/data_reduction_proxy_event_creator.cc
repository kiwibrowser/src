// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_creator.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/base/proxy_server.h"
#include "net/log/net_log.h"
#include "net/log/net_log_entry.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"

namespace data_reduction_proxy {

namespace {

std::unique_ptr<base::Value> BuildDataReductionProxyEvent(
    net::NetLogEventType type,
    const net::NetLogSource& source,
    net::NetLogEventPhase phase,
    const net::NetLogParametersCallback& parameters_callback) {
  base::TimeTicks ticks_now = base::TimeTicks::Now();
  net::NetLogEntryData entry_data(type, source, phase, ticks_now,
                                  &parameters_callback);
  net::NetLogEntry entry(&entry_data,
                         net::NetLogCaptureMode::IncludeSocketBytes());
  std::unique_ptr<base::Value> entry_value(entry.ToValue());

  return entry_value;
}

int64_t GetExpirationTicks(int bypass_seconds) {
  base::TimeTicks expiration_ticks =
      base::TimeTicks::Now() + base::TimeDelta::FromSeconds(bypass_seconds);
  return (expiration_ticks - base::TimeTicks()).InMilliseconds();
}

// A callback which creates a base::Value containing information about enabling
// the Data Reduction Proxy.
std::unique_ptr<base::Value> EnableDataReductionProxyCallback(
    bool secure_transport_restricted,
    const std::vector<net::ProxyServer>& proxies_for_http,
    net::NetLogCaptureMode /* capture_mode */) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetBoolean("enabled", true);
  dict->SetBoolean("secure_transport_restricted", secure_transport_restricted);
  std::unique_ptr<base::ListValue> http_proxy_list(new base::ListValue());
  for (const auto& proxy : proxies_for_http)
    http_proxy_list->AppendString(proxy.ToURI());

  dict->Set("http_proxy_list", std::move(http_proxy_list));

  return std::move(dict);
}

// A callback which creates a base::Value containing information about disabling
// the Data Reduction Proxy.
std::unique_ptr<base::Value> DisableDataReductionProxyCallback(
    net::NetLogCaptureMode /* capture_mode */) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetBoolean("enabled", false);
  return std::move(dict);
}

// A callback which creates a base::Value containing information about bypassing
// the Data Reduction Proxy.
std::unique_ptr<base::Value> UrlBypassActionCallback(
    DataReductionProxyBypassAction action,
    const std::string& request_method,
    const GURL& url,
    bool should_retry,
    int bypass_seconds,
    int64_t expiration_ticks,
    net::NetLogCaptureMode /* capture_mode */) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetInteger("bypass_action_type", action);
  dict->SetString("method", request_method);
  dict->SetString("url", url.spec());
  dict->SetBoolean("should_retry", should_retry);
  dict->SetString("bypass_duration_seconds",
                  base::Int64ToString(bypass_seconds));
  dict->SetString("expiration", base::Int64ToString(expiration_ticks));
  return std::move(dict);
}

// A callback which creates a base::Value containing information about bypassing
// the Data Reduction Proxy.
std::unique_ptr<base::Value> UrlBypassTypeCallback(
    DataReductionProxyBypassType bypass_type,
    const std::string& request_method,
    const GURL& url,
    bool should_retry,
    int bypass_seconds,
    int64_t expiration_ticks,
    net::NetLogCaptureMode /* capture_mode */) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetInteger("bypass_type", bypass_type);
  dict->SetString("method", request_method);
  dict->SetString("url", url.spec());
  dict->SetBoolean("should_retry", should_retry);
  dict->SetString("bypass_duration_seconds",
                  base::Int64ToString(bypass_seconds));
  dict->SetString("expiration", base::Int64ToString(expiration_ticks));
  return std::move(dict);
}

// A callback that creates a base::Value containing information about a proxy
// fallback event for a Data Reduction Proxy.
std::unique_ptr<base::Value> FallbackCallback(
    const std::string& proxy_url,
    int net_error,
    net::NetLogCaptureMode /* capture_mode */) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetString("proxy", proxy_url);
  dict->SetInteger("net_error", net_error);
  return std::move(dict);
}

// A callback which creates a base::Value containing information about
// completing the Data Reduction Proxy secure proxy check.
std::unique_ptr<base::Value> EndCanaryRequestCallback(
    int net_error,
    int http_response_code,
    bool succeeded,
    net::NetLogCaptureMode /* capture_mode */) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetInteger("net_error", net_error);
  dict->SetInteger("http_response_code", http_response_code);
  dict->SetBoolean("check_succeeded", succeeded);
  return std::move(dict);
}

// A callback that creates a base::Value containing information about
// completing the Data Reduction Proxy configuration request.
std::unique_ptr<base::Value> EndConfigRequestCallback(
    int net_error,
    int http_response_code,
    int failure_count,
    const std::vector<net::ProxyServer>& proxies_for_http,
    int64_t refresh_duration_minutes,
    int64_t expiration_ticks,
    net::NetLogCaptureMode /* capture_mode */) {
  std::unique_ptr<base::ListValue> http_proxy_list(new base::ListValue());
  for (const auto& proxy : proxies_for_http)
    http_proxy_list->AppendString(proxy.ToURI());
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetInteger("net_error", net_error);
  dict->SetInteger("http_response_code", http_response_code);
  dict->SetInteger("failure_count", failure_count);
  dict->Set("http_proxy_list_in_config", std::move(http_proxy_list));
  dict->SetString("refresh_duration",
                  base::Int64ToString(refresh_duration_minutes) + " minutes");
  dict->SetString("expiration", base::Int64ToString(expiration_ticks));
  return std::move(dict);
}

}  // namespace

DataReductionProxyEventCreator::DataReductionProxyEventCreator(
    DataReductionProxyEventStorageDelegate* storage_delegate)
    : storage_delegate_(storage_delegate) {
  DCHECK(storage_delegate);
  // Constructed on the UI thread, but should be checked on the IO thread.
  thread_checker_.DetachFromThread();
}

DataReductionProxyEventCreator::~DataReductionProxyEventCreator() {
}

void DataReductionProxyEventCreator::AddProxyEnabledEvent(
    net::NetLog* net_log,
    bool secure_transport_restricted,
    const std::vector<net::ProxyServer>& proxies_for_http) {
  DCHECK(thread_checker_.CalledOnValidThread());
  const net::NetLogParametersCallback& parameters_callback =
      base::Bind(&EnableDataReductionProxyCallback, secure_transport_restricted,
                 proxies_for_http);
  PostEnabledEvent(net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_ENABLED,
                   true, parameters_callback);
}

void DataReductionProxyEventCreator::AddProxyDisabledEvent(
    net::NetLog* net_log) {
  DCHECK(thread_checker_.CalledOnValidThread());
  const net::NetLogParametersCallback& parameters_callback =
      base::Bind(&DisableDataReductionProxyCallback);
  PostEnabledEvent(net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_ENABLED,
                   false, parameters_callback);
}

void DataReductionProxyEventCreator::AddBypassActionEvent(
    const net::NetLogWithSource& net_log,
    DataReductionProxyBypassAction bypass_action,
    const std::string& request_method,
    const GURL& url,
    bool should_retry,
    const base::TimeDelta& bypass_duration) {
  DCHECK(thread_checker_.CalledOnValidThread());
  int64_t expiration_ticks = GetExpirationTicks(bypass_duration.InSeconds());
  const net::NetLogParametersCallback& parameters_callback =
      base::Bind(&UrlBypassActionCallback, bypass_action, request_method, url,
                 should_retry, bypass_duration.InSeconds(), expiration_ticks);
  PostNetLogWithSourceBypassEvent(
      net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_BYPASS_REQUESTED,
      net::NetLogEventPhase::NONE, expiration_ticks, parameters_callback);
}

void DataReductionProxyEventCreator::AddBypassTypeEvent(
    const net::NetLogWithSource& net_log,
    DataReductionProxyBypassType bypass_type,
    const std::string& request_method,
    const GURL& url,
    bool should_retry,
    const base::TimeDelta& bypass_duration) {
  DCHECK(thread_checker_.CalledOnValidThread());
  int64_t expiration_ticks = GetExpirationTicks(bypass_duration.InSeconds());
  const net::NetLogParametersCallback& parameters_callback =
      base::Bind(&UrlBypassTypeCallback, bypass_type, request_method, url,
                 should_retry, bypass_duration.InSeconds(), expiration_ticks);
  PostNetLogWithSourceBypassEvent(
      net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_BYPASS_REQUESTED,
      net::NetLogEventPhase::NONE, expiration_ticks, parameters_callback);
}

void DataReductionProxyEventCreator::AddProxyFallbackEvent(
    net::NetLog* net_log,
    const std::string& proxy_url,
    int net_error) {
  DCHECK(thread_checker_.CalledOnValidThread());
  const net::NetLogParametersCallback& parameters_callback =
      base::Bind(&FallbackCallback, proxy_url, net_error);
  PostEvent(net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_FALLBACK,
            parameters_callback);
}

void DataReductionProxyEventCreator::BeginSecureProxyCheck(
    const net::NetLogWithSource& net_log,
    const GURL& url) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // This callback must be invoked synchronously
  const net::NetLogParametersCallback& parameters_callback =
      net::NetLog::StringCallback("url", &url.spec());
  PostNetLogWithSourceSecureProxyCheckEvent(
      net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_CANARY_REQUEST,
      net::NetLogEventPhase::BEGIN,
      DataReductionProxyEventStorageDelegate::CHECK_PENDING,
      parameters_callback);
}

void DataReductionProxyEventCreator::EndSecureProxyCheck(
    const net::NetLogWithSource& net_log,
    int net_error,
    int http_response_code,
    bool succeeded) {
  DCHECK(thread_checker_.CalledOnValidThread());
  const net::NetLogParametersCallback& parameters_callback = base::Bind(
      &EndCanaryRequestCallback, net_error, http_response_code, succeeded);
  PostNetLogWithSourceSecureProxyCheckEvent(
      net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_CANARY_REQUEST,
      net::NetLogEventPhase::END,
      succeeded ? DataReductionProxyEventStorageDelegate::CHECK_SUCCESS
                : DataReductionProxyEventStorageDelegate::CHECK_FAILED,
      parameters_callback);
}

void DataReductionProxyEventCreator::BeginConfigRequest(
    const net::NetLogWithSource& net_log,
    const GURL& url) {
  // This callback must be invoked synchronously.
  const net::NetLogParametersCallback& parameters_callback =
      net::NetLog::StringCallback("url", &url.spec());
  PostNetLogWithSourceConfigRequestEvent(
      net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_CONFIG_REQUEST,
      net::NetLogEventPhase::BEGIN, parameters_callback);
}

void DataReductionProxyEventCreator::EndConfigRequest(
    const net::NetLogWithSource& net_log,
    int net_error,
    int http_response_code,
    int failure_count,
    const std::vector<net::ProxyServer>& proxies_for_http,
    const base::TimeDelta& refresh_duration,
    const base::TimeDelta& retry_delay) {
  int64_t refresh_duration_minutes = refresh_duration.InMinutes();
  int64_t expiration_ticks = GetExpirationTicks(retry_delay.InSeconds());
  const net::NetLogParametersCallback& parameters_callback = base::Bind(
      &EndConfigRequestCallback, net_error, http_response_code, failure_count,
      proxies_for_http, refresh_duration_minutes, expiration_ticks);
  PostNetLogWithSourceConfigRequestEvent(
      net_log, net::NetLogEventType::DATA_REDUCTION_PROXY_CONFIG_REQUEST,
      net::NetLogEventPhase::END, parameters_callback);
}

void DataReductionProxyEventCreator::PostEvent(
    net::NetLog* net_log,
    net::NetLogEventType type,
    const net::NetLogParametersCallback& callback) {
  std::unique_ptr<base::Value> event = BuildDataReductionProxyEvent(
      type, net::NetLogSource(), net::NetLogEventPhase::NONE, callback);
  if (event)
    storage_delegate_->AddEvent(std::move(event));

  if (net_log)
    net_log->AddGlobalEntry(type, callback);
}

void DataReductionProxyEventCreator::PostEnabledEvent(
    net::NetLog* net_log,
    net::NetLogEventType type,
    bool enabled,
    const net::NetLogParametersCallback& callback) {
  std::unique_ptr<base::Value> event = BuildDataReductionProxyEvent(
      type, net::NetLogSource(), net::NetLogEventPhase::NONE, callback);
  if (event)
    storage_delegate_->AddEnabledEvent(std::move(event), enabled);

  if (net_log)
    net_log->AddGlobalEntry(type, callback);
}

void DataReductionProxyEventCreator::PostNetLogWithSourceBypassEvent(
    const net::NetLogWithSource& net_log,
    net::NetLogEventType type,
    net::NetLogEventPhase phase,
    int64_t expiration_ticks,
    const net::NetLogParametersCallback& callback) {
  std::unique_ptr<base::Value> event =
      BuildDataReductionProxyEvent(type, net_log.source(), phase, callback);
  if (event)
    storage_delegate_->AddAndSetLastBypassEvent(std::move(event),
                                                expiration_ticks);
  net_log.AddEntry(type, phase, callback);
}

void DataReductionProxyEventCreator::PostNetLogWithSourceSecureProxyCheckEvent(
    const net::NetLogWithSource& net_log,
    net::NetLogEventType type,
    net::NetLogEventPhase phase,
    DataReductionProxyEventStorageDelegate::SecureProxyCheckState state,
    const net::NetLogParametersCallback& callback) {
  std::unique_ptr<base::Value> event(
      BuildDataReductionProxyEvent(type, net_log.source(), phase, callback));
  if (event)
    storage_delegate_->AddEventAndSecureProxyCheckState(std::move(event),
                                                        state);
  net_log.AddEntry(type, phase, callback);
}

void DataReductionProxyEventCreator::PostNetLogWithSourceConfigRequestEvent(
    const net::NetLogWithSource& net_log,
    net::NetLogEventType type,
    net::NetLogEventPhase phase,
    const net::NetLogParametersCallback& callback) {
  std::unique_ptr<base::Value> event(
      BuildDataReductionProxyEvent(type, net_log.source(), phase, callback));
  if (event) {
    storage_delegate_->AddEvent(std::move(event));
    net_log.AddEntry(type, phase, callback);
  }
}

}  // namespace data_reduction_proxy
