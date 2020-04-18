// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/network_context_owner.h"

#include <memory>

#include "ios/web/public/web_thread.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_context_getter_observer.h"
#include "services/network/network_context.h"

namespace web {

NetworkContextOwner::NetworkContextOwner(
    net::URLRequestContextGetter* request_context,
    network::mojom::NetworkContextPtr* network_context_client)
    : request_context_(request_context) {
  DCHECK_CURRENTLY_ON(WebThread::UI);
  web::WebThread::PostTask(
      web::WebThread::IO, FROM_HERE,
      base::BindOnce(&NetworkContextOwner::InitializeOnIOThread,
                     // This is safe, since |this| will be deleted on the IO
                     // thread, which would have to happen afterwards.
                     base::Unretained(this),
                     mojo::MakeRequest(network_context_client)));
}

NetworkContextOwner::~NetworkContextOwner() {
  DCHECK_CURRENTLY_ON(WebThread::IO);
  if (request_context_)
    request_context_->RemoveObserver(this);
}

void NetworkContextOwner::InitializeOnIOThread(
    network::mojom::NetworkContextRequest network_context_request) {
  DCHECK_CURRENTLY_ON(WebThread::IO);
  DCHECK(!network_context_);

  network_context_ = std::make_unique<network::NetworkContext>(
      nullptr, std::move(network_context_request),
      request_context_->GetURLRequestContext());
  request_context_->AddObserver(this);
}

void NetworkContextOwner::OnContextShuttingDown() {
  DCHECK_CURRENTLY_ON(WebThread::IO);

  // Cancels any pending requests owned by the NetworkContext.
  network_context_.reset();

  request_context_->RemoveObserver(this);
  request_context_ = nullptr;
}

}  // namespace web
