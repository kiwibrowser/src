// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NETWORK_SERVICE_IMPL_H_
#define CONTENT_BROWSER_NETWORK_SERVICE_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "url/gurl.h"

namespace content {

class NetworkServiceClient : public network::mojom::NetworkServiceClient {
 public:
  explicit NetworkServiceClient(network::mojom::NetworkServiceClientRequest
                                    network_service_client_request);
  ~NetworkServiceClient() override;

  // network::mojom::NetworkServiceClient implementation:
  void OnAuthRequired(uint32_t process_id,
                      uint32_t routing_id,
                      uint32_t request_id,
                      const GURL& url,
                      const GURL& site_for_cookies,
                      bool first_auth_attempt,
                      const scoped_refptr<net::AuthChallengeInfo>& auth_info,
                      int32_t resource_type,
                      network::mojom::AuthChallengeResponderPtr
                          auth_challenge_responder) override;
  void OnCertificateRequested(
      uint32_t process_id,
      uint32_t routing_id,
      uint32_t request_id,
      const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
      network::mojom::NetworkServiceClient::OnCertificateRequestedCallback
          callback) override;
  void OnSSLCertificateError(uint32_t process_id,
                             uint32_t routing_id,
                             uint32_t request_id,
                             int32_t resource_type,
                             const GURL& url,
                             const net::SSLInfo& ssl_info,
                             bool fatal,
                             OnSSLCertificateErrorCallback response) override;

 private:
  mojo::Binding<network::mojom::NetworkServiceClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceClient);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NETWORK_SERVICE_IMPL_H_
