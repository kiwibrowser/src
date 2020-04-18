// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_handler.h"

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/loader/merkle_integrity_source_stream.h"
#include "content/browser/web_package/signed_exchange_cert_fetcher_factory.h"
#include "content/browser/web_package/signed_exchange_certificate_chain.h"
#include "content/browser/web_package/signed_exchange_devtools_proxy.h"
#include "content/browser/web_package/signed_exchange_header.h"
#include "content/browser/web_package/signed_exchange_signature_verifier.h"
#include "content/browser/web_package/signed_exchange_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_loader_throttle.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/x509_certificate.h"
#include "net/filter/source_stream.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"
#include "net/ssl/ssl_info.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

namespace {

// 256KB (Maximum header size) * 2, since signed exchange header contains
// request and response headers.
constexpr size_t kMaxHeadersCBORLength = 512 * 1024;

constexpr char kMiHeader[] = "MI";

net::CertVerifier* g_cert_verifier_for_testing = nullptr;

base::Optional<base::Time> g_verification_time_for_testing;

base::Time GetVerificationTime() {
  if (g_verification_time_for_testing)
    return *g_verification_time_for_testing;
  return base::Time::Now();
}

}  // namespace

// static
void SignedExchangeHandler::SetCertVerifierForTesting(
    net::CertVerifier* cert_verifier) {
  g_cert_verifier_for_testing = cert_verifier;
}

// static
void SignedExchangeHandler::SetVerificationTimeForTesting(
    base::Optional<base::Time> verification_time_for_testing) {
  g_verification_time_for_testing = verification_time_for_testing;
}

SignedExchangeHandler::SignedExchangeHandler(
    std::string content_type,
    std::unique_ptr<net::SourceStream> body,
    ExchangeHeadersCallback headers_callback,
    std::unique_ptr<SignedExchangeCertFetcherFactory> cert_fetcher_factory,
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    std::unique_ptr<SignedExchangeDevToolsProxy> devtools_proxy)
    : headers_callback_(std::move(headers_callback)),
      source_(std::move(body)),
      cert_fetcher_factory_(std::move(cert_fetcher_factory)),
      request_context_getter_(std::move(request_context_getter)),
      net_log_(net::NetLogWithSource::Make(
          request_context_getter_->GetURLRequestContext()->net_log(),
          net::NetLogSourceType::CERT_VERIFIER_JOB)),
      devtools_proxy_(std::move(devtools_proxy)),
      weak_factory_(this) {
  DCHECK(signed_exchange_utils::IsSignedExchangeHandlingEnabled());
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHandler::SignedExchangeHandler");

  if (!SignedExchangeHeaderParser::GetVersionParamFromContentType(content_type,
                                                                  &version_) ||
      (version_ != SignedExchangeVersion::kB0 &&
       version_ != SignedExchangeVersion::kB1)) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&SignedExchangeHandler::RunErrorCallback,
                                  weak_factory_.GetWeakPtr(),
                                  net::ERR_INVALID_SIGNED_EXCHANGE));
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::SignedExchangeHandler",
        base::StringPrintf("Unsupported version of the content type. Currentry "
                           "content type must be "
                           "\"application/signed-exchange;v={b0,b1}\". But the "
                           "response content type was \"%s\"",
                           content_type.c_str()));
    return;
  }

  // Triggering the read (asynchronously) for the encoded header length.
  SetupBuffers(SignedExchangeHeader::kEncodedLengthInBytes);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&SignedExchangeHandler::DoHeaderLoop,
                                weak_factory_.GetWeakPtr()));
  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeHandler::SignedExchangeHandler");
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

SignedExchangeHandler::SignedExchangeHandler() : weak_factory_(this) {}

void SignedExchangeHandler::SetupBuffers(size_t size) {
  header_buf_ = base::MakeRefCounted<net::IOBuffer>(size);
  header_read_buf_ =
      base::MakeRefCounted<net::DrainableIOBuffer>(header_buf_.get(), size);
}

void SignedExchangeHandler::DoHeaderLoop() {
  DCHECK(state_ == State::kReadingHeadersLength ||
         state_ == State::kReadingHeaders);
  int rv = source_->Read(
      header_read_buf_.get(), header_read_buf_->BytesRemaining(),
      base::BindRepeating(&SignedExchangeHandler::DidReadHeader,
                          base::Unretained(this), false /* sync */));
  if (rv != net::ERR_IO_PENDING)
    DidReadHeader(true /* sync */, rv);
}

