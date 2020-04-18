// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_HANDLER_H_
#define CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_HANDLER_H_

#include <string>

#include "base/callback.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/browser/web_package/signed_exchange_consts.h"
#include "content/browser/web_package/signed_exchange_header.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/completion_callback.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_result.h"
#include "net/log/net_log_with_source.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net {
class CertVerifier;
class CertVerifyResult;
class DrainableIOBuffer;
class SourceStream;
class URLRequestContextGetter;
struct OCSPVerifyResult;
}  // namespace net

namespace network {
struct ResourceResponseHead;
}  // namespace network

namespace content {

class SignedExchangeCertFetcher;
class SignedExchangeCertFetcherFactory;
class SignedExchangeCertificateChain;
class SignedExchangeDevToolsProxy;

// IMPORTANT: Currenly SignedExchangeHandler partially implements the verifying
// logic.
// TODO(https://crbug.com/803774): Implement verifying logic.
class CONTENT_EXPORT SignedExchangeHandler {
 public:
  // TODO(https://crbug.com/803774): Add verification status here.
  using ExchangeHeadersCallback = base::OnceCallback<void(
      net::Error error,
      const GURL& request_url,
      const std::string& request_method,
      const network::ResourceResponseHead&,
      std::unique_ptr<net::SourceStream> payload_stream)>;

  // TODO(https://crbug.com/817187): Find a more sophisticated way to use a
  // MockCertVerifier in browser tests instead of using the static method.
  static void SetCertVerifierForTesting(net::CertVerifier* cert_verifier);

  static void SetVerificationTimeForTesting(
      base::Optional<base::Time> verification_time_for_testing);

  // Once constructed |this| starts reading the |body| and parses the response
  // as a signed HTTP exchange. The response body of the exchange can be read
  // from |payload_stream| passed to |headers_callback|. |cert_fetcher_factory|
  // is used to create a SignedExchangeCertFetcher that fetches the certificate.
  SignedExchangeHandler(
      std::string content_type,
      std::unique_ptr<net::SourceStream> body,
      ExchangeHeadersCallback headers_callback,
      std::unique_ptr<SignedExchangeCertFetcherFactory> cert_fetcher_factory,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      std::unique_ptr<SignedExchangeDevToolsProxy> devtools_proxy);
  ~SignedExchangeHandler();

 protected:
  SignedExchangeHandler();

 private:
  enum class State {
    kReadingHeadersLength,
    kReadingHeaders,
    kFetchingCertificate,
    kHeadersCallbackCalled,
  };

  void SetupBuffers(size_t size);
  void DoHeaderLoop();
  void DidReadHeader(bool completed_syncly, int result);
  bool ParseHeadersLength();
  bool ParseHeadersAndFetchCertificate();
  void RunErrorCallback(net::Error);

  void OnCertReceived(
      std::unique_ptr<SignedExchangeCertificateChain> cert_chain);
  void OnCertVerifyComplete(int result);
  bool CheckOCSPStatus(const net::OCSPVerifyResult& ocsp_result);

  ExchangeHeadersCallback headers_callback_;
  base::Optional<SignedExchangeVersion> version_;
  std::unique_ptr<net::SourceStream> source_;

  State state_ = State::kReadingHeadersLength;
  // Buffer used for header reading.
  scoped_refptr<net::IOBuffer> header_buf_;
  // Wrapper around |header_buf_| to progressively read fixed-size data.
  scoped_refptr<net::DrainableIOBuffer> header_read_buf_;
  size_t headers_length_ = 0;

  base::Optional<SignedExchangeHeader> header_;

  std::unique_ptr<SignedExchangeCertFetcherFactory> cert_fetcher_factory_;
  std::unique_ptr<SignedExchangeCertFetcher> cert_fetcher_;

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  std::unique_ptr<SignedExchangeCertificateChain> unverified_cert_chain_;

  // CertVerifyResult must be freed after the Request has been destructed.
  // So |cert_verify_result_| must be written before |cert_verifier_request_|.
  net::CertVerifyResult cert_verify_result_;
  std::unique_ptr<net::CertVerifier::Request> cert_verifier_request_;

  // TODO(https://crbug.com/767450): figure out what we should do for NetLog
  // with Network Service.
  net::NetLogWithSource net_log_;

  std::unique_ptr<SignedExchangeDevToolsProxy> devtools_proxy_;

  base::WeakPtrFactory<SignedExchangeHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeHandler);
};

// Used only for testing.
class SignedExchangeHandlerFactory {
 public:
  virtual ~SignedExchangeHandlerFactory() {}

  virtual std::unique_ptr<SignedExchangeHandler> Create(
      std::unique_ptr<net::SourceStream> body,
      SignedExchangeHandler::ExchangeHeadersCallback headers_callback,
      std::unique_ptr<SignedExchangeCertFetcherFactory>
          cert_fetcher_factory) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_HANDLER_H_
