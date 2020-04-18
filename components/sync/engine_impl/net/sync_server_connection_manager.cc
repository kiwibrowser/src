// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/net/sync_server_connection_manager.h"

#include <stdint.h>

#include "base/callback_helpers.h"
#include "components/sync/base/cancelation_signal.h"
#include "components/sync/engine/net/http_post_provider_factory.h"
#include "components/sync/engine/net/http_post_provider_interface.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"

namespace syncer {

SyncBridgedConnection::SyncBridgedConnection(
    ServerConnectionManager* scm,
    HttpPostProviderFactory* factory,
    CancelationSignal* cancelation_signal)
    : Connection(scm),
      factory_(factory),
      cancelation_signal_(cancelation_signal) {
  post_provider_ = factory_->Create();
}

SyncBridgedConnection::~SyncBridgedConnection() {
  DCHECK(post_provider_);
  factory_->Destroy(post_provider_);
  post_provider_ = nullptr;
}

bool SyncBridgedConnection::Init(const char* path,
                                 const std::string& auth_token,
                                 const std::string& payload,
                                 HttpResponse* response) {
  std::string sync_server;
  int sync_server_port = 0;
  bool use_ssl = false;
  GetServerParams(&sync_server, &sync_server_port, &use_ssl);
  std::string connection_url = MakeConnectionURL(sync_server, path, use_ssl);

  HttpPostProviderInterface* http = post_provider_;
  http->SetURL(connection_url.c_str(), sync_server_port);

  if (!auth_token.empty()) {
    std::string headers;
    headers = "Authorization: Bearer " + auth_token;
    http->SetExtraRequestHeaders(headers.c_str());
  }

  // Must be octet-stream, or the payload may be parsed for a cookie.
  http->SetPostPayload("application/octet-stream", payload.length(),
                       payload.data());

  // Issue the POST, blocking until it finishes.
  int error_code = 0;
  int response_code = 0;
  if (!cancelation_signal_->TryRegisterHandler(this)) {
    // Return early because cancelation signal was signaled.
    response->server_status = HttpResponse::CONNECTION_UNAVAILABLE;
    return false;
  }
  base::ScopedClosureRunner auto_unregister(base::Bind(
      &CancelationSignal::UnregisterHandler,
      base::Unretained(cancelation_signal_), base::Unretained(this)));

  if (!http->MakeSynchronousPost(&error_code, &response_code)) {
    DCHECK_NE(error_code, net::OK);
    DVLOG(1) << "Http POST failed, error returns: " << error_code;
    response->server_status = HttpResponse::CONNECTION_UNAVAILABLE;
    return false;
  }

  // We got a server response, copy over response codes and content.
  response->response_code = response_code;
  response->content_length =
      static_cast<int64_t>(http->GetResponseContentLength());
  response->payload_length =
      static_cast<int64_t>(http->GetResponseContentLength());
  if (response->response_code < 400)
    response->server_status = HttpResponse::SERVER_CONNECTION_OK;
  else if (response->response_code == net::HTTP_UNAUTHORIZED)
    response->server_status = HttpResponse::SYNC_AUTH_ERROR;
  else
    response->server_status = HttpResponse::SYNC_SERVER_ERROR;

  // Write the content into our buffer.
  buffer_.assign(http->GetResponseContent(), http->GetResponseContentLength());
  return true;
}

void SyncBridgedConnection::Abort() {
  DCHECK(post_provider_);
  post_provider_->Abort();
}

void SyncBridgedConnection::OnSignalReceived() {
  DCHECK(post_provider_);
  post_provider_->Abort();
}

SyncServerConnectionManager::SyncServerConnectionManager(
    const std::string& server,
    int port,
    bool use_ssl,
    HttpPostProviderFactory* factory,
    CancelationSignal* cancelation_signal)
    : ServerConnectionManager(server, port, use_ssl, cancelation_signal),
      post_provider_factory_(factory),
      cancelation_signal_(cancelation_signal) {
  DCHECK(post_provider_factory_);
}

SyncServerConnectionManager::~SyncServerConnectionManager() = default;

std::unique_ptr<ServerConnectionManager::Connection>
SyncServerConnectionManager::MakeConnection() {
  return std::make_unique<SyncBridgedConnection>(
      this, post_provider_factory_.get(), cancelation_signal_);
}

}  // namespace syncer
