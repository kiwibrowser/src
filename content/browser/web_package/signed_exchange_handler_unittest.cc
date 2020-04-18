// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_handler.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "content/browser/web_package/signed_exchange_cert_fetcher_factory.h"
#include "content/browser/web_package/signed_exchange_devtools_proxy.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_paths.h"
#include "net/base/io_buffer.h"
#include "net/base/test_completion_callback.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/filter/mock_source_stream.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Property;
using testing::Return;
using testing::SetArgPointee;
using testing::Truly;

namespace content {

namespace {

const uint64_t kSignatureHeaderDate = 1520834000;
const int kOutputBufferSize = 4096;

std::string GetTestFileContents(base::StringPiece name) {
  base::FilePath path;
  base::PathService::Get(content::DIR_TEST_DATA, &path);
  path = path.AppendASCII("htxg").AppendASCII(name);

  std::string contents;
  CHECK(base::ReadFileToString(path, &contents));
  return contents;
}

scoped_refptr<net::X509Certificate> LoadCertificate(
    const std::string& cert_file) {
  base::ScopedAllowBlockingForTesting allow_io;
  return net::CreateCertificateChainFromFile(
      net::GetTestCertsDirectory(), cert_file,
      net::X509Certificate::FORMAT_PEM_CERT_SEQUENCE);
}

class MockSignedExchangeCertFetcherFactory
    : public SignedExchangeCertFetcherFactory {
 public:
  void ExpectFetch(const GURL& cert_url, const std::string& cert_str) {
    expected_cert_url_ = cert_url;
    cert_str_ = cert_str;
  }

  std::unique_ptr<SignedExchangeCertFetcher> CreateFetcherAndStart(
      const GURL& cert_url,
      bool force_fetch,
      SignedExchangeVersion version,
      SignedExchangeCertFetcher::CertificateCallback callback,
      SignedExchangeDevToolsProxy* devtools_proxy) override {
    EXPECT_EQ(cert_url, expected_cert_url_);

    auto cert_chain = SignedExchangeCertificateChain::Parse(
        version, base::as_bytes(base::make_span(cert_str_)), devtools_proxy);
    EXPECT_TRUE(cert_chain);

    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(cert_chain)));
    return nullptr;
  }

 private:
  GURL expected_cert_url_;
  std::string cert_str_;
};

class GMockCertVerifier : public net::CertVerifier {
 public:
  MOCK_METHOD6(Verify,
               int(const net::CertVerifier::RequestParams& params,
                   net::CRLSet* crl_set,
                   net::CertVerifyResult* verify_result,
                   const net::CompletionCallback& callback,
                   std::unique_ptr<net::CertVerifier::Request>* out_req,
                   const net::NetLogWithSource& net_log));
};

}  // namespace

