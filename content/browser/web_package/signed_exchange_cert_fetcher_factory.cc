// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_cert_fetcher_factory.h"

#include "content/public/common/url_loader_throttle.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace content {

SignedExchangeCertFetcherFactory::~SignedExchangeCertFetcherFactory() = default;

class SignedExchangeCertFetcherFactoryImpl
    : public SignedExchangeCertFetcherFactory {
 public:
  SignedExchangeCertFetcherFactoryImpl(
      url::Origin request_initiator,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      URLLoaderThrottlesGetter url_loader_throttles_getter)
      : request_initiator_(std::move(request_initiator)),
        url_loader_factory_(std::move(url_loader_factory)),
        url_loader_throttles_getter_(std::move(url_loader_throttles_getter)) {}

  std::unique_ptr<SignedExchangeCertFetcher> CreateFetcherAndStart(
      const GURL& cert_url,
      bool force_fetch,
      SignedExchangeVersion version,
      SignedExchangeCertFetcher::CertificateCallback callback,
      SignedExchangeDevToolsProxy* devtools_proxy) override;

 private:
  url::Origin request_initiator_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  URLLoaderThrottlesGetter url_loader_throttles_getter_;
};

std::unique_ptr<SignedExchangeCertFetcher>
SignedExchangeCertFetcherFactoryImpl::CreateFetcherAndStart(
    const GURL& cert_url,
    bool force_fetch,
    SignedExchangeVersion version,
    SignedExchangeCertFetcher::CertificateCallback callback,
    SignedExchangeDevToolsProxy* devtools_proxy) {
  DCHECK(url_loader_factory_);
  DCHECK(url_loader_throttles_getter_);
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles =
      std::move(url_loader_throttles_getter_).Run();
  return SignedExchangeCertFetcher::CreateAndStart(
      std::move(url_loader_factory_), std::move(throttles), cert_url,
      std::move(request_initiator_), force_fetch, version, std::move(callback),
      devtools_proxy);
}

// static
std::unique_ptr<SignedExchangeCertFetcherFactory>
SignedExchangeCertFetcherFactory::Create(
    url::Origin request_initiator,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    URLLoaderThrottlesGetter url_loader_throttles_getter) {
  return std::make_unique<SignedExchangeCertFetcherFactoryImpl>(
      std::move(request_initiator), std::move(url_loader_factory),
      std::move(url_loader_throttles_getter));
}

}  // namespace content