void SignedExchangeHandler::DidReadHeader(bool completed_syncly, int result) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHandler::DidReadHeader");
  if (result < 0) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::DidReadHeader",
        base::StringPrintf("Error reading body stream. result: %d", result));
    RunErrorCallback(static_cast<net::Error>(result));
    return;
  }

  if (result == 0) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::DidReadHeader",
        "Stream ended while reading signed exchange header.");
    RunErrorCallback(net::ERR_INVALID_SIGNED_EXCHANGE);
    return;
  }

  header_read_buf_->DidConsume(result);
  if (header_read_buf_->BytesRemaining() == 0) {
    switch (state_) {
      case State::kReadingHeadersLength:
        if (!ParseHeadersLength())
          RunErrorCallback(net::ERR_INVALID_SIGNED_EXCHANGE);
        break;
      case State::kReadingHeaders:
        if (!ParseHeadersAndFetchCertificate())
          RunErrorCallback(net::ERR_INVALID_SIGNED_EXCHANGE);
        break;
      default:
        NOTREACHED();
    }
  }

  if (state_ != State::kReadingHeadersLength &&
      state_ != State::kReadingHeaders) {
    TRACE_EVENT_END1(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHandler::DidReadHeader", "state",
                     static_cast<int>(state_));
    return;
  }

  // Trigger the next read.
  if (completed_syncly) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&SignedExchangeHandler::DoHeaderLoop,
                                  weak_factory_.GetWeakPtr()));
  } else {
    DoHeaderLoop();
  }
  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeHandler::DidReadHeader");
}

bool SignedExchangeHandler::ParseHeadersLength() {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHandler::ParseEncodedLength");
  DCHECK_EQ(state_, State::kReadingHeadersLength);

  headers_length_ = SignedExchangeHeader::ParseEncodedLength(
      base::make_span(reinterpret_cast<uint8_t*>(header_buf_->data()),
                      SignedExchangeHeader::kEncodedLengthInBytes));
  if (headers_length_ == 0 || headers_length_ > kMaxHeadersCBORLength) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::ParseEncodedLength",
        base::StringPrintf("Invalid CBOR header length: %zu", headers_length_));
    return false;
  }

  // Set up a new buffer for CBOR-encoded buffer reading.
  SetupBuffers(headers_length_);
  state_ = State::kReadingHeaders;
  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeHandler::ParseEncodedLength");
  return true;
}

bool SignedExchangeHandler::ParseHeadersAndFetchCertificate() {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHandler::ParseHeadersAndFetchCertificate");
  DCHECK_EQ(state_, State::kReadingHeaders);

  header_ = SignedExchangeHeader::Parse(
      base::make_span(reinterpret_cast<uint8_t*>(header_buf_->data()),
                      headers_length_),
      devtools_proxy_.get());
  header_read_buf_ = nullptr;
  header_buf_ = nullptr;
  if (!header_) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(),
        "SignedExchangeHandler::ParseHeadersAndFetchCertificate",
        "Failed to parse SignedExchange header.");
    return false;
  }

  const GURL cert_url = header_->signature().cert_url;
  // TODO(https://crbug.com/819467): When we will support ed25519Key, |cert_url|
  // may be empty.
  DCHECK(cert_url.is_valid());
  DCHECK(version_.has_value());

  DCHECK(cert_fetcher_factory_);
  cert_fetcher_ = std::move(cert_fetcher_factory_)
                      ->CreateFetcherAndStart(
                          cert_url, false, *version_,
                          base::BindOnce(&SignedExchangeHandler::OnCertReceived,
                                         base::Unretained(this)),
                          devtools_proxy_.get());

  state_ = State::kFetchingCertificate;
  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeHandler::ParseHeadersAndFetchCertificate");
  return true;
}

void SignedExchangeHandler::RunErrorCallback(net::Error error) {
  DCHECK_NE(state_, State::kHeadersCallbackCalled);
  if (devtools_proxy_)
    devtools_proxy_->OnSignedExchangeReceived(header_, nullptr);
  std::move(headers_callback_)
      .Run(error, GURL(), std::string(), network::ResourceResponseHead(),
           nullptr);
  state_ = State::kHeadersCallbackCalled;
}

void SignedExchangeHandler::OnCertReceived(
    std::unique_ptr<SignedExchangeCertificateChain> cert_chain) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHandler::OnCertReceived");
  DCHECK_EQ(state_, State::kFetchingCertificate);
  if (!cert_chain) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::OnCertReceived",
        "Failed to fetch the certificate.");
    RunErrorCallback(net::ERR_INVALID_SIGNED_EXCHANGE);
    return;
  }

  if (SignedExchangeSignatureVerifier::Verify(*header_, cert_chain->cert(),
                                              GetVerificationTime(),
                                              devtools_proxy_.get()) !=
      SignedExchangeSignatureVerifier::Result::kSuccess) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::OnCertReceived",
        "Failed to verify the signed exchange header.");
    RunErrorCallback(net::ERR_INVALID_SIGNED_EXCHANGE);
    return;
  }
  net::URLRequestContext* request_context =
      request_context_getter_->GetURLRequestContext();
  if (!request_context) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::OnCertReceived",
        "No request context available.");
    RunErrorCallback(net::ERR_CONTEXT_SHUT_DOWN);
    return;
  }

  unverified_cert_chain_ = std::move(cert_chain);

  net::SSLConfig config;
  request_context->ssl_config_service()->GetSSLConfig(&config);

  net::CertVerifier* cert_verifier = g_cert_verifier_for_testing
                                         ? g_cert_verifier_for_testing
                                         : request_context->cert_verifier();
  int result = cert_verifier->Verify(
      net::CertVerifier::RequestParams(
          unverified_cert_chain_->cert(), header_->request_url().host(),
          config.GetCertVerifyFlags(), unverified_cert_chain_->ocsp(),
          net::CertificateList()),
      net::SSLConfigService::GetCRLSet().get(), &cert_verify_result_,
      base::BindRepeating(&SignedExchangeHandler::OnCertVerifyComplete,
                          base::Unretained(this)),
      &cert_verifier_request_, net_log_);
  // TODO(https://crbug.com/803774): Avoid these recursive patterns by using
  // explicit state machines (eg: DoLoop() in //net).
  if (result != net::ERR_IO_PENDING)
    OnCertVerifyComplete(result);

  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeHandler::OnCertReceived");
}