class SignedExchangeHandlerTest
    : public ::testing::TestWithParam<net::MockSourceStream::Mode> {
 public:
  SignedExchangeHandlerTest()
      : request_initiator_(
            url::Origin::Create(GURL("https://htxg.example.com/test.htxg"))) {}

  virtual std::string ContentType() {
    return "application/signed-exchange;v=b0";
  }

  void SetUp() override {
    SignedExchangeHandler::SetVerificationTimeForTesting(
        base::Time::UnixEpoch() +
        base::TimeDelta::FromSeconds(kSignatureHeaderDate));
    feature_list_.InitAndEnableFeature(features::kSignedHTTPExchange);

    std::unique_ptr<net::MockSourceStream> source(new net::MockSourceStream());
    source->set_read_one_byte_at_a_time(true);
    source_ = source.get();
    auto cert_fetcher_factory =
        std::make_unique<MockSignedExchangeCertFetcherFactory>();
    mock_cert_fetcher_factory_ = cert_fetcher_factory.get();
    request_context_getter_ = new net::TestURLRequestContextGetter(
        scoped_task_environment_.GetMainThreadTaskRunner());
    handler_ = std::make_unique<SignedExchangeHandler>(
        ContentType(), std::move(source),
        base::BindOnce(&SignedExchangeHandlerTest::OnHeaderFound,
                       base::Unretained(this)),
        std::move(cert_fetcher_factory), request_context_getter_,
        nullptr /* devtools_proxy */);
  }

  void TearDown() override {
    SignedExchangeHandler::SetCertVerifierForTesting(nullptr);
    SignedExchangeHandler::SetVerificationTimeForTesting(
        base::Optional<base::Time>());
  }

  void SetCertVerifier(std::unique_ptr<net::CertVerifier> cert_verifier) {
    cert_verifier_ = std::move(cert_verifier);
    SignedExchangeHandler::SetCertVerifierForTesting(cert_verifier_.get());
  }

  // Reads from |stream| until an error occurs or the EOF is reached.
  // When an error occurs, returns the net error code. When an EOF is reached,
  // returns the number of bytes read. If |output| is non-null, appends data
  // read to it.
  int ReadStream(net::SourceStream* stream, std::string* output) {
    scoped_refptr<net::IOBuffer> output_buffer =
        new net::IOBuffer(kOutputBufferSize);
    int bytes_read = 0;
    while (true) {
      net::TestCompletionCallback callback;
      int rv = stream->Read(output_buffer.get(), kOutputBufferSize,
                            callback.callback());
      if (rv == net::ERR_IO_PENDING) {
        while (source_->awaiting_completion())
          source_->CompleteNextRead();
        rv = callback.WaitForResult();
      }
      if (rv == net::OK)
        break;
      if (rv < net::OK)
        return rv;
      EXPECT_GT(rv, net::OK);
      bytes_read += rv;
      if (output)
        output->append(output_buffer->data(), rv);
    }
    return bytes_read;
  }

  int ReadPayloadStream(std::string* output) {
    return ReadStream(payload_stream_.get(), output);
  }

  bool read_header() const { return read_header_; }
  net::Error error() const { return error_; }
  const network::ResourceResponseHead& resource_response() const {
    return resource_response_;
  }

  void WaitForHeader() {
    while (!read_header()) {
      while (source_->awaiting_completion())
        source_->CompleteNextRead();
      scoped_task_environment_.RunUntilIdle();
    }
  }

 protected:
  MockSignedExchangeCertFetcherFactory* mock_cert_fetcher_factory_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  net::MockSourceStream* source_;
  std::unique_ptr<SignedExchangeHandler> handler_;

 private:
  void OnHeaderFound(net::Error error,
                     const GURL&,
                     const std::string&,
                     const network::ResourceResponseHead& resource_response,
                     std::unique_ptr<net::SourceStream> payload_stream) {
    read_header_ = true;
    error_ = error;
    resource_response_ = resource_response;
    payload_stream_ = std::move(payload_stream);
  }

  base::test::ScopedFeatureList feature_list_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  const url::Origin request_initiator_;

  bool read_header_ = false;
  net::Error error_;
  network::ResourceResponseHead resource_response_;
  std::unique_ptr<net::SourceStream> payload_stream_;
};

class SignedExchangeHandlerB1Test : public SignedExchangeHandlerTest {
  std::string ContentType() override {
    return "application/signed-exchange;v=b1";
  }
};

TEST_P(SignedExchangeHandlerTest, Empty) {
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_INVALID_SIGNED_EXCHANGE, error());
}

TEST_P(SignedExchangeHandlerTest, Simple) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.msg"));

  // Make the MockCertVerifier treat the certificate "wildcard.pem" as valid for
  // "*.example.org".
  scoped_refptr<net::X509Certificate> original_cert =
      LoadCertificate("wildcard.pem");
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = original_cert;
  dummy_result.cert_status = net::OK;
  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->AddResultForCertAndHost(original_cert, "*.example.org",
                                              dummy_result, net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::OK, error());
  EXPECT_EQ(200, resource_response().headers->response_code());
  EXPECT_EQ("text/html", resource_response().mime_type);
  EXPECT_EQ("utf-8", resource_response().charset);
  EXPECT_FALSE(resource_response().load_timing.request_start_time.is_null());
  EXPECT_FALSE(resource_response().load_timing.request_start.is_null());
  EXPECT_FALSE(resource_response().load_timing.send_start.is_null());
  EXPECT_FALSE(resource_response().load_timing.send_end.is_null());
  EXPECT_FALSE(resource_response().load_timing.receive_headers_end.is_null());

  std::string payload;
  int rv = ReadPayloadStream(&payload);

  std::string expected_payload = GetTestFileContents("test.html");

  EXPECT_EQ(payload, expected_payload);
  EXPECT_EQ(rv, static_cast<int>(expected_payload.size()));
}

TEST_P(SignedExchangeHandlerTest, MimeType) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.msg"));

  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->set_default_result(net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_hello.txt.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::OK, error());
  EXPECT_EQ(200, resource_response().headers->response_code());
  EXPECT_EQ("text/plain", resource_response().mime_type);
  EXPECT_EQ("iso-8859-1", resource_response().charset);

  std::string payload;
  int rv = ReadPayloadStream(&payload);

  std::string expected_payload = GetTestFileContents("hello.txt");

  EXPECT_EQ(payload, expected_payload);
  EXPECT_EQ(rv, static_cast<int>(expected_payload.size()));
}

TEST_P(SignedExchangeHandlerTest, ParseError) {
  const uint8_t data[] = {0x00, 0x00, 0x01, 0x00};
  source_->AddReadResult(reinterpret_cast<const char*>(data), sizeof(data),
                         net::OK, GetParam());
  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_INVALID_SIGNED_EXCHANGE, error());
}

