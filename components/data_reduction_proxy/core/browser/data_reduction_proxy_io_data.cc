// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_io_data.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_bypass_protocol.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_bypass_stats.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_compression_stats.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config_service_client.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_configurator.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_delegate.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_interceptor.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_mutable_config_values.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_network_delegate.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_service.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_settings.h"
#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_creator.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_storage_delegate.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_switches.h"
#include "components/previews/core/previews_decider.h"
#include "net/base/load_flags.h"
#include "net/url_request/http_user_agent_settings.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"

namespace data_reduction_proxy {

// A |net::URLRequestContextGetter| which uses only vanilla HTTP/HTTPS for
// performing requests. This is used by the secure proxy check to prevent the
// use of SPDY and QUIC which may be used by the primary request contexts.
class BasicHTTPURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  BasicHTTPURLRequestContextGetter(
      const std::string& user_agent,
      const scoped_refptr<base::SingleThreadTaskRunner>& network_task_runner);

  // Overridden from net::URLRequestContextGetter:
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

 private:
  ~BasicHTTPURLRequestContextGetter() override;

  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;
  std::unique_ptr<net::HttpUserAgentSettings> user_agent_settings_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;

  DISALLOW_COPY_AND_ASSIGN(BasicHTTPURLRequestContextGetter);
};

BasicHTTPURLRequestContextGetter::BasicHTTPURLRequestContextGetter(
    const std::string& user_agent,
    const scoped_refptr<base::SingleThreadTaskRunner>& network_task_runner)
    : network_task_runner_(network_task_runner),
      user_agent_settings_(
          new net::StaticHttpUserAgentSettings(std::string(), user_agent)) {
}

net::URLRequestContext*
BasicHTTPURLRequestContextGetter::GetURLRequestContext() {
  if (!url_request_context_) {
    net::URLRequestContextBuilder builder;
    builder.set_proxy_resolution_service(net::ProxyResolutionService::CreateDirect());
    builder.SetSpdyAndQuicEnabled(false, false);
    url_request_context_ = builder.Build();
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
BasicHTTPURLRequestContextGetter::GetNetworkTaskRunner() const {
  return network_task_runner_;
}

BasicHTTPURLRequestContextGetter::~BasicHTTPURLRequestContextGetter() {
}

DataReductionProxyIOData::DataReductionProxyIOData(
    Client client,
    PrefService* prefs,
    net::NetLog* net_log,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    bool enabled,
    const std::string& user_agent,
    const std::string& channel)
    : client_(client),
      net_log_(net_log),
      io_task_runner_(io_task_runner),
      ui_task_runner_(ui_task_runner),
      data_use_observer_(nullptr),
      enabled_(enabled),
      url_request_context_getter_(nullptr),
      basic_url_request_context_getter_(
          new BasicHTTPURLRequestContextGetter(user_agent, io_task_runner)),
      channel_(channel),
      weak_factory_(this) {
  DCHECK(net_log);
  DCHECK(io_task_runner_);
  DCHECK(ui_task_runner_);
  std::unique_ptr<DataReductionProxyParams> params(
      new DataReductionProxyParams());
  event_creator_.reset(new DataReductionProxyEventCreator(this));
  configurator_.reset(
      new DataReductionProxyConfigurator(net_log, event_creator_.get()));
  bool use_config_client =
      params::IsConfigClientEnabled() && client_ != Client::CRONET_ANDROID;
  DataReductionProxyMutableConfigValues* raw_mutable_config = nullptr;
  if (use_config_client) {
    std::unique_ptr<DataReductionProxyMutableConfigValues> mutable_config =
        std::make_unique<DataReductionProxyMutableConfigValues>();
    raw_mutable_config = mutable_config.get();
    config_.reset(new DataReductionProxyConfig(
        io_task_runner, net_log, std::move(mutable_config), configurator_.get(),
        event_creator_.get()));
  } else {
    config_.reset(new DataReductionProxyConfig(
        io_task_runner, net_log, std::move(params), configurator_.get(),
        event_creator_.get()));
  }

  // It is safe to use base::Unretained here, since it gets executed
  // synchronously on the IO thread, and |this| outlives the caller (since the
  // caller is owned by |this|.
  bypass_stats_.reset(new DataReductionProxyBypassStats(
      config_.get(), base::Bind(&DataReductionProxyIOData::SetUnreachable,
                                base::Unretained(this))));
  request_options_.reset(
      new DataReductionProxyRequestOptions(client_, config_.get()));
  request_options_->Init();
  if (use_config_client) {
    // It is safe to use base::Unretained here, since it gets executed
    // synchronously on the IO thread, and |this| outlives the caller (since the
    // caller is owned by |this|.
    config_client_.reset(new DataReductionProxyConfigServiceClient(
        std::move(params), GetBackoffPolicy(), request_options_.get(),
        raw_mutable_config, config_.get(), event_creator_.get(), this, net_log_,
        base::Bind(&DataReductionProxyIOData::StoreSerializedConfig,
                   base::Unretained(this))));
  }

  proxy_delegate_.reset(new DataReductionProxyDelegate(
      config_.get(), configurator_.get(), event_creator_.get(),
      bypass_stats_.get(), net_log_));
  network_properties_manager_.reset(new NetworkPropertiesManager(
      base::DefaultClock::GetInstance(), prefs, ui_task_runner_));
}

DataReductionProxyIOData::DataReductionProxyIOData(
    PrefService* prefs,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : client_(Client::UNKNOWN),
      net_log_(nullptr),
      io_task_runner_(io_task_runner),
      ui_task_runner_(ui_task_runner),
      url_request_context_getter_(nullptr),
      weak_factory_(this) {
  DCHECK(ui_task_runner_);
  DCHECK(io_task_runner_);
  network_properties_manager_.reset(new NetworkPropertiesManager(
      base::DefaultClock::GetInstance(), prefs, ui_task_runner_));
}

DataReductionProxyIOData::~DataReductionProxyIOData() {
  // Guaranteed to be destroyed on IO thread if the IO thread is still
  // available at the time of destruction. If the IO thread is unavailable,
  // then the destruction will happen on the UI thread.
}

void DataReductionProxyIOData::ShutdownOnUIThread() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  network_properties_manager_->ShutdownOnUIThread();
}

void DataReductionProxyIOData::SetDataReductionProxyService(
    base::WeakPtr<DataReductionProxyService> data_reduction_proxy_service) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  service_ = data_reduction_proxy_service;
  url_request_context_getter_ = service_->url_request_context_getter();
  // Using base::Unretained is safe here, unless the browser is being shut down
  // before the Initialize task can be executed. The task is only created as
  // part of class initialization.
  if (io_task_runner_->BelongsToCurrentThread()) {
    InitializeOnIOThread();
    return;
  }
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&DataReductionProxyIOData::InitializeOnIOThread,
                 base::Unretained(this)));
}

