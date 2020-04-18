// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/android/cert/cert_verifier_cache_serializer.h"

#include <memory>
#include <string>

#include "base/android/path_utils.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "components/cronet/android/cert/proto/cert_verification.pb.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/cert/caching_cert_verifier.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/log/net_log_with_source.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cronet {

namespace {

// Helper function that verifies the cerificate with the given |cert_name|
// against the given |hostname| using the given |verifier|. Result from the cert
// verification is ignored.
void VerifyCert(const std::string& cert_name,
                const std::string& hostname,
                net::CachingCertVerifier* verifier,
                net::CertVerifyResult* verify_result) {
  // Set up server certs.
  scoped_refptr<net::X509Certificate> cert(
      net::ImportCertFromFile(net::GetTestCertsDirectory(), cert_name));
  ASSERT_TRUE(cert);

  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;

  ignore_result(callback.GetResult(verifier->Verify(
      net::CertVerifier::RequestParams(cert.get(), hostname, 0, std::string(),
                                       net::CertificateList()),
      nullptr, verify_result, callback.callback(), &request,
      net::NetLogWithSource())));
}

}  // namespace

TEST(CertVerifierCacheSerializerTest, RestoreEmptyData) {
  // Restoring empty data should fail.
  cronet_pb::CertVerificationCache cert_cache;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier));
}

TEST(CertVerifierCacheSerializerTest, SerializeCache) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  // Verify atleast one certificate is serialized.
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());
}

// Create a new Verifier and restoring the data into it should succeed.
TEST(CertVerifierCacheSerializerTest, RestoreMultipleEntriesIntoNewVerifier) {
  scoped_refptr<net::X509Certificate> ok_cert(
      net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(ok_cert);

  const scoped_refptr<net::X509Certificate> root_cert =
      net::ImportCertFromFile(net::GetTestCertsDirectory(), "root_ca_cert.pem");
  ASSERT_TRUE(root_cert);

  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;

  // Verify www.example.com host's certificate.
  std::string example_hostname("www.example.com");
  net::CertVerifyResult verifier1_result1;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(VerifyCert("ok_cert.pem", example_hostname, &verifier,
                                     &verifier1_result1));

  // Verify www2.example.com host's certificate.
  std::string example2_hostname("www2.example.com");
  net::CertVerifyResult verifier1_result2;

  // Create a certificate that contains both a leaf and an intermediate/root and
  // use that certificate for www2.example.com.
  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> chain;
  chain.push_back(net::x509_util::DupCryptoBuffer(root_cert->cert_buffer()));
  const scoped_refptr<net::X509Certificate> combined_cert =
      net::X509Certificate::CreateFromBuffer(
          net::x509_util::DupCryptoBuffer(ok_cert->cert_buffer()),
          std::move(chain));
  ASSERT_TRUE(combined_cert);

  ignore_result(callback.GetResult(verifier.Verify(
      net::CertVerifier::RequestParams(combined_cert, example2_hostname, 0,
                                       std::string(), net::CertificateList()),
      nullptr, &verifier1_result2, callback.callback(), &request,
      net::NetLogWithSource())));

  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  // Verify two certificates are serialized.
  DCHECK_EQ(2, cert_cache.cert_entry_size());
  DCHECK_EQ(2, cert_cache.cache_entry_size());

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());

  // Populate |verifier2|'s cache.
  EXPECT_TRUE(DeserializeCertVerifierCache(cert_cache, &verifier2));

  // Verify the cert for www.example.com with |verifier2|.
  net::CertVerifyResult verifier2_result1;
  ASSERT_NO_FATAL_FAILURE(VerifyCert("ok_cert.pem", example_hostname,
                                     &verifier2, &verifier2_result1));

  // CertVerifyResult for www.example.com with |verifier2| should match
  // what was serialized with |verifier|.
  EXPECT_EQ(verifier2_result1, verifier1_result1);

  // Verify the cert for www2.example.com with |verifier2|.
  net::CertVerifyResult verifier2_result2;
  ignore_result(callback.GetResult(verifier2.Verify(
      net::CertVerifier::RequestParams(combined_cert, example2_hostname, 0,
                                       std::string(), net::CertificateList()),
      nullptr, &verifier2_result2, callback.callback(), &request,
      net::NetLogWithSource())));

  // CertVerifyResult for www2.example.com with |verifier2| should match
  // what was serialized with |verifier|.
  EXPECT_EQ(verifier2_result2, verifier1_result2);
}