TEST_P(SignedExchangeHandlerTest, TruncatedInHeader) {
  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  contents.resize(30);
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_INVALID_SIGNED_EXCHANGE, error());
}

TEST_P(SignedExchangeHandlerTest, CertSha256Mismatch) {
  // The certificate is for "127.0.0.1". And the SHA 256 hash of the certificate
  // is different from the certSha256 of the signature in the htxg file. So the
  // certification verification must fail.
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("127.0.0.1.public.pem.msg"));

  // Set the default result of MockCertVerifier to OK, to check that the
  // verification of SignedExchange must fail even if the certificate is valid.
  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->set_default_result(net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_INVALID_SIGNED_EXCHANGE, error());
  // Drain the MockSourceStream, otherwise its destructer causes DCHECK failure.
  ReadStream(source_, nullptr);
}

TEST_P(SignedExchangeHandlerTest, VerifyCertFailure) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.msg"));

  // Make the MockCertVerifier treat the certificate "wildcard.pem" as valid for
  // "*.example.org".
  scoped_refptr<net::X509Certificate> original_cert =
      LoadCertificate("wildcard.pem");
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = original_cert;
  dummy_result.cert_status = net::OK;
  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->AddResultForCertAndHost(original_cert, "*.example.org",
                                              dummy_result, net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  // The certificate is for "*.example.com". But the request URL of the htxg
  // file is "https://test.example.com/test/". So the certification verification
  // must fail.
  std::string contents =
      GetTestFileContents("test.example.com_invalid_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_INVALID_SIGNED_EXCHANGE, error());
  // Drain the MockSourceStream, otherwise its destructer causes DCHECK failure.
  ReadStream(source_, nullptr);
}

TEST_P(SignedExchangeHandlerB1Test, Simple) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.cbor"));

  // Make the MockCertVerifier treat the certificate "wildcard.pem" as valid for
  // "*.example.org".
  scoped_refptr<net::X509Certificate> original_cert =
      LoadCertificate("wildcard.pem");
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = original_cert;
  dummy_result.cert_status = net::OK;
  dummy_result.ocsp_result.response_status = net::OCSPVerifyResult::PROVIDED;
  dummy_result.ocsp_result.revocation_status = net::OCSPRevocationStatus::GOOD;
  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->AddResultForCertAndHost(original_cert, "*.example.org",
                                              dummy_result, net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::OK, error());
  EXPECT_EQ(200, resource_response().headers->response_code());
  EXPECT_EQ("text/html", resource_response().mime_type);
  EXPECT_EQ("utf-8", resource_response().charset);
  EXPECT_FALSE(resource_response().load_timing.request_start_time.is_null());
  EXPECT_FALSE(resource_response().load_timing.request_start.is_null());
  EXPECT_FALSE(resource_response().load_timing.send_start.is_null());
  EXPECT_FALSE(resource_response().load_timing.send_end.is_null());
  EXPECT_FALSE(resource_response().load_timing.receive_headers_end.is_null());

  std::string payload;
  int rv = ReadPayloadStream(&payload);

  std::string expected_payload = GetTestFileContents("test.html");

  EXPECT_EQ(payload, expected_payload);
  EXPECT_EQ(rv, static_cast<int>(expected_payload.size()));
}

TEST_P(SignedExchangeHandlerB1Test, OCSPNotChecked) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.cbor"));

  // Make the MockCertVerifier treat the certificate "wildcard.pem" as valid for
  // "*.example.org".
  scoped_refptr<net::X509Certificate> original_cert =
      LoadCertificate("wildcard.pem");
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = original_cert;
  dummy_result.cert_status = net::OK;
  dummy_result.ocsp_result.response_status = net::OCSPVerifyResult::NOT_CHECKED;
  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->AddResultForCertAndHost(original_cert, "*.example.org",
                                              dummy_result, net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_FAILED, error());
  // Drain the MockSourceStream, otherwise its destructer causes DCHECK failure.
  ReadStream(source_, nullptr);
}

TEST_P(SignedExchangeHandlerB1Test, OCSPNotProvided) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.cbor"));

  // Make the MockCertVerifier treat the certificate "wildcard.pem" as valid for
  // "*.example.org".
  scoped_refptr<net::X509Certificate> original_cert =
      LoadCertificate("wildcard.pem");
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = original_cert;
  dummy_result.cert_status = net::OK;
  dummy_result.ocsp_result.response_status = net::OCSPVerifyResult::MISSING;
  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->AddResultForCertAndHost(original_cert, "*.example.org",
                                              dummy_result, net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_FAILED, error());
  // Drain the MockSourceStream, otherwise its destructer causes DCHECK failure.
  ReadStream(source_, nullptr);
}

