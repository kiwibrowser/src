// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/ssl_config_service_mojo.h"

#include "mojo/public/cpp/bindings/type_converter.h"
#include "services/network/ssl_config_type_converter.h"

namespace network {

SSLConfigServiceMojo::SSLConfigServiceMojo(
    mojom::SSLConfigPtr initial_config,
    mojom::SSLConfigClientRequest ssl_config_client_request)
    : binding_(this),
      ssl_config_(initial_config ? mojo::ConvertTo<net::SSLConfig>(
                                       std::move(initial_config))
                                 : net::SSLConfig()) {
  if (ssl_config_client_request)
    binding_.Bind(std::move(ssl_config_client_request));
}

void SSLConfigServiceMojo::OnSSLConfigUpdated(mojom::SSLConfigPtr ssl_config) {
  net::SSLConfig old_config = ssl_config_;
  ssl_config_ = mojo::ConvertTo<net::SSLConfig>(std::move(ssl_config));
  ProcessConfigUpdate(old_config, ssl_config_);
}

void SSLConfigServiceMojo::GetSSLConfig(net::SSLConfig* ssl_config) {
  *ssl_config = ssl_config_;
}

SSLConfigServiceMojo::~SSLConfigServiceMojo() {}

}  // namespace network
