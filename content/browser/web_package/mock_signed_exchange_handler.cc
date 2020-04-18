// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/mock_signed_exchange_handler.h"

#include "base/callback.h"
#include "base/strings/stringprintf.h"
#include "content/browser/web_package/signed_exchange_cert_fetcher_factory.h"
#include "net/filter/source_stream.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"

namespace content {

MockSignedExchangeHandler::MockSignedExchangeHandler(
    net::Error error,
    const GURL& request_url,
    const std::string& mime_type,
    const std::vector<std::string>& response_headers,
    std::unique_ptr<net::SourceStream> body,
    ExchangeHeadersCallback headers_callback) {
  network::ResourceResponseHead head;
  if (error == net::OK) {
    head.headers =
        base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
    head.mime_type = mime_type;
    head.headers->AddHeader(
        base::StringPrintf("Content-type: %s", mime_type.c_str()));
    for (const auto& header : response_headers)
      head.headers->AddHeader(header);
  }
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(headers_callback), error, request_url,
                                "GET", head, std::move(body)));
}

MockSignedExchangeHandler::~MockSignedExchangeHandler() {}

MockSignedExchangeHandlerFactory::MockSignedExchangeHandlerFactory(
    net::Error error,
    const GURL& request_url,
    const std::string& mime_type,
    std::vector<std::string> response_headers)
    : error_(error),
      request_url_(request_url),
      mime_type_(mime_type),
      response_headers_(std::move(response_headers)) {}

MockSignedExchangeHandlerFactory::~MockSignedExchangeHandlerFactory() = default;

std::unique_ptr<SignedExchangeHandler> MockSignedExchangeHandlerFactory::Create(
    std::unique_ptr<net::SourceStream> body,
    ExchangeHeadersCallback headers_callback,
    std::unique_ptr<SignedExchangeCertFetcherFactory> cert_fetcher_factory) {
  return std::make_unique<MockSignedExchangeHandler>(
      error_, request_url_, mime_type_, response_headers_, std::move(body),
      std::move(headers_callback));
}

}  // namespace content
