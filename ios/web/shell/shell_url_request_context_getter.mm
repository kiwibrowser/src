// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/shell/shell_url_request_context_getter.h"

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#import "ios/net/cookies/cookie_store_ios_persistent.h"
#import "ios/web/public/web_client.h"
#include "ios/web/shell/shell_network_delegate.h"
#include "net/base/cache_type.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/dns/host_resolver.h"
#include "net/extras/sqlite/sqlite_persistent_cookie_store.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/transport_security_persister.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log.h"
#include "net/proxy_resolution/proxy_config_service_ios.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_job_factory_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

ShellURLRequestContextGetter::ShellURLRequestContextGetter(
    const base::FilePath& base_path,
    const scoped_refptr<base::SingleThreadTaskRunner>& network_task_runner)
    : base_path_(base_path),
      network_task_runner_(network_task_runner),
      proxy_config_service_(
          new net::ProxyConfigServiceIOS(NO_TRAFFIC_ANNOTATION_YET)),
      net_log_(new net::NetLog()) {}

ShellURLRequestContextGetter::~ShellURLRequestContextGetter() {}

net::URLRequestContext* ShellURLRequestContextGetter::GetURLRequestContext() {
  DCHECK(network_task_runner_->BelongsToCurrentThread());

  if (!url_request_context_) {
    url_request_context_.reset(new net::URLRequestContext());
    url_request_context_->set_net_log(net_log_.get());
    DCHECK(!network_delegate_.get());
    network_delegate_.reset(new ShellNetworkDelegate);
    url_request_context_->set_network_delegate(network_delegate_.get());

    storage_.reset(
        new net::URLRequestContextStorage(url_request_context_.get()));

    // Setup the cookie store.
    base::FilePath cookie_path;
    bool cookie_path_found =
        base::PathService::Get(base::DIR_APP_DATA, &cookie_path);
    DCHECK(cookie_path_found);
    cookie_path = cookie_path.Append("WebShell").Append("Cookies");
    scoped_refptr<net::CookieMonster::PersistentCookieStore> persistent_store =
        new net::SQLitePersistentCookieStore(
            cookie_path, network_task_runner_,
            base::CreateSequencedTaskRunnerWithTraits(
                {base::MayBlock(), base::TaskPriority::BACKGROUND}),
            true, nullptr);
    std::unique_ptr<net::CookieStoreIOS> cookie_store(
        new net::CookieStoreIOSPersistent(persistent_store.get()));
    storage_->set_cookie_store(std::move(cookie_store));

    std::string user_agent =
        web::GetWebClient()->GetUserAgent(web::UserAgentType::MOBILE);
    storage_->set_http_user_agent_settings(
        std::make_unique<net::StaticHttpUserAgentSettings>("en-us,en",
                                                           user_agent));
    storage_->set_proxy_resolution_service(
        net::ProxyResolutionService::CreateUsingSystemProxyResolver(
            std::move(proxy_config_service_), url_request_context_->net_log()));
    storage_->set_ssl_config_service(new net::SSLConfigServiceDefaults);
    storage_->set_cert_verifier(net::CertVerifier::CreateDefault());

    storage_->set_transport_security_state(
        std::make_unique<net::TransportSecurityState>());
    storage_->set_cert_transparency_verifier(
        base::WrapUnique(new net::MultiLogCTVerifier));
    storage_->set_ct_policy_enforcer(
        base::WrapUnique(new net::DefaultCTPolicyEnforcer));
    transport_security_persister_ =
        std::make_unique<net::TransportSecurityPersister>(
            url_request_context_->transport_security_state(), base_path_,
            base::CreateSequencedTaskRunnerWithTraits(
                {base::MayBlock(), base::TaskPriority::BACKGROUND}));
    storage_->set_channel_id_service(std::make_unique<net::ChannelIDService>(
        new net::DefaultChannelIDStore(nullptr)));
    storage_->set_http_server_properties(
        std::unique_ptr<net::HttpServerProperties>(
            new net::HttpServerPropertiesImpl()));

    std::unique_ptr<net::HostResolver> host_resolver(
        net::HostResolver::CreateDefaultResolver(
            url_request_context_->net_log()));
    storage_->set_http_auth_handler_factory(
        net::HttpAuthHandlerFactory::CreateDefault(host_resolver.get()));
    storage_->set_host_resolver(std::move(host_resolver));

    net::HttpNetworkSession::Context network_session_context;
    network_session_context.cert_verifier =
        url_request_context_->cert_verifier();
    network_session_context.transport_security_state =
        url_request_context_->transport_security_state();
    network_session_context.cert_transparency_verifier =
        url_request_context_->cert_transparency_verifier();
    network_session_context.ct_policy_enforcer =
        url_request_context_->ct_policy_enforcer();
    network_session_context.channel_id_service =
        url_request_context_->channel_id_service();
    network_session_context.net_log = url_request_context_->net_log();
    network_session_context.proxy_resolution_service =
        url_request_context_->proxy_resolution_service();
    network_session_context.ssl_config_service =
        url_request_context_->ssl_config_service();
    network_session_context.http_auth_handler_factory =
        url_request_context_->http_auth_handler_factory();
    network_session_context.http_server_properties =
        url_request_context_->http_server_properties();
    network_session_context.host_resolver =
        url_request_context_->host_resolver();

    base::FilePath cache_path = base_path_.Append(FILE_PATH_LITERAL("Cache"));
    std::unique_ptr<net::HttpCache::DefaultBackend> main_backend(
        new net::HttpCache::DefaultBackend(
            net::DISK_CACHE, net::CACHE_BACKEND_DEFAULT, cache_path, 0));

    storage_->set_http_network_session(
        std::make_unique<net::HttpNetworkSession>(
            net::HttpNetworkSession::Params(), network_session_context));
    storage_->set_http_transaction_factory(std::make_unique<net::HttpCache>(
        storage_->http_network_session(), std::move(main_backend),
        true /* set_up_quic_server_info */));

    std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(
        new net::URLRequestJobFactoryImpl());
    bool set_protocol = job_factory->SetProtocolHandler(
        "data", base::WrapUnique(new net::DataProtocolHandler));
    DCHECK(set_protocol);

    storage_->set_job_factory(std::move(job_factory));
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
ShellURLRequestContextGetter::GetNetworkTaskRunner() const {
  return network_task_runner_;
}

}  // namespace web