// A corrupted cert_entry in the serialized data should fail to be deserialized.
// Should not deserialize a corrupted cert_entry.
TEST(CertVerifierCacheSerializerTest, DeserializeCorruptedCerts) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  cert_cache.clear_cert_entry();

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |cache_entry| in the serialized data should fail to be
// deserialized. Should not deserialize a corrupted |cert_entry|.
TEST(CertVerifierCacheSerializerTest, DeserializeCorruptedCacheEntry) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  // Corrupt |cache_entry|.
  cert_cache.clear_cache_entry();

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |request_params| in the serialized data should fail to be
// deserialized. Should not deserialize a corrupted |request_params|.
TEST(CertVerifierCacheSerializerTest, DeserializeCorruptedRequestParams) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    // Corrupt |request_params|.
    cache_entry->clear_request_params();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |certificate| in |request_params| in the serialized data should
// fail to be deserialized. Should not deserialize a corrupted |certificate|.
TEST(CertVerifierCacheSerializerTest, DeserializeRequestParamsNoCertificate) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationRequestParams* request_params =
        cache_entry->mutable_request_params();
    // Corrupt certificate.
    request_params->clear_certificate();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |hostname| in |request_params| in the serialized data should
// fail to be deserialized. Should not deserialize a corrupted |hostname|.
TEST(CertVerifierCacheSerializerTest, DeserializeRequestParamsNoHostname) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationRequestParams* request_params =
        cache_entry->mutable_request_params();
    // Corrupt hostname.
    request_params->clear_hostname();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// An invalid |hostname| in |request_params| in the serialized data should
// fail to be deserialized. Should not deserialize an invalid |hostname|.
TEST(CertVerifierCacheSerializerTest, DeserializeRequestParamsEmptyHostname) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationRequestParams* request_params =
        cache_entry->mutable_request_params();
    // Set bogus hostname.
    request_params->set_hostname("");
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |flags| in |request_params| in the serialized data should
// fail to be deserialized. Should not deserialize a corrupted |flags|.
TEST(CertVerifierCacheSerializerTest, DeserializeRequestParamsNoFlags) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationRequestParams* request_params =
        cache_entry->mutable_request_params();
    // Corrupt flags.
    request_params->clear_flags();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |ocsp_response| in |request_params| in the serialized data should
// fail to be deserialized. Should not deserialize a corrupted |ocsp_response|.
TEST(CertVerifierCacheSerializerTest, DeserializeRequestParamsNoOcspResponse) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationRequestParams* request_params =
        cache_entry->mutable_request_params();
    // Corrupt |ocsp_response|.
    request_params->clear_ocsp_response();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// An empty certificate number in |request_params| in the serialized data should
// fail to be deserialized. Should not deserialize an empty certificate number.
TEST(CertVerifierCacheSerializerTest,
     DeserializeRequestParamsCertificateNoCertNumbers) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationRequestParams* request_params =
        cache_entry->mutable_request_params();
    cronet_pb::CertVerificationCertificate* certificate =
        request_params->mutable_certificate();
    // Corrupt certificate number.
    certificate->clear_cert_numbers();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// An invalid certificate number in |request_params| in the serialized data