void DataReductionProxyIOData::InitializeOnIOThread() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(network_properties_manager_);
  config_->InitializeOnIOThread(basic_url_request_context_getter_.get(),
                                url_request_context_getter_,
                                network_properties_manager_.get());
  bypass_stats_->InitializeOnIOThread();
  proxy_delegate_->InitializeOnIOThread(this);
  if (config_client_)
    config_client_->InitializeOnIOThread(url_request_context_getter_);
  if (ui_task_runner_->BelongsToCurrentThread()) {
    service_->SetIOData(weak_factory_.GetWeakPtr());
    return;
  }
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&DataReductionProxyService::SetIOData,
                 service_, weak_factory_.GetWeakPtr()));
}

bool DataReductionProxyIOData::IsEnabled() const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return enabled_;
}

void DataReductionProxyIOData::SetPingbackReportingFraction(
    float pingback_reporting_fraction) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&DataReductionProxyService::SetPingbackReportingFraction,
                 service_, pingback_reporting_fraction));
}

void DataReductionProxyIOData::DeleteBrowsingHistory(const base::Time start,
                                                     const base::Time end) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  network_properties_manager_->DeleteHistory();
}

void DataReductionProxyIOData::OnCacheCleared(const base::Time start,
                                              const base::Time end) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  network_properties_manager_->DeleteHistory();
}

std::unique_ptr<net::URLRequestInterceptor>
DataReductionProxyIOData::CreateInterceptor() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return std::make_unique<DataReductionProxyInterceptor>(
      config_.get(), config_client_.get(), bypass_stats_.get(),
      event_creator_.get());
}

std::unique_ptr<DataReductionProxyNetworkDelegate>
DataReductionProxyIOData::CreateNetworkDelegate(
    std::unique_ptr<net::NetworkDelegate> wrapped_network_delegate,
    bool track_proxy_bypass_statistics) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  std::unique_ptr<DataReductionProxyNetworkDelegate> network_delegate(
      new DataReductionProxyNetworkDelegate(
          std::move(wrapped_network_delegate), config_.get(),
          request_options_.get(), configurator_.get()));
  if (track_proxy_bypass_statistics)
    network_delegate->InitIODataAndUMA(this, bypass_stats_.get());

  return network_delegate;
}

std::unique_ptr<DataReductionProxyDelegate>
DataReductionProxyIOData::CreateProxyDelegate() const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return std::make_unique<DataReductionProxyDelegate>(
      config_.get(), configurator_.get(), event_creator_.get(),
      bypass_stats_.get(), net_log_);
}

