// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_MOCK_SIGNED_EXCHANGE_HANDLER_H_
#define CONTENT_BROWSER_WEB_PACKAGE_MOCK_SIGNED_EXCHANGE_HANDLER_H_

#include <string>
#include <vector>

#include "content/browser/web_package/signed_exchange_handler.h"
#include "url/gurl.h"

namespace content {

class SignedExchangeCertFetcherFactory;

class MockSignedExchangeHandler final : public SignedExchangeHandler {
 public:
  MockSignedExchangeHandler(net::Error error,
                            const GURL& request_url,
                            const std::string& mime_type,
                            const std::vector<std::string>& response_headers,
                            std::unique_ptr<net::SourceStream> body,
                            ExchangeHeadersCallback headers_callback);
  ~MockSignedExchangeHandler();

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSignedExchangeHandler);
};

class MockSignedExchangeHandlerFactory final
    : public SignedExchangeHandlerFactory {
 public:
  using ExchangeHeadersCallback =
      SignedExchangeHandler::ExchangeHeadersCallback;

  // Creates a factory that creates SignedExchangeHandler which always fires
  // a headers callback with the given |error|, |request_url|, |mime_type|
  // and |response_headers|.
  // |mime_type| and |response_headers| are ignored if |error| is not
  // net::OK.
  MockSignedExchangeHandlerFactory(net::Error error,
                                   const GURL& request_url,
                                   const std::string& mime_type,
                                   std::vector<std::string> response_headers);
  ~MockSignedExchangeHandlerFactory() override;

  std::unique_ptr<SignedExchangeHandler> Create(
      std::unique_ptr<net::SourceStream> body,
      ExchangeHeadersCallback headers_callback,
      std::unique_ptr<SignedExchangeCertFetcherFactory> cert_fetcher_factory)
      override;

 private:
  const net::Error error_;
  const GURL request_url_;
  const std::string mime_type_;
  const std::vector<std::string> response_headers_;

  DISALLOW_COPY_AND_ASSIGN(MockSignedExchangeHandlerFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_MOCK_SIGNED_EXCHANGE_HANDLER_H_
