// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_CERT_CETCHER_H_
#define CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_CERT_CETCHER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "content/browser/web_package/signed_exchange_certificate_chain.h"
#include "content/common/content_export.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/origin.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace mojo {
class SimpleWatcher;
}  // namespace mojo

namespace content {

class SignedExchangeDevToolsProxy;
class ThrottlingURLLoader;
class URLLoaderThrottle;

class CONTENT_EXPORT SignedExchangeCertFetcher
    : public network::mojom::URLLoaderClient {
 public:
  using CertificateCallback =
      base::OnceCallback<void(std::unique_ptr<SignedExchangeCertificateChain>)>;

  // Starts fetching the certificate using a ThrottlingURLLoader created with
  // the |shared_url_loader_factory| and the |throttles|. The |callback| will
  // be called with the certificate if succeeded. Otherwise it will be called
  // with null. If the returned fetcher is destructed before the |callback| is
  // called, the request will be canceled and the |callback| will no be called.
  //
  // Using SignedExchangeCertFetcherFactory is preferred rather than directly
  // calling this.
  static std::unique_ptr<SignedExchangeCertFetcher> CreateAndStart(
      scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory,
      std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
      const GURL& cert_url,
      url::Origin request_initiator,
      bool force_fetch,
      SignedExchangeVersion version,
      CertificateCallback callback,
      SignedExchangeDevToolsProxy* devtools_proxy);

  ~SignedExchangeCertFetcher() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SignedExchangeCertFetcherTest, MaxCertSize_Exceeds);
  FRIEND_TEST_ALL_PREFIXES(SignedExchangeCertFetcherTest, MaxCertSize_SameSize);
  FRIEND_TEST_ALL_PREFIXES(SignedExchangeCertFetcherTest,
                           MaxCertSize_MultipleChunked);
  FRIEND_TEST_ALL_PREFIXES(SignedExchangeCertFetcherTest,
                           MaxCertSize_ContentLengthCheck);

  static base::ScopedClosureRunner SetMaxCertSizeForTest(size_t max_cert_size);

  SignedExchangeCertFetcher(
      scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory,
      std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
      const GURL& cert_url,
      url::Origin request_initiator,
      bool force_fetch,
      SignedExchangeVersion version,
      CertificateCallback callback,
      SignedExchangeDevToolsProxy* devtools_proxy);
  void Start();
  void Abort();
  void OnHandleReady(MojoResult result);
  void OnDataComplete();

  // network::mojom::URLLoaderClient
  void OnReceiveResponse(
      const network::ResourceResponseHead& head,
      network::mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const network::ResourceResponseHead& head) override;
  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback callback) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(const network::URLLoaderCompletionStatus& status) override;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles_;
  std::unique_ptr<network::ResourceRequest> resource_request_;
  const SignedExchangeVersion version_;
  CertificateCallback callback_;

  std::unique_ptr<ThrottlingURLLoader> url_loader_;
  mojo::ScopedDataPipeConsumerHandle body_;
  std::unique_ptr<mojo::SimpleWatcher> handle_watcher_;
  std::string body_string_;

  // This is owned by SignedExchangeHandler which is the owner of |this|.
  SignedExchangeDevToolsProxy* devtools_proxy_;
  base::Optional<base::UnguessableToken> cert_request_id_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeCertFetcher);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_CERT_CETCHER_H_