// TODO(kundaji): Rename this method to something more descriptive.
// Bug http://crbug/488190.
void DataReductionProxyIOData::SetProxyPrefs(bool enabled, bool at_startup) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(url_request_context_getter_->GetURLRequestContext()->proxy_resolution_service());
  enabled_ = enabled;
  config_->SetProxyConfig(enabled, at_startup);
  if (config_client_) {
    config_client_->SetEnabled(enabled);
    if (enabled)
      config_client_->RetrieveConfig();
  }

  // If Data Saver is disabled, reset data reduction proxy state.
  if (!enabled) {
    net::ProxyResolutionService* proxy_resolution_service =
        url_request_context_getter_->GetURLRequestContext()->proxy_resolution_service();
    proxy_resolution_service->ClearBadProxiesCache();
    bypass_stats_->ClearRequestCounts();
    bypass_stats_->NotifyUnavailabilityIfChanged();
  }
}

void DataReductionProxyIOData::SetDataReductionProxyConfiguration(
    const std::string& serialized_config) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (config_client_)
    config_client_->ApplySerializedConfig(serialized_config);
}

bool DataReductionProxyIOData::ShouldAcceptServerPreview(
    const net::URLRequest& request,
    previews::PreviewsDecider* previews_decider) {
  DCHECK(previews_decider);
  DCHECK((request.load_flags() & net::LOAD_MAIN_FRAME_DEPRECATED) != 0);
  if (!config_ || (config_->IsBypassedByDataReductionProxyLocalRules(
                      request, configurator_->GetProxyConfig()))) {
    return false;
  }
  return config_->ShouldAcceptServerPreview(request, *previews_decider);
}

void DataReductionProxyIOData::UpdateDataUseForHost(int64_t network_bytes,
                                                    int64_t original_bytes,
                                                    const std::string& host) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DataReductionProxyService::UpdateDataUseForHost,
                            service_, network_bytes, original_bytes, host));
}

void DataReductionProxyIOData::UpdateContentLengths(
    int64_t data_used,
    int64_t original_size,
    bool data_reduction_proxy_enabled,
    DataReductionProxyRequestType request_type,
    const std::string& mime_type) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&DataReductionProxyService::UpdateContentLengths, service_,
                 data_used, original_size, data_reduction_proxy_enabled,
                 request_type, mime_type));
}

void DataReductionProxyIOData::AddEvent(std::unique_ptr<base::Value> event) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DataReductionProxyService::AddEvent, service_,
                                std::move(event)));
}

void DataReductionProxyIOData::AddEnabledEvent(
    std::unique_ptr<base::Value> event,
    bool enabled) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DataReductionProxyService::AddEnabledEvent,
                                service_, std::move(event), enabled));
}

void DataReductionProxyIOData::AddEventAndSecureProxyCheckState(
    std::unique_ptr<base::Value> event,
    SecureProxyCheckState state) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &DataReductionProxyService::AddEventAndSecureProxyCheckState,
          service_, std::move(event), state));
}

void DataReductionProxyIOData::AddAndSetLastBypassEvent(
    std::unique_ptr<base::Value> event,
    int64_t expiration_ticks) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&DataReductionProxyService::AddAndSetLastBypassEvent,
                     service_, std::move(event), expiration_ticks));
}

void DataReductionProxyIOData::SetUnreachable(bool unreachable) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&DataReductionProxyService::SetUnreachable,
                 service_, unreachable));
}

void DataReductionProxyIOData::SetInt64Pref(const std::string& pref_path,
                                            int64_t value) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DataReductionProxyService::SetInt64Pref, service_,
                            pref_path, value));
}

void DataReductionProxyIOData::SetStringPref(const std::string& pref_path,
                                             const std::string& value) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DataReductionProxyService::SetStringPref, service_,
                            pref_path, value));
}

void DataReductionProxyIOData::StoreSerializedConfig(
    const std::string& serialized_config) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  SetStringPref(prefs::kDataReductionProxyConfig, serialized_config);
  SetInt64Pref(prefs::kDataReductionProxyLastConfigRetrievalTime,
               (base::Time::Now() - base::Time()).InMicroseconds());
}

void DataReductionProxyIOData::SetDataUseAscriber(
    data_use_measurement::DataUseAscriber* data_use_ascriber) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(data_use_ascriber);
  data_use_observer_.reset(
      new DataReductionProxyDataUseObserver(this, data_use_ascriber));

  // Disable data use ascriber when data saver is not enabled.
  if (!IsEnabled()) {
    data_use_ascriber->DisableAscriber();
  }
}

void DataReductionProxyIOData::SetPreviewsDecider(
    previews::PreviewsDecider* previews_decider) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(previews_decider);
  previews_decider_ = previews_decider;
}

}  // namespace data_reduction_proxy
