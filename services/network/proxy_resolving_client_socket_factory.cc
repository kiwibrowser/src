// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/proxy_resolving_client_socket_factory.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "net/base/ip_address.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/url_request/url_request_context.h"
#include "services/network/proxy_resolving_client_socket.h"

namespace network {

ProxyResolvingClientSocketFactory::ProxyResolvingClientSocketFactory(
    net::ClientSocketFactory* socket_factory,
    net::URLRequestContext* request_context)
    : request_context_(request_context) {
  DCHECK(request_context);

  net::HttpNetworkSession::Context session_context;
  session_context.client_socket_factory = socket_factory;
  session_context.host_resolver = request_context->host_resolver();
  session_context.cert_verifier = request_context->cert_verifier();
  session_context.transport_security_state =
      request_context->transport_security_state();
  session_context.cert_transparency_verifier =
      request_context->cert_transparency_verifier();
  session_context.ct_policy_enforcer = request_context->ct_policy_enforcer();
  // TODO(rkn): This is NULL because ChannelIDService is not thread safe.
  // TODO(mmenke):  The above comment makes no sense, as not a single one of
  // these classes is thread safe. Figure out if the comment's wrong, or if this
  // entire class is badly broken.
  session_context.channel_id_service = NULL;
  session_context.proxy_resolution_service =
      request_context->proxy_resolution_service();
  session_context.ssl_config_service = request_context->ssl_config_service();
  session_context.http_auth_handler_factory =
      request_context->http_auth_handler_factory();
  session_context.http_server_properties =
      request_context->http_server_properties();
  session_context.net_log = request_context->net_log();

  const net::HttpNetworkSession::Params* reference_params =
      request_context->GetNetworkSessionParams();
  net::HttpNetworkSession::Params session_params;
  if (reference_params) {
    // TODO(mmenke):  Just copying specific parameters seems highly regression
    // prone.  Should have a better way to do this.
    session_params.host_mapping_rules = reference_params->host_mapping_rules;
    session_params.ignore_certificate_errors =
        reference_params->ignore_certificate_errors;
    session_params.testing_fixed_http_port =
        reference_params->testing_fixed_http_port;
    session_params.testing_fixed_https_port =
        reference_params->testing_fixed_https_port;
    session_params.enable_http2 = reference_params->enable_http2;
    session_params.enable_http2_alternative_service =
        reference_params->enable_http2_alternative_service;
  }

  network_session_ = std::make_unique<net::HttpNetworkSession>(session_params,
                                                               session_context);
}

ProxyResolvingClientSocketFactory::~ProxyResolvingClientSocketFactory() {}

std::unique_ptr<ProxyResolvingClientSocket>
ProxyResolvingClientSocketFactory::CreateSocket(
    const net::SSLConfig& ssl_config,
    const GURL& url,
    bool use_tls) {
  // |request_context|'s HttpAuthCache might have updates. For example, a user
  // might have since entered proxy credentials. Clear the http auth of
  // |network_session_| and copy over the data from |request_context|'s auth
  // cache.
  network_session_->http_auth_cache()->ClearAllEntries();
  net::HttpAuthCache* other_auth_cache =
      request_context_->http_transaction_factory()
          ->GetSession()
          ->http_auth_cache();
  network_session_->http_auth_cache()->UpdateAllFrom(*other_auth_cache);
  return std::make_unique<ProxyResolvingClientSocket>(network_session_.get(),
                                                      ssl_config, url, use_tls);
}

}  // namespace network
