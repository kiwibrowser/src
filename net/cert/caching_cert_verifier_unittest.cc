// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/caching_cert_verifier.h"

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/log/net_log_with_source.h"
#include "net/test/cert_test_util.h"
#include "net/test/gtest_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsError;
using net::test::IsOk;

using testing::_;
using testing::Mock;
using testing::Return;
using testing::ReturnRef;

namespace net {

namespace {

class MockCacheVisitor : public CachingCertVerifier::CacheVisitor {
 public:
  MockCacheVisitor() = default;
  ~MockCacheVisitor() override = default;

  MOCK_METHOD5(VisitEntry,
               bool(const CachingCertVerifier::RequestParams& params,
                    int error,
                    const CertVerifyResult& result,
                    base::Time verification_time,
                    base::Time expiration_time));
};

}  // namespace

class CachingCertVerifierTest : public ::testing::Test {
 public:
  CachingCertVerifierTest() : verifier_(std::make_unique<MockCertVerifier>()) {}
  ~CachingCertVerifierTest() override = default;

 protected:
  CachingCertVerifier verifier_;
};

TEST_F(CachingCertVerifierTest, CacheHit) {
  base::FilePath certs_dir = GetTestCertsDirectory();
  scoped_refptr<X509Certificate> test_cert(
      ImportCertFromFile(certs_dir, "ok_cert.pem"));
  ASSERT_TRUE(test_cert.get());

  int error;
  CertVerifyResult verify_result;
  TestCompletionCallback callback;
  std::unique_ptr<CertVerifier::Request> request;

  error = callback.GetResult(verifier_.Verify(
      CertVerifier::RequestParams(test_cert, "www.example.com", 0,
                                  std::string(), CertificateList()),
      nullptr, &verify_result, callback.callback(), &request,
      NetLogWithSource()));
  ASSERT_TRUE(IsCertificateError(error));
  ASSERT_EQ(1u, verifier_.requests());
  ASSERT_EQ(0u, verifier_.cache_hits());
  ASSERT_EQ(1u, verifier_.GetCacheSize());

  error = verifier_.Verify(
      CertVerifier::RequestParams(test_cert, "www.example.com", 0,
                                  std::string(), CertificateList()),
      nullptr, &verify_result, callback.callback(), &request,
      NetLogWithSource());
  // Synchronous completion.
  ASSERT_NE(ERR_IO_PENDING, error);
  ASSERT_TRUE(IsCertificateError(error));
  ASSERT_FALSE(request);
  ASSERT_EQ(2u, verifier_.requests());
  ASSERT_EQ(1u, verifier_.cache_hits());
  ASSERT_EQ(1u, verifier_.GetCacheSize());
}

TEST_F(CachingCertVerifierTest, Visitor) {
  base::FilePath certs_dir = GetTestCertsDirectory();
  scoped_refptr<X509Certificate> test_cert(
      ImportCertFromFile(certs_dir, "ok_cert.pem"));
  ASSERT_TRUE(test_cert.get());

  TestCompletionCallback callback;
  std::unique_ptr<CertVerifier::Request> request;

  // Add some entries to the cache
  CertVerifier::RequestParams params1(test_cert, "www.example.com", 0,
                                      std::string(), CertificateList());
  CertVerifyResult result1;
  int error1 = callback.GetResult(
      verifier_.Verify(params1, nullptr, &result1, callback.callback(),
                       &request, NetLogWithSource()));
  ASSERT_TRUE(IsCertificateError(error1));
  ASSERT_EQ(1u, verifier_.requests());
  ASSERT_EQ(0u, verifier_.cache_hits());
  ASSERT_EQ(1u, verifier_.GetCacheSize());

  CertVerifier::RequestParams params2(test_cert, "www.example.net", 0,
                                      std::string(), CertificateList());
  CertVerifyResult result2;
  int error2 = callback.GetResult(
      verifier_.Verify(params2, nullptr, &result2, callback.callback(),
                       &request, NetLogWithSource()));
  ASSERT_TRUE(IsCertificateError(error2));
  ASSERT_EQ(2u, verifier_.requests());
  ASSERT_EQ(0u, verifier_.cache_hits());
  ASSERT_EQ(2u, verifier_.GetCacheSize());

  CertVerifier::RequestParams params3(test_cert, "www.example.org", 0,
                                      std::string(), CertificateList());
  CertVerifyResult result3;
  int error3 = callback.GetResult(
      verifier_.Verify(params3, nullptr, &result3, callback.callback(),
                       &request, NetLogWithSource()));
  ASSERT_TRUE(IsCertificateError(error3));
  ASSERT_EQ(3u, verifier_.requests());
  ASSERT_EQ(0u, verifier_.cache_hits());
  ASSERT_EQ(3u, verifier_.GetCacheSize());

  // Iterate through all entries.
  {
    MockCacheVisitor mock_visitor;
    EXPECT_CALL(mock_visitor, VisitEntry(params1, error1, _, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_visitor, VisitEntry(params2, error2, _, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_visitor, VisitEntry(params3, error3, _, _, _))
        .WillOnce(Return(true));
    verifier_.VisitEntries(&mock_visitor);
  }

  // Now perform partial iteration
  {
    MockCacheVisitor mock_visitor;
    ::testing::InSequence sequence;
    EXPECT_CALL(mock_visitor, VisitEntry(_, _, _, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mock_visitor, VisitEntry(_, _, _, _, _))
        .WillOnce(Return(false));
    verifier_.VisitEntries(&mock_visitor);
  }
}

TEST_F(CachingCertVerifierTest, AddsEntries) {
  base::FilePath certs_dir = GetTestCertsDirectory();
  scoped_refptr<X509Certificate> test_cert(
      ImportCertFromFile(certs_dir, "ok_cert.pem"));
  ASSERT_TRUE(test_cert.get());

  CertVerifyResult result_1;
  result_1.verified_cert = test_cert;
  result_1.cert_status = CERT_STATUS_WEAK_SIGNATURE_ALGORITHM;
  result_1.has_md2 = true;
  result_1.is_issued_by_known_root = false;

  CertVerifyResult result_2;
  result_2.verified_cert = test_cert;
  result_2.cert_status = CERT_STATUS_IS_EV;
  result_2.is_issued_by_known_root = true;

  CertVerifier::RequestParams params(test_cert, "www.example.com", 0,
                                     std::string(), CertificateList());

  base::Time now = base::Time::Now();

  // On an empty cache, it should be fine to add an entry.
  EXPECT_TRUE(verifier_.AddEntry(params, ERR_CERT_WEAK_KEY, result_1, now));
  ASSERT_EQ(0u, verifier_.requests());
  ASSERT_EQ(0u, verifier_.cache_hits());
  ASSERT_EQ(1u, verifier_.GetCacheSize());

  TestCompletionCallback callback;
  std::unique_ptr<CertVerifier::Request> request;

  CertVerifyResult cached_result;
  int error = callback.GetResult(
      verifier_.Verify(params, nullptr, &cached_result, callback.callback(),
                       &request, NetLogWithSource()));
  ASSERT_THAT(error, IsError(ERR_CERT_WEAK_KEY));
  EXPECT_TRUE(cached_result.has_md2);
  EXPECT_FALSE(cached_result.is_issued_by_known_root);

  ASSERT_EQ(1u, verifier_.requests());
  ASSERT_EQ(1u, verifier_.cache_hits());
  ASSERT_EQ(1u, verifier_.GetCacheSize());

  // But it should not be fine to replace it with an existing entry, even
  // if that entry is 'newer'.
  EXPECT_FALSE(verifier_.AddEntry(params, OK, result_2,
                                  now + base::TimeDelta::FromMinutes(1)));

  error = callback.GetResult(verifier_.Verify(params, nullptr, &cached_result,
                                              callback.callback(), &request,
                                              NetLogWithSource()));
  ASSERT_THAT(error, IsError(ERR_CERT_WEAK_KEY));
  EXPECT_TRUE(cached_result.has_md2);
  EXPECT_FALSE(cached_result.is_issued_by_known_root);

  ASSERT_EQ(2u, verifier_.requests());
  ASSERT_EQ(2u, verifier_.cache_hits());
  ASSERT_EQ(1u, verifier_.GetCacheSize());
}

// Tests the same server certificate with different intermediate CA
// certificates.  These should be treated as different certificate chains even
// though the two X509Certificate objects contain the same server certificate.
TEST_F(CachingCertVerifierTest, DifferentCACerts) {
  base::FilePath certs_dir = GetTestCertsDirectory();

  scoped_refptr<X509Certificate> server_cert =
      ImportCertFromFile(certs_dir, "salesforce_com_test.pem");
  ASSERT_TRUE(server_cert);

  scoped_refptr<X509Certificate> intermediate_cert1 =
      ImportCertFromFile(certs_dir, "verisign_intermediate_ca_2011.pem");
  ASSERT_TRUE(intermediate_cert1);

  scoped_refptr<X509Certificate> intermediate_cert2 =
      ImportCertFromFile(certs_dir, "verisign_intermediate_ca_2016.pem");
  ASSERT_TRUE(intermediate_cert2);

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(
      x509_util::DupCryptoBuffer(intermediate_cert1->cert_buffer()));
  scoped_refptr<X509Certificate> cert_chain1 =
      X509Certificate::CreateFromBuffer(
          x509_util::DupCryptoBuffer(server_cert->cert_buffer()),
          std::move(intermediates));
  ASSERT_TRUE(cert_chain1);

  intermediates.clear();
  intermediates.push_back(
      x509_util::DupCryptoBuffer(intermediate_cert2->cert_buffer()));
  scoped_refptr<X509Certificate> cert_chain2 =
      X509Certificate::CreateFromBuffer(
          x509_util::DupCryptoBuffer(server_cert->cert_buffer()),
          std::move(intermediates));
  ASSERT_TRUE(cert_chain2);

  int error;
  CertVerifyResult verify_result;
  TestCompletionCallback callback;
  std::unique_ptr<CertVerifier::Request> request;

  error = callback.GetResult(verifier_.Verify(
      CertVerifier::RequestParams(cert_chain1, "www.example.com", 0,
                                  std::string(), CertificateList()),
      nullptr, &verify_result, callback.callback(), &request,
      NetLogWithSource()));
  ASSERT_TRUE(IsCertificateError(error));
  ASSERT_EQ(1u, verifier_.requests());
  ASSERT_EQ(0u, verifier_.cache_hits());
  ASSERT_EQ(1u, verifier_.GetCacheSize());

  error = callback.GetResult(verifier_.Verify(
      CertVerifier::RequestParams(cert_chain2, "www.example.com", 0,
                                  std::string(), CertificateList()),
      nullptr, &verify_result, callback.callback(), &request,
      NetLogWithSource()));
  ASSERT_TRUE(IsCertificateError(error));
  ASSERT_EQ(2u, verifier_.requests());
  ASSERT_EQ(0u, verifier_.cache_hits());
  ASSERT_EQ(2u, verifier_.GetCacheSize());
}

}  // namespace net