// should fail to be deserialized. Should not deserialize an invalid certificate
// number.
TEST(CertVerifierCacheSerializerTest,
     DeserializeCorruptedRequestParamsCertNumbers) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationRequestParams* request_params =
        cache_entry->mutable_request_params();
    cronet_pb::CertVerificationCertificate* certificate =
        request_params->mutable_certificate();
    // Set bogus certificate number.
    certificate->set_cert_numbers(0, 100);
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted certificate number in |additional_trust_anchors| of
// |request_params| in the serialized data should fail to be deserialized.
// Should not deserialize a corrupted certificate number.
TEST(CertVerifierCacheSerializerTest,
     DeserializeRequestParamsCertificateNoTrustAnchors) {
  net::CertificateList ca_cert_list = net::CreateCertificateListFromFile(
      net::GetTestCertsDirectory(), "root_ca_cert.pem",
      net::X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, ca_cert_list.size());
  scoped_refptr<net::X509Certificate> ca_cert(ca_cert_list[0]);

  net::CertificateList cert_list = net::CreateCertificateListFromFile(
      net::GetTestCertsDirectory(), "ok_cert.pem",
      net::X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, cert_list.size());
  scoped_refptr<net::X509Certificate> cert(cert_list[0]);

  // Now add the |ca_cert| to the |trust_anchors|, and verification should pass.
  net::CertificateList trust_anchors;
  trust_anchors.push_back(ca_cert);

  net::CertVerifyResult verify_result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;

  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  // Verify the |cert| with |trust_anchors|.
  ignore_result(callback.GetResult(verifier.Verify(
      net::CertVerifier::RequestParams(cert, "www.example.com", 0,
                                       std::string(), trust_anchors),
      nullptr, &verify_result, callback.callback(), &request,
      net::NetLogWithSource())));

  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(2, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationRequestParams* request_params =
        cache_entry->mutable_request_params();
    for (int j = 0; j < request_params->additional_trust_anchors_size(); ++j) {
      cronet_pb::CertVerificationCertificate* certificate =
          request_params->mutable_additional_trust_anchors(j);
      // Corrupt the certificate number in |additional_trust_anchors|.
      certificate->clear_cert_numbers();
    }
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |cached_result| in the serialized data should fail to be
// deserialized. Should not deserialize a corrupted |cached_result|.
TEST(CertVerifierCacheSerializerTest, DeserializeCorruptedCachedResult) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    // Corrupt the |cached_result|.
    cache_entry->clear_cached_result();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |error| in the serialized data should fail to be deserialized.
// Should not deserialize a corrupted |error|.
TEST(CertVerifierCacheSerializerTest, DeserializeCachedResultNoError) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationCachedResult* cached_result =
        cache_entry->mutable_cached_result();
    // Corrupt |error|.
    cached_result->clear_error();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |result| in the serialized data should fail to be deserialized.
// Should not deserialize a corrupted |result|.
TEST(CertVerifierCacheSerializerTest, DeserializeCachedResultNoResult) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationCachedResult* cached_result =
        cache_entry->mutable_cached_result();
    // Corrupt the |result|.
    cached_result->clear_result();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |cert_status| in the serialized data should fail to be
// deserialized. Should not deserialize a corrupted |cert_status|.
TEST(CertVerifierCacheSerializerTest, DeserializeCachedResultNoCertStatus) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationCachedResult* cached_result =
        cache_entry->mutable_cached_result();
    cronet_pb::CertVerificationResult* result = cached_result->mutable_result();
    // Corrupt the |cert_status|.
    result->clear_cert_status();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |verification_time| in the serialized data should fail to be
// deserialized. Should not deserialize a corrupted |verification_time|.
TEST(CertVerifierCacheSerializerTest, DeserializeCachedResultNoVerifiedCert) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationCachedResult* cached_result =
        cache_entry->mutable_cached_result();
    cronet_pb::CertVerificationResult* result = cached_result->mutable_result();
    // Corrupt the |verified_cert|.
    result->clear_verified_cert();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |verified_cert| in the serialized data should fail to be
// deserialized. Should not deserialize a corrupted |verified_cert|.
TEST(CertVerifierCacheSerializerTest,
     DeserializeCachedResultNoVerifiedCertNumber) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationCachedResult* cached_result =
        cache_entry->mutable_cached_result();
    cronet_pb::CertVerificationResult* result = cached_result->mutable_result();
    result->clear_verified_cert();
    // Corrupt the verified cert's certificate number.
    cronet_pb::CertVerificationCertificate* certificate =
        result->mutable_verified_cert();
    certificate->clear_cert_numbers();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// An invalid certificate number of |verified_cert| in the serialized data
// should fail to be deserialized. Should not deserialize an invalid certificate
// number.
TEST(CertVerifierCacheSerializerTest,
     DeserializeCorruptedCachedResultVerifiedCertNumber) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationCachedResult* cached_result =
        cache_entry->mutable_cached_result();
    cronet_pb::CertVerificationResult* result = cached_result->mutable_result();
    cronet_pb::CertVerificationCertificate* certificate =
        result->mutable_verified_cert();
    // Set bogus certificate number for |verified_cert|.
    certificate->set_cert_numbers(0, 100);
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |public_key_hashes| in the serialized data should fail to be
// deserialized. Should not deserialize a corrupted |public_key_hashes|.
TEST(CertVerifierCacheSerializerTest,
     DeserializeCorruptedCachedResultPublicKeyHashes) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    cronet_pb::CertVerificationCachedResult* cached_result =
        cache_entry->mutable_cached_result();
    cronet_pb::CertVerificationResult* result = cached_result->mutable_result();
    // Set bogus |public_key_hashes|.
    result->add_public_key_hashes("");
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

// A corrupted |verification_time| in the serialized data should fail to be
// deserialized. Should not deserialize a corrupted |verification_time|.
TEST(CertVerifierCacheSerializerTest, DeserializeCorruptedVerificationTime) {
  net::CertVerifyResult verify_result;
  net::CachingCertVerifier verifier(std::make_unique<net::MockCertVerifier>());
  ASSERT_NO_FATAL_FAILURE(
      VerifyCert("ok_cert.pem", "www.example.com", &verifier, &verify_result));
  cronet_pb::CertVerificationCache cert_cache =
      SerializeCertVerifierCache(verifier);
  DCHECK_EQ(1, cert_cache.cert_entry_size());
  DCHECK_EQ(1, cert_cache.cache_entry_size());

  for (int i = 0; i < cert_cache.cache_entry_size(); ++i) {
    cronet_pb::CertVerificationCacheEntry* cache_entry =
        cert_cache.mutable_cache_entry(i);
    // Corrupt |verification_time|.
    cache_entry->clear_verification_time();
  }

  net::CachingCertVerifier verifier2(std::make_unique<net::MockCertVerifier>());
  EXPECT_FALSE(DeserializeCertVerifierCache(cert_cache, &verifier2));
}

}  // namespace cronet