TEST_P(SignedExchangeHandlerB1Test, OCSPInvalid) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.cbor"));

  // Make the MockCertVerifier treat the certificate "wildcard.pem" as valid for
  // "*.example.org".
  scoped_refptr<net::X509Certificate> original_cert =
      LoadCertificate("wildcard.pem");
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = original_cert;
  dummy_result.cert_status = net::OK;
  dummy_result.ocsp_result.response_status =
      net::OCSPVerifyResult::INVALID_DATE;
  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->AddResultForCertAndHost(original_cert, "*.example.org",
                                              dummy_result, net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_FAILED, error());
  // Drain the MockSourceStream, otherwise its destructer causes DCHECK failure.
  ReadStream(source_, nullptr);
}

TEST_P(SignedExchangeHandlerB1Test, OCSPRevoked) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.cbor"));

  // Make the MockCertVerifier treat the certificate "wildcard.pem" as valid for
  // "*.example.org".
  scoped_refptr<net::X509Certificate> original_cert =
      LoadCertificate("wildcard.pem");
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = original_cert;
  dummy_result.cert_status = net::OK;
  dummy_result.ocsp_result.response_status = net::OCSPVerifyResult::PROVIDED;
  dummy_result.ocsp_result.revocation_status =
      net::OCSPRevocationStatus::REVOKED;
  auto mock_cert_verifier = std::make_unique<net::MockCertVerifier>();
  mock_cert_verifier->AddResultForCertAndHost(original_cert, "*.example.org",
                                              dummy_result, net::OK);
  SetCertVerifier(std::move(mock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::ERR_FAILED, error());
  // Drain the MockSourceStream, otherwise its destructer causes DCHECK failure.
  ReadStream(source_, nullptr);
}

// Test that fetching a signed exchange properly extracts and
// attempts to verify both the certificate and the OCSP response.
TEST_P(SignedExchangeHandlerB1Test, CertVerifierParams) {
  mock_cert_fetcher_factory_->ExpectFetch(
      GURL("https://cert.example.org/cert.msg"),
      GetTestFileContents("wildcard_example.org.public.pem.cbor"));

  scoped_refptr<net::X509Certificate> original_cert =
      LoadCertificate("wildcard.pem");
  net::CertVerifyResult fake_result;
  fake_result.verified_cert = original_cert;
  fake_result.cert_status = net::OK;
  fake_result.ocsp_result.response_status = net::OCSPVerifyResult::PROVIDED;
  fake_result.ocsp_result.revocation_status = net::OCSPRevocationStatus::GOOD;

  // "wildcard_example.org.public.pem.cbor" has this dummy data instead of a
  // real OCSP response.
  constexpr base::StringPiece kExpectedOCSPDer = "OCSP";

  std::unique_ptr<GMockCertVerifier> gmock_cert_verifier =
      std::make_unique<GMockCertVerifier>();
  EXPECT_CALL(
      *gmock_cert_verifier,
      Verify(
          AllOf(Property(&net::CertVerifier::RequestParams::ocsp_response,
                         kExpectedOCSPDer),
                Property(
                    &net::CertVerifier::RequestParams::certificate,
                    Truly([&original_cert](
                              const scoped_refptr<net::X509Certificate>& cert) {
                      return original_cert->EqualsIncludingChain(cert.get());
                    })),
                Property(&net::CertVerifier::RequestParams::hostname,
                         "test.example.org")),
          _ /* crl_set */, _ /* verify_result */, _ /* callback */,
          _ /* out_req */, _ /* net_log */
          ))
      .WillOnce(DoAll(SetArgPointee<2>(fake_result), Return(net::OK)));
  SetCertVerifier(std::move(gmock_cert_verifier));

  std::string contents = GetTestFileContents("test.example.org_test.htxg");
  source_->AddReadResult(contents.data(), contents.size(), net::OK, GetParam());
  source_->AddReadResult(nullptr, 0, net::OK, GetParam());

  WaitForHeader();

  ASSERT_TRUE(read_header());
  EXPECT_EQ(net::OK, error());
  std::string payload;
  int rv = ReadPayloadStream(&payload);
  std::string expected_payload = GetTestFileContents("test.html");

  EXPECT_EQ(expected_payload, payload);
  EXPECT_EQ(static_cast<int>(expected_payload.size()), rv);
}

INSTANTIATE_TEST_CASE_P(SignedExchangeHandlerTests,
                        SignedExchangeHandlerTest,
                        ::testing::Values(net::MockSourceStream::SYNC,
                                          net::MockSourceStream::ASYNC));

INSTANTIATE_TEST_CASE_P(SignedExchangeHandlerB1Tests,
                        SignedExchangeHandlerB1Test,
                        ::testing::Values(net::MockSourceStream::SYNC,
                                          net::MockSourceStream::ASYNC));

}  // namespace content