bool SignedExchangeHandler::CheckOCSPStatus(
    const net::OCSPVerifyResult& ocsp_result) {
  // https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#cross-origin-trust
  // Step 6.3 Validate that main-certificate has an ocsp property (Section 3.3)
  // with a valid OCSP response whose lifetime (nextUpdate - thisUpdate) is less
  // than 7 days ([RFC6960]). [spec text]
  //
  // OCSP verification is done in CertVerifier::Verify(), so we just check the
  // result here.

  // The b0 implementation checkpoint has no OCSP check.
  if (version_ == SignedExchangeVersion::kB0)
    return true;

  if (ocsp_result.response_status != net::OCSPVerifyResult::PROVIDED ||
      ocsp_result.revocation_status != net::OCSPRevocationStatus::GOOD)
    return false;

  return true;
}

void SignedExchangeHandler::OnCertVerifyComplete(int result) {
  TRACE_EVENT_BEGIN0(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "SignedExchangeHandler::OnCertVerifyComplete");

  if (result != net::OK) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::OnCertVerifyComplete",
        base::StringPrintf("Certificate verification error: %s",
                           net::ErrorToShortString(result).c_str()));
    RunErrorCallback(net::ERR_INVALID_SIGNED_EXCHANGE);
    return;
  }

  if (!CheckOCSPStatus(cert_verify_result_.ocsp_result)) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::OnCertVerifyComplete",
        base::StringPrintf(
            "OCSP check failed. response status: %d, revocation status: %d",
            cert_verify_result_.ocsp_result.response_status,
            cert_verify_result_.ocsp_result.revocation_status));
    RunErrorCallback(static_cast<net::Error>(net::ERR_FAILED));
    return;
  }

  network::ResourceResponseHead response_head;
  response_head.headers = header_->BuildHttpResponseHeaders();
  response_head.headers->GetMimeTypeAndCharset(&response_head.mime_type,
                                               &response_head.charset);

  // TODO(https://crbug.com/803774): Resource timing for signed exchange
  // loading is not speced yet. https://github.com/WICG/webpackage/issues/156
  response_head.load_timing.request_start_time = base::Time::Now();
  base::TimeTicks now(base::TimeTicks::Now());
  response_head.load_timing.request_start = now;
  response_head.load_timing.send_start = now;
  response_head.load_timing.send_end = now;
  response_head.load_timing.receive_headers_end = now;

  std::string mi_header_value;
  if (!response_head.headers->EnumerateHeader(nullptr, kMiHeader,
                                              &mi_header_value)) {
    signed_exchange_utils::ReportErrorAndEndTraceEvent(
        devtools_proxy_.get(), "SignedExchangeHandler::OnCertVerifyComplete",
        "Signed exchange has no MI: header");
    RunErrorCallback(net::ERR_INVALID_SIGNED_EXCHANGE);
    return;
  }
  auto mi_stream = std::make_unique<MerkleIntegritySourceStream>(
      mi_header_value, std::move(source_));

  net::SSLInfo ssl_info;
  ssl_info.cert = cert_verify_result_.verified_cert;
  ssl_info.unverified_cert = unverified_cert_chain_->cert();
  ssl_info.cert_status = cert_verify_result_.cert_status;
  ssl_info.is_issued_by_known_root =
      cert_verify_result_.is_issued_by_known_root;
  ssl_info.public_key_hashes = cert_verify_result_.public_key_hashes;
  ssl_info.ocsp_result = cert_verify_result_.ocsp_result;
  ssl_info.is_fatal_cert_error =
      net::IsCertStatusError(ssl_info.cert_status) &&
      !net::IsCertStatusMinorError(ssl_info.cert_status);

  if (devtools_proxy_)
    devtools_proxy_->OnSignedExchangeReceived(header_, &ssl_info);

  response_head.ssl_info = std::move(ssl_info);
  // TODO(https://crbug.com/815025): Verify the Certificate Transparency status.
  std::move(headers_callback_)
      .Run(net::OK, header_->request_url(), header_->request_method(),
           response_head, std::move(mi_stream));
  state_ = State::kHeadersCallbackCalled;
  TRACE_EVENT_END0(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "SignedExchangeHandler::OnCertVerifyComplete");
}

}  // namespace content
