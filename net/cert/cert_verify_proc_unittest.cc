// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/cert_verify_proc.h"

#include <vector>

#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/sha1.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "crypto/sha2.h"
#include "net/base/net_errors.h"
#include "net/cert/asn1_util.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_proc_builtin.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/crl_set.h"
#include "net/cert/ev_root_ca_metadata.h"
#include "net/cert/internal/signature_algorithm.h"
#include "net/cert/test_root_certs.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/der/input.h"
#include "net/der/parser.h"
#include "net/test/cert_test_util.h"
#include "net/test/gtest_util.h"
#include "net/test/test_certificate_data.h"
#include "net/test/test_data_directory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include "base/mac/mac_util.h"
#endif

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

// TODO(crbug.com/649017): Add tests that only certificates with
// serverAuth are accepted.

using net::test::IsError;
using net::test::IsOk;

using base::HexEncode;

namespace net {

namespace {

const char kTLSFeatureExtensionHistogram[] =
    "Net.Certificate.TLSFeatureExtensionWithPrivateRoot";
const char kTLSFeatureExtensionOCSPHistogram[] =
    "Net.Certificate.TLSFeatureExtensionWithPrivateRootHasOCSP";
const char kTrustAnchorVerifyHistogram[] = "Net.Certificate.TrustAnchor.Verify";
const char kTrustAnchorVerifyOutOfDateHistogram[] =
    "Net.Certificate.TrustAnchor.VerifyOutOfDate";

// Mock CertVerifyProc that sets the CertVerifyResult to a given value for
// all certificates that are Verify()'d
class MockCertVerifyProc : public CertVerifyProc {
 public:
  explicit MockCertVerifyProc(const CertVerifyResult& result)
      : result_(result) {}
  // CertVerifyProc implementation:
  bool SupportsAdditionalTrustAnchors() const override { return false; }
  bool SupportsOCSPStapling() const override { return false; }

 protected:
  ~MockCertVerifyProc() override = default;

 private:
  int VerifyInternal(X509Certificate* cert,
                     const std::string& hostname,
                     const std::string& ocsp_response,
                     int flags,
                     CRLSet* crl_set,
                     const CertificateList& additional_trust_anchors,
                     CertVerifyResult* verify_result) override;

  const CertVerifyResult result_;

  DISALLOW_COPY_AND_ASSIGN(MockCertVerifyProc);
};

int MockCertVerifyProc::VerifyInternal(
    X509Certificate* cert,
    const std::string& hostname,
    const std::string& ocsp_response,
    int flags,
    CRLSet* crl_set,
    const CertificateList& additional_trust_anchors,
    CertVerifyResult* verify_result) {
  *verify_result = result_;
  verify_result->verified_cert = cert;
  return OK;
}

// This enum identifies a concrete implemenation of CertVerifyProc.
//
// The type is erased by CertVerifyProc::CreateDefault(), however
// needs to be known for some of the test expectations.
enum CertVerifyProcType {
  CERT_VERIFY_PROC_NSS,
  CERT_VERIFY_PROC_ANDROID,
  CERT_VERIFY_PROC_IOS,
  CERT_VERIFY_PROC_MAC,
  CERT_VERIFY_PROC_WIN,
  CERT_VERIFY_PROC_BUILTIN,
};

// Returns the CertVerifyProcType corresponding to what
// CertVerifyProc::CreateDefault() returns. This needs to be kept in sync with
// CreateDefault().
CertVerifyProcType GetDefaultCertVerifyProcType() {
#if defined(USE_NSS_CERTS)
  return CERT_VERIFY_PROC_NSS;
#elif defined(OS_ANDROID)
  return CERT_VERIFY_PROC_ANDROID;
#elif defined(OS_IOS)
  return CERT_VERIFY_PROC_IOS;
#elif defined(OS_MACOSX)
  return CERT_VERIFY_PROC_MAC;
#elif defined(OS_WIN)
  return CERT_VERIFY_PROC_WIN;
#elif defined(OS_FUCHSIA)
  return CERT_VERIFY_PROC_BUILTIN;
#else
// Will fail to compile.
#endif
}

// Whether the test is running within the iphone simulator.
const bool kTargetIsIphoneSimulator =
#if TARGET_IPHONE_SIMULATOR
    true;
#else
    false;
#endif

// Returns a textual description of the CertVerifyProc implementation
// that is being tested, used to give better names to parameterized
// tests.
std::string VerifyProcTypeToName(
    const testing::TestParamInfo<CertVerifyProcType>& params) {
  switch (params.param) {
    case CERT_VERIFY_PROC_NSS:
      return "CertVerifyProcNSS";
    case CERT_VERIFY_PROC_ANDROID:
      return "CertVerifyProcAndroid";
    case CERT_VERIFY_PROC_IOS:
      return "CertVerifyProcIOS";
    case CERT_VERIFY_PROC_MAC:
      return "CertVerifyProcMac";
    case CERT_VERIFY_PROC_WIN:
      return "CertVerifyProcWin";
    case CERT_VERIFY_PROC_BUILTIN:
      return "CertVerifyProcBuiltin";
  }

  return nullptr;
}

// The set of all CertVerifyProcTypes that tests should be
// parameterized on.
const std::vector<CertVerifyProcType> kAllCertVerifiers = {
    GetDefaultCertVerifyProcType()

// TODO(crbug.com/649017): Enable this everywhere. Right now this is
// gated on having CertVerifyProcBuiltin understand the roots added
// via TestRootCerts.
#if defined(USE_NSS_CERTS) || (defined(OS_MACOSX) && !defined(OS_IOS))
        ,
    CERT_VERIFY_PROC_BUILTIN
#endif
};

// Returns true if a test root added through ScopedTestRoot can verify
// successfully as a target certificate with chain of length 1 on the given
// CertVerifyProcType.
bool ScopedTestRootCanTrustTargetCert(CertVerifyProcType verify_proc_type) {
  return verify_proc_type == CERT_VERIFY_PROC_MAC ||
         verify_proc_type == CERT_VERIFY_PROC_IOS ||
         verify_proc_type == CERT_VERIFY_PROC_NSS ||
         verify_proc_type == CERT_VERIFY_PROC_ANDROID;
}

// TODO(crbug.com/649017): This is not parameterized by the CertVerifyProc
// because the CertVerifyProc::Verify() does this unconditionally based on the
// platform.
bool AreSHA1IntermediatesAllowed() {
#if defined(OS_WIN)
  // TODO(rsleevi): Remove this once https://crbug.com/588789 is resolved
  // for Windows 7/2008 users.
  // Note: This must be kept in sync with cert_verify_proc.cc
  return base::win::GetVersion() < base::win::VERSION_WIN8;
#else
  return false;
#endif
}

}  // namespace

// This fixture is for tests that apply to concrete implementations of
// CertVerifyProc. It will be run for all of the concrete CertVerifyProc types.
//
// It is called "Internal" as it tests the internal methods like
// "VerifyInternal()".
class CertVerifyProcInternalTest
    : public testing::TestWithParam<CertVerifyProcType> {
 protected:
  void SetUp() override {
    CertVerifyProcType type = verify_proc_type();
    if (type == CERT_VERIFY_PROC_BUILTIN) {
      verify_proc_ = CreateCertVerifyProcBuiltin();
    } else if (type == GetDefaultCertVerifyProcType()) {
      verify_proc_ = CertVerifyProc::CreateDefault();
    } else {
      ADD_FAILURE() << "Unhandled CertVerifyProcType";
    }
  }

  int Verify(X509Certificate* cert,
             const std::string& hostname,
             int flags,
             CRLSet* crl_set,
             const CertificateList& additional_trust_anchors,
             CertVerifyResult* verify_result) {
    return verify_proc_->Verify(cert, hostname, std::string(), flags, crl_set,
                                additional_trust_anchors, verify_result);
  }

  CertVerifyProcType verify_proc_type() const { return GetParam(); }

  bool SupportsAdditionalTrustAnchors() const {
    return verify_proc_->SupportsAdditionalTrustAnchors();
  }

  bool SupportsReturningVerifiedChain() const {
#if defined(OS_ANDROID)
    // Before API level 17 (SDK_VERSION_JELLY_BEAN_MR1), Android does
    // not expose the APIs necessary to get at the verified
    // certificate chain.
    if (verify_proc_type() == CERT_VERIFY_PROC_ANDROID &&
        base::android::BuildInfo::GetInstance()->sdk_int() <
            base::android::SDK_VERSION_JELLY_BEAN_MR1)
      return false;
#endif
    return true;
  }

  bool WeakKeysAreInvalid() const {
#if defined(OS_MACOSX) && !defined(OS_IOS)
    // Starting with Mac OS 10.12, certs with weak keys are treated as
    // (recoverable) invalid certificate errors.
    if (verify_proc_type() == CERT_VERIFY_PROC_MAC &&
        base::mac::IsAtLeastOS10_12()) {
      return true;
    }
#endif
    return false;
  }

  bool SupportsCRLSet() const {
    return verify_proc_type() == CERT_VERIFY_PROC_NSS ||
           verify_proc_type() == CERT_VERIFY_PROC_WIN ||
           verify_proc_type() == CERT_VERIFY_PROC_MAC ||
           verify_proc_type() == CERT_VERIFY_PROC_BUILTIN;
  }

  bool SupportsCRLSetsInPathBuilding() const {
    return verify_proc_type() == CERT_VERIFY_PROC_WIN ||
           verify_proc_type() == CERT_VERIFY_PROC_NSS ||
           verify_proc_type() == CERT_VERIFY_PROC_BUILTIN;
  }

  bool SupportsEV() const {
    // TODO(crbug.com/117478): Android and iOS do not support EV.
    return verify_proc_type() == CERT_VERIFY_PROC_NSS ||
           verify_proc_type() == CERT_VERIFY_PROC_WIN ||
           verify_proc_type() == CERT_VERIFY_PROC_MAC ||
           verify_proc_type() == CERT_VERIFY_PROC_BUILTIN;
  }

  CertVerifyProc* verify_proc() const { return verify_proc_.get(); }

 private:
  scoped_refptr<CertVerifyProc> verify_proc_;
};

INSTANTIATE_TEST_CASE_P(,
                        CertVerifyProcInternalTest,
                        testing::ValuesIn(kAllCertVerifiers),
                        VerifyProcTypeToName);

// Tests that a certificate is recognized as EV, when the valid EV policy OID
// for the trust anchor is the second candidate EV oid in the target
// certificate. This is a regression test for crbug.com/705285.
TEST_P(CertVerifyProcInternalTest, EVVerificationMultipleOID) {
  if (!SupportsEV()) {
    LOG(INFO) << "Skipping test as EV verification is not yet supported";
    return;
  }

  // TODO(eroman): Update this test to use a synthetic certificate, so the test
  // does not break in the future. The certificate chain in question expires on
  // Dec 22 23:59:59 2018 GMT 2018, at which point this test will start failing.
  if (base::Time::Now() >
      base::Time::UnixEpoch() + base::TimeDelta::FromSeconds(1545523199)) {
    FAIL() << "This test uses a certificate chain which is now expired. Please "
              "disable and file a bug.";
    return;
  }

  scoped_refptr<X509Certificate> chain = CreateCertificateChainFromFile(
      GetTestCertsDirectory(), "trustcenter.websecurity.symantec.com.pem",
      X509Certificate::FORMAT_PEM_CERT_SEQUENCE);
  ASSERT_TRUE(chain);

  // Build a CRLSet that covers the target certificate.
  //
  // This way CRLSet coverage will be sufficient for EV revocation checking,
  // so this test does not depend on online revocation checking.
  ASSERT_EQ(1u, chain->intermediate_buffers().size());
  base::StringPiece spki;
  ASSERT_TRUE(
      asn1::ExtractSPKIFromDERCert(x509_util::CryptoBufferAsStringPiece(
                                       chain->intermediate_buffers()[0].get()),
                                   &spki));
  SHA256HashValue spki_sha256;
  crypto::SHA256HashString(spki, spki_sha256.data, sizeof(spki_sha256.data));
  scoped_refptr<CRLSet> crl_set(
      CRLSet::ForTesting(false, &spki_sha256, "", "", {}));

  CertVerifyResult verify_result;
  int flags = 0;
  int error = Verify(chain.get(), "trustcenter.websecurity.symantec.com", flags,
                     crl_set.get(), CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_IS_EV);
}

// Target cert has an EV policy, and verifies successfully, but has a chain of
// length 1 because the target cert was directly trusted in the trust store.
// Should verify OK but not with STATUS_IS_EV.
TEST_P(CertVerifyProcInternalTest, TrustedTargetCertWithEVPolicy) {
  // The policy that "explicit-policy-chain.pem" target certificate asserts.
  static const char kEVTestCertPolicy[] = "1.2.3.4";
  ScopedTestEVPolicy scoped_test_ev_policy(
      EVRootCAMetadata::GetInstance(), SHA256HashValue(), kEVTestCertPolicy);

  scoped_refptr<X509Certificate> cert =
      ImportCertFromFile(GetTestCertsDirectory(), "explicit-policy-chain.pem");
  ASSERT_TRUE(cert);
  ScopedTestRoot scoped_test_root(cert.get());

  CertVerifyResult verify_result;
  int flags = 0;
  int error = Verify(cert.get(), "policy_test.example", flags,
                     nullptr /*crl_set*/, CertificateList(), &verify_result);
  if (ScopedTestRootCanTrustTargetCert(verify_proc_type())) {
    EXPECT_THAT(error, IsOk());
    ASSERT_TRUE(verify_result.verified_cert);
    EXPECT_TRUE(verify_result.verified_cert->intermediate_buffers().empty());
  } else {
    EXPECT_THAT(error, IsError(ERR_CERT_AUTHORITY_INVALID));
  }
  EXPECT_FALSE(verify_result.cert_status & CERT_STATUS_IS_EV);
}

// Target cert has an EV policy, and verifies successfully with a chain of
// length 1, and its fingerprint matches the cert fingerprint for that ev
// policy. This should never happen in reality, but just test that things don't
// explode if it does.
TEST_P(CertVerifyProcInternalTest,
       TrustedTargetCertWithEVPolicyAndEVFingerprint) {
  // The policy that "explicit-policy-chain.pem" target certificate asserts.
  static const char kEVTestCertPolicy[] = "1.2.3.4";
  // This the fingerprint of the "explicit-policy-chain.pem" target certificate.
  // See net/data/ssl/certificates/explicit-policy-chain.pem
  static const SHA256HashValue kEVTestCertFingerprint = {
      {0x71, 0xac, 0xfa, 0x12, 0xa4, 0x42, 0x31, 0x3c, 0xff, 0x10, 0xd2,
       0x9d, 0xb6, 0x1b, 0x4a, 0xe8, 0x25, 0x4e, 0x77, 0xd3, 0x9f, 0xa3,
       0x2f, 0xb3, 0x19, 0x8d, 0x46, 0x9f, 0xb7, 0x73, 0x07, 0x30}};
  ScopedTestEVPolicy scoped_test_ev_policy(EVRootCAMetadata::GetInstance(),
                                           kEVTestCertFingerprint,
                                           kEVTestCertPolicy);

  scoped_refptr<X509Certificate> cert =
      ImportCertFromFile(GetTestCertsDirectory(), "explicit-policy-chain.pem");
  ASSERT_TRUE(cert);
  ScopedTestRoot scoped_test_root(cert.get());

  CertVerifyResult verify_result;
  int flags = 0;
  int error = Verify(cert.get(), "policy_test.example", flags,
                     nullptr /*crl_set*/, CertificateList(), &verify_result);
  if (ScopedTestRootCanTrustTargetCert(verify_proc_type())) {
    EXPECT_THAT(error, IsOk());
    ASSERT_TRUE(verify_result.verified_cert);
    EXPECT_TRUE(verify_result.verified_cert->intermediate_buffers().empty());
  } else {
    EXPECT_THAT(error, IsError(ERR_CERT_AUTHORITY_INVALID));
  }
  // An EV Root certificate should never be used as an end-entity certificate.
  EXPECT_FALSE(verify_result.cert_status & CERT_STATUS_IS_EV);
}

// TODO(crbug.com/605457): the test expectation was incorrect on some
// configurations, so disable the test until it is fixed (better to have
// a bug to track a failing test than a false sense of security due to
// false positive).
TEST_P(CertVerifyProcInternalTest, DISABLED_PaypalNullCertParsing) {
  // A certificate for www.paypal.com with a NULL byte in the common name.
  // From http://www.gossamer-threads.com/lists/fulldisc/full-disclosure/70363
  SHA256HashValue paypal_null_fingerprint = {{0x00}};

  scoped_refptr<X509Certificate> paypal_null_cert(
      X509Certificate::CreateFromBytes(
          reinterpret_cast<const char*>(paypal_null_der),
          sizeof(paypal_null_der)));

  ASSERT_NE(static_cast<X509Certificate*>(NULL), paypal_null_cert.get());

  EXPECT_EQ(paypal_null_fingerprint, X509Certificate::CalculateFingerprint256(
                                         paypal_null_cert->cert_buffer()));

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(paypal_null_cert.get(), "www.paypal.com", flags, NULL,
                     CertificateList(), &verify_result);

  if (verify_proc_type() == CERT_VERIFY_PROC_NSS ||
      verify_proc_type() == CERT_VERIFY_PROC_ANDROID) {
    EXPECT_THAT(error, IsError(ERR_CERT_COMMON_NAME_INVALID));
  } else if (verify_proc_type() == CERT_VERIFY_PROC_IOS &&
             kTargetIsIphoneSimulator) {
    // iOS returns a ERR_CERT_INVALID error on the simulator, while returning
    // ERR_CERT_AUTHORITY_INVALID on the real device.
    EXPECT_THAT(error, IsError(ERR_CERT_INVALID));
  } else {
    // TODO(bulach): investigate why macosx and win aren't returning
    // ERR_CERT_INVALID or ERR_CERT_COMMON_NAME_INVALID.
    EXPECT_THAT(error, IsError(ERR_CERT_AUTHORITY_INVALID));
  }

  // Either the system crypto library should correctly report a certificate
  // name mismatch, or our certificate blacklist should cause us to report an
  // invalid certificate.
  if (verify_proc_type() == CERT_VERIFY_PROC_NSS ||
      verify_proc_type() == CERT_VERIFY_PROC_WIN) {
    EXPECT_TRUE(verify_result.cert_status &
                (CERT_STATUS_COMMON_NAME_INVALID | CERT_STATUS_INVALID));
  }

  // TODO(crbug.com/649017): What expectations to use for the other verifiers?
}

// Tests the case where the target certificate is accepted by
// X509CertificateBytes, but has errors that should cause verification to fail.
TEST_P(CertVerifyProcInternalTest, InvalidTarget) {
  base::FilePath certs_dir =
      GetTestNetDataDirectory().AppendASCII("parse_certificate_unittest");
  scoped_refptr<X509Certificate> bad_cert =
      ImportCertFromFile(certs_dir, "signature_algorithm_null.pem");
  ASSERT_TRUE(bad_cert);

  scoped_refptr<X509Certificate> ok_cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(ok_cert);

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(x509_util::DupCryptoBuffer(ok_cert->cert_buffer()));
  scoped_refptr<X509Certificate> cert_with_bad_target(
      X509Certificate::CreateFromBuffer(
          x509_util::DupCryptoBuffer(bad_cert->cert_buffer()),
          std::move(intermediates)));
  ASSERT_TRUE(cert_with_bad_target);
  EXPECT_EQ(1U, cert_with_bad_target->intermediate_buffers().size());

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert_with_bad_target.get(), "127.0.0.1", flags, NULL,
                     CertificateList(), &verify_result);

  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_INVALID);
  EXPECT_THAT(error, IsError(ERR_CERT_INVALID));
}

// Tests the case where an intermediate certificate is accepted by
// X509CertificateBytes, but has errors that should prevent using it during
// verification.  The verification should succeed, since the intermediate
// wasn't necessary.
TEST_P(CertVerifyProcInternalTest, UnnecessaryInvalidIntermediate) {
  ScopedTestRoot test_root(
      ImportCertFromFile(GetTestCertsDirectory(), "root_ca_cert.pem").get());

  base::FilePath certs_dir =
      GetTestNetDataDirectory().AppendASCII("parse_certificate_unittest");
  bssl::UniquePtr<CRYPTO_BUFFER> bad_cert =
      x509_util::CreateCryptoBuffer(base::StringPiece("invalid"));
  ASSERT_TRUE(bad_cert);

  scoped_refptr<X509Certificate> ok_cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(ok_cert);

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(std::move(bad_cert));
  scoped_refptr<X509Certificate> cert_with_bad_intermediate(
      X509Certificate::CreateFromBuffer(
          x509_util::DupCryptoBuffer(ok_cert->cert_buffer()),
          std::move(intermediates)));
  ASSERT_TRUE(cert_with_bad_intermediate);
  EXPECT_EQ(1U, cert_with_bad_intermediate->intermediate_buffers().size());

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert_with_bad_intermediate.get(), "127.0.0.1", flags, NULL,
                     CertificateList(), &verify_result);

  EXPECT_THAT(error, IsOk());
  EXPECT_EQ(0u, verify_result.cert_status);
}

// A regression test for http://crbug.com/31497.
TEST_P(CertVerifyProcInternalTest, IntermediateCARequireExplicitPolicy) {
  if (verify_proc_type() == CERT_VERIFY_PROC_ANDROID) {
    // Disabled on Android, as the Android verification libraries require an
    // explicit policy to be specified, even when anyPolicy is permitted.
    LOG(INFO) << "Skipping test on Android";
    return;
  }

  base::FilePath certs_dir = GetTestCertsDirectory();

  CertificateList certs = CreateCertificateListFromFile(
      certs_dir, "explicit-policy-chain.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(3U, certs.size());

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[1]->cert_buffer()));

  scoped_refptr<X509Certificate> cert = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(certs[0]->cert_buffer()),
      std::move(intermediates));
  ASSERT_TRUE(cert.get());

  ScopedTestRoot scoped_root(certs[2].get());

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert.get(), "policy_test.example", flags, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_EQ(0u, verify_result.cert_status);
}

TEST_P(CertVerifyProcInternalTest, RejectExpiredCert) {
  base::FilePath certs_dir = GetTestCertsDirectory();

  // Load root_ca_cert.pem into the test root store.
  ScopedTestRoot test_root(
      ImportCertFromFile(certs_dir, "root_ca_cert.pem").get());

  scoped_refptr<X509Certificate> cert = CreateCertificateChainFromFile(
      certs_dir, "expired_cert.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_TRUE(cert);
  ASSERT_EQ(0U, cert->intermediate_buffers().size());

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert.get(), "127.0.0.1", flags, NULL, CertificateList(),
                     &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_DATE_INVALID));
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_DATE_INVALID);
}

// Currently, only RSA and DSA keys are checked for weakness, and our example
// weak size is 768. These could change in the future.
//
// Note that this means there may be false negatives: keys for other
// algorithms and which are weak will pass this test.
static bool IsWeakKeyType(const std::string& key_type) {
  size_t pos = key_type.find("-");
  std::string size = key_type.substr(0, pos);
  std::string type = key_type.substr(pos + 1);

  if (type == "rsa" || type == "dsa")
    return size == "768";

  return false;
}

TEST_P(CertVerifyProcInternalTest, RejectWeakKeys) {
  base::FilePath certs_dir = GetTestCertsDirectory();
  typedef std::vector<std::string> Strings;
  Strings key_types;

  // generate-weak-test-chains.sh currently has:
  //     key_types="768-rsa 1024-rsa 2048-rsa prime256v1-ecdsa"
  // We must use the same key types here. The filenames generated look like:
  //     2048-rsa-ee-by-768-rsa-intermediate.pem
  key_types.push_back("768-rsa");
  key_types.push_back("1024-rsa");
  key_types.push_back("2048-rsa");
  key_types.push_back("prime256v1-ecdsa");

  // Add the root that signed the intermediates for this test.
  scoped_refptr<X509Certificate> root_cert =
      ImportCertFromFile(certs_dir, "2048-rsa-root.pem");
  ASSERT_NE(static_cast<X509Certificate*>(NULL), root_cert.get());
  ScopedTestRoot scoped_root(root_cert.get());

  // Now test each chain.
  for (Strings::const_iterator ee_type = key_types.begin();
       ee_type != key_types.end(); ++ee_type) {
    for (Strings::const_iterator signer_type = key_types.begin();
         signer_type != key_types.end(); ++signer_type) {
      std::string basename =
          *ee_type + "-ee-by-" + *signer_type + "-intermediate.pem";
      SCOPED_TRACE(basename);
      scoped_refptr<X509Certificate> ee_cert =
          ImportCertFromFile(certs_dir, basename);
      ASSERT_NE(static_cast<X509Certificate*>(NULL), ee_cert.get());

      basename = *signer_type + "-intermediate.pem";
      scoped_refptr<X509Certificate> intermediate =
          ImportCertFromFile(certs_dir, basename);
      ASSERT_NE(static_cast<X509Certificate*>(NULL), intermediate.get());

      std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
      intermediates.push_back(
          x509_util::DupCryptoBuffer(intermediate->cert_buffer()));
      scoped_refptr<X509Certificate> cert_chain =
          X509Certificate::CreateFromBuffer(
              x509_util::DupCryptoBuffer(ee_cert->cert_buffer()),
              std::move(intermediates));
      ASSERT_TRUE(cert_chain);

      CertVerifyResult verify_result;
      int error = Verify(cert_chain.get(), "127.0.0.1", 0, NULL,
                         CertificateList(), &verify_result);

      if (IsWeakKeyType(*ee_type) || IsWeakKeyType(*signer_type)) {
        EXPECT_NE(OK, error);
        EXPECT_EQ(CERT_STATUS_WEAK_KEY,
                  verify_result.cert_status & CERT_STATUS_WEAK_KEY);
        EXPECT_EQ(WeakKeysAreInvalid() ? CERT_STATUS_INVALID : 0,
                  verify_result.cert_status & CERT_STATUS_INVALID);
      } else {
        EXPECT_THAT(error, IsOk());
        EXPECT_EQ(0U, verify_result.cert_status & CERT_STATUS_WEAK_KEY);
      }
    }
  }
}

// Regression test for http://crbug.com/108514.
TEST_P(CertVerifyProcInternalTest, ExtraneousMD5RootCert) {
  if (!SupportsReturningVerifiedChain()) {
    LOG(INFO) << "Skipping this test in this platform.";
    return;
  }

  if (verify_proc_type() == CERT_VERIFY_PROC_MAC) {
    // Disabled on OS X - Security.framework doesn't ignore superflous
    // certificates provided by servers.
    // TODO(eroman): Is this still needed?
    LOG(INFO) << "Skipping this test as Security.framework doesn't ignore "
                 "superflous certificates provided by servers.";
    return;
  }

  base::FilePath certs_dir = GetTestCertsDirectory();

  scoped_refptr<X509Certificate> server_cert =
      ImportCertFromFile(certs_dir, "cross-signed-leaf.pem");
  ASSERT_NE(static_cast<X509Certificate*>(NULL), server_cert.get());

  scoped_refptr<X509Certificate> extra_cert =
      ImportCertFromFile(certs_dir, "cross-signed-root-md5.pem");
  ASSERT_NE(static_cast<X509Certificate*>(NULL), extra_cert.get());

  scoped_refptr<X509Certificate> root_cert =
      ImportCertFromFile(certs_dir, "cross-signed-root-sha256.pem");
  ASSERT_NE(static_cast<X509Certificate*>(NULL), root_cert.get());

  ScopedTestRoot scoped_root(root_cert.get());

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(
      x509_util::DupCryptoBuffer(extra_cert->cert_buffer()));
  scoped_refptr<X509Certificate> cert_chain = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(server_cert->cert_buffer()),
      std::move(intermediates));
  ASSERT_TRUE(cert_chain);

  CertVerifyResult verify_result;
  int flags = 0;
  int error = Verify(cert_chain.get(), "127.0.0.1", flags, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());

  // The extra MD5 root should be discarded
  ASSERT_TRUE(verify_result.verified_cert.get());
  ASSERT_EQ(1u, verify_result.verified_cert->intermediate_buffers().size());
  EXPECT_TRUE(x509_util::CryptoBufferEqual(
      verify_result.verified_cert->intermediate_buffers().front().get(),
      root_cert->cert_buffer()));

  EXPECT_FALSE(verify_result.has_md5);
}

// Test for bug 94673.
TEST_P(CertVerifyProcInternalTest, GoogleDigiNotarTest) {
  base::FilePath certs_dir = GetTestCertsDirectory();

  scoped_refptr<X509Certificate> server_cert =
      ImportCertFromFile(certs_dir, "google_diginotar.pem");
  ASSERT_NE(static_cast<X509Certificate*>(NULL), server_cert.get());

  scoped_refptr<X509Certificate> intermediate_cert =
      ImportCertFromFile(certs_dir, "diginotar_public_ca_2025.pem");
  ASSERT_NE(static_cast<X509Certificate*>(NULL), intermediate_cert.get());

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(
      x509_util ::DupCryptoBuffer(intermediate_cert->cert_buffer()));
  scoped_refptr<X509Certificate> cert_chain = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(server_cert->cert_buffer()),
      std::move(intermediates));
  ASSERT_TRUE(cert_chain);

  CertVerifyResult verify_result;
  int flags = CertVerifier::VERIFY_REV_CHECKING_ENABLED;
  int error = Verify(cert_chain.get(), "mail.google.com", flags, NULL,
                     CertificateList(), &verify_result);
  EXPECT_NE(OK, error);

  // Now turn off revocation checking.  Certificate verification should still
  // fail.
  flags = 0;
  error = Verify(cert_chain.get(), "mail.google.com", flags, NULL,
                 CertificateList(), &verify_result);
  EXPECT_NE(OK, error);
}

// Ensures the CertVerifyProc blacklist remains in sorted order, so that it
// can be binary-searched.
TEST(CertVerifyProcTest, BlacklistIsSorted) {
// Defines kBlacklistedSPKIs.
#include "net/cert/cert_verify_proc_blacklist.inc"
  for (size_t i = 0; i < base::size(kBlacklistedSPKIs) - 1; ++i) {
    EXPECT_GT(0, memcmp(kBlacklistedSPKIs[i], kBlacklistedSPKIs[i + 1],
                        crypto::kSHA256Length))
        << " at index " << i;
  }
}

TEST(CertVerifyProcTest, DigiNotarCerts) {
  static const char* const kDigiNotarFilenames[] = {
      "diginotar_root_ca.pem",          "diginotar_cyber_ca.pem",
      "diginotar_services_1024_ca.pem", "diginotar_pkioverheid.pem",
      "diginotar_pkioverheid_g2.pem",   NULL,
  };

  base::FilePath certs_dir = GetTestCertsDirectory();

  for (size_t i = 0; kDigiNotarFilenames[i]; i++) {
    scoped_refptr<X509Certificate> diginotar_cert =
        ImportCertFromFile(certs_dir, kDigiNotarFilenames[i]);
    base::StringPiece spki;
    ASSERT_TRUE(asn1::ExtractSPKIFromDERCert(
        x509_util::CryptoBufferAsStringPiece(diginotar_cert->cert_buffer()),
        &spki));

    std::string spki_sha256 = crypto::SHA256HashString(spki);

    HashValueVector public_keys;
    HashValue hash(HASH_VALUE_SHA256);
    ASSERT_EQ(hash.size(), spki_sha256.size());
    memcpy(hash.data(), spki_sha256.data(), spki_sha256.size());
    public_keys.push_back(hash);

    EXPECT_TRUE(CertVerifyProc::IsPublicKeyBlacklisted(public_keys))
        << "Public key not blocked for " << kDigiNotarFilenames[i];
  }
}

TEST_P(CertVerifyProcInternalTest, NameConstraintsOk) {
  CertificateList ca_cert_list =
      CreateCertificateListFromFile(GetTestCertsDirectory(), "root_ca_cert.pem",
                                    X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, ca_cert_list.size());
  ScopedTestRoot test_root(ca_cert_list[0].get());

  scoped_refptr<X509Certificate> leaf = CreateCertificateChainFromFile(
      GetTestCertsDirectory(), "name_constraint_good.pem",
      X509Certificate::FORMAT_AUTO);
  ASSERT_TRUE(leaf);
  ASSERT_EQ(0U, leaf->intermediate_buffers().size());

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(leaf.get(), "test.example.com", flags, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_EQ(0U, verify_result.cert_status);

  error = Verify(leaf.get(), "foo.test2.example.com", flags, NULL,
                 CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_EQ(0U, verify_result.cert_status);
}

// This fixture is for testing the verification of a certificate chain which
// has some sort of mismatched signature algorithm (i.e.
// Certificate.signatureAlgorithm and TBSCertificate.algorithm are different).
class CertVerifyProcInspectSignatureAlgorithmsTest : public ::testing::Test {
 protected:
  // In the test setup, SHA384 is given special treatment as an unknown
  // algorithm.
  static constexpr DigestAlgorithm kUnknownDigestAlgorithm =
      DigestAlgorithm::Sha384;

  struct CertParams {
    // Certificate.signatureAlgorithm
    DigestAlgorithm cert_algorithm;

    // TBSCertificate.algorithm
    DigestAlgorithm tbs_algorithm;
  };

  // On some platforms trying to import a certificate with mismatched signature
  // will fail. Consequently the rest of the tests can't be performed.
  WARN_UNUSED_RESULT bool SupportsImportingMismatchedAlgorithms() const {
#if defined(OS_IOS)
    LOG(INFO) << "Skipping test on iOS because certs with mismatched "
                 "algorithms cannot be imported";
    return false;
#elif defined(OS_MACOSX)
    if (base::mac::IsAtLeastOS10_12()) {
      LOG(INFO) << "Skipping test on macOS >= 10.12 because certs with "
                   "mismatched algorithms cannot be imported";
      return false;
    }
    return true;
#else
    return true;
#endif
  }

  // Shorthand for VerifyChain() where only the leaf's parameters need
  // to be specified.
  WARN_UNUSED_RESULT int VerifyLeaf(const CertParams& leaf_params) {
    return VerifyChain({// Target
                        leaf_params,
                        // Root
                        {DigestAlgorithm::Sha256, DigestAlgorithm::Sha256}});
  }

  // Shorthand for VerifyChain() where only the intermediate's parameters need
  // to be specified.
  WARN_UNUSED_RESULT int VerifyIntermediate(
      const CertParams& intermediate_params) {
    return VerifyChain({// Target
                        {DigestAlgorithm::Sha256, DigestAlgorithm::Sha256},
                        // Intermediate
                        intermediate_params,
                        // Root
                        {DigestAlgorithm::Sha256, DigestAlgorithm::Sha256}});
  }

  // Shorthand for VerifyChain() where only the root's parameters need to be
  // specified.
  WARN_UNUSED_RESULT int VerifyRoot(const CertParams& root_params) {
    return VerifyChain({// Target
                        {DigestAlgorithm::Sha256, DigestAlgorithm::Sha256},
                        // Intermediate
                        {DigestAlgorithm::Sha256, DigestAlgorithm::Sha256},
                        // Root
                        root_params});
  }

  // Manufactures a certificate chain where each certificate has the indicated
  // signature algorithms, and then returns the result of verifying this chain.
  //
  // TODO(eroman): Instead of building certificates at runtime, move their
  //               generation to external scripts.
  WARN_UNUSED_RESULT int VerifyChain(
      const std::vector<CertParams>& chain_params) {
    auto chain = CreateChain(chain_params);
    if (!chain) {
      ADD_FAILURE() << "Failed creating certificate chain";
      return ERR_UNEXPECTED;
    }

    int flags = 0;
    CertVerifyResult dummy_result;
    CertVerifyResult verify_result;

    scoped_refptr<CertVerifyProc> verify_proc =
        new MockCertVerifyProc(dummy_result);

    return verify_proc->Verify(chain.get(), "test.example.com", std::string(),
                               flags, NULL, CertificateList(), &verify_result);
  }

 private:
  // Overwrites the AlgorithmIdentifier pointed to by |algorithm_sequence| with
  // |algorithm|. Note this violates the constness of StringPiece.
  WARN_UNUSED_RESULT static bool SetAlgorithmSequence(
      DigestAlgorithm algorithm,
      base::StringPiece* algorithm_sequence) {
    // This string of bytes is the full SEQUENCE for an AlgorithmIdentifier.
    std::vector<uint8_t> replacement_sequence;
    switch (algorithm) {
      case DigestAlgorithm::Sha1:
        // sha1WithRSAEncryption
        replacement_sequence = {0x30, 0x0D, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
                                0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00};
        break;
      case DigestAlgorithm::Sha256:
        // sha256WithRSAEncryption
        replacement_sequence = {0x30, 0x0D, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
                                0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00};
        break;
      case kUnknownDigestAlgorithm:
        // This shouldn't be anything meaningful (modified numbers at random).
        replacement_sequence = {0x30, 0x0D, 0x06, 0x09, 0x8a, 0x87, 0x18, 0x46,
                                0xd7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00};
        break;
      default:
        ADD_FAILURE() << "Unsupported digest algorithm";
        return false;
    }

    // For this simple replacement to work (without modifying any
    // other sequence lengths) the original algorithm and replacement
    // algorithm must have the same encoded length.
    if (algorithm_sequence->size() != replacement_sequence.size()) {
      ADD_FAILURE() << "AlgorithmIdentifier must have length "
                    << replacement_sequence.size();
      return false;
    }

    memcpy(const_cast<char*>(algorithm_sequence->data()),
           replacement_sequence.data(), replacement_sequence.size());
    return true;
  }

  // Locate the serial number bytes.
  WARN_UNUSED_RESULT static bool ExtractSerialNumberFromDERCert(
      base::StringPiece der_cert,
      base::StringPiece* serial_value) {
    der::Parser parser((der::Input(der_cert)));
    der::Parser certificate;
    if (!parser.ReadSequence(&certificate))
      return false;

    der::Parser tbs_certificate;
    if (!certificate.ReadSequence(&tbs_certificate))
      return false;

    bool unused;
    if (!tbs_certificate.SkipOptionalTag(
            der::kTagConstructed | der::kTagContextSpecific | 0, &unused)) {
      return false;
    }

    // serialNumber
    der::Input serial_value_der;
    if (!tbs_certificate.ReadTag(der::kInteger, &serial_value_der))
      return false;

    *serial_value = serial_value_der.AsStringPiece();
    return true;
  }

  // Creates a certificate (based on some base certificate file) using the
  // specified signature algorithms.
  static scoped_refptr<X509Certificate> CreateCertificate(
      const CertParams& params) {
    // Dosn't really matter which base certificate is used, so long as it is
    // valid and uses a signature AlgorithmIdentifier with the same encoded
    // length as sha1WithRSASignature.
    const char* kLeafFilename = "name_constraint_good.pem";

    auto cert = CreateCertificateChainFromFile(
        GetTestCertsDirectory(), kLeafFilename, X509Certificate::FORMAT_AUTO);
    if (!cert) {
      ADD_FAILURE() << "Failed to load certificate: " << kLeafFilename;
      return nullptr;
    }

    // Start with the DER bytes of a valid certificate. The der data is copied
    // to a new std::string as it will modified to create a new certificate.
    std::string cert_der(
        x509_util::CryptoBufferAsStringPiece(cert->cert_buffer()));

    // Parse the certificate and identify the locations of interest within
    // |cert_der|.
    base::StringPiece cert_algorithm_sequence;
    base::StringPiece tbs_algorithm_sequence;
    if (!asn1::ExtractSignatureAlgorithmsFromDERCert(
            cert_der, &cert_algorithm_sequence, &tbs_algorithm_sequence)) {
      ADD_FAILURE() << "Failed parsing certificate algorithms";
      return nullptr;
    }

    base::StringPiece serial_value;
    if (!ExtractSerialNumberFromDERCert(cert_der, &serial_value)) {
      ADD_FAILURE() << "Failed parsing certificate serial number";
      return nullptr;
    }

    // Give each certificate a unique serial number based on its content (which
    // in turn is a function of |params|, otherwise importing it may fail.

    // Upper bound for last entry in DigestAlgorithm
    const int kNumDigestAlgorithms = 15;
    *const_cast<char*>(serial_value.data()) +=
        static_cast<int>(params.tbs_algorithm) * kNumDigestAlgorithms +
        static_cast<int>(params.cert_algorithm);

    // Change the signature AlgorithmIdentifiers.
    if (!SetAlgorithmSequence(params.cert_algorithm,
                              &cert_algorithm_sequence) ||
        !SetAlgorithmSequence(params.tbs_algorithm, &tbs_algorithm_sequence)) {
      return nullptr;
    }

    // NOTE: The signature is NOT recomputed over TBSCertificate -- for these
    // tests it isn't needed.
    return X509Certificate::CreateFromBytes(cert_der.data(), cert_der.size());
  }

  static scoped_refptr<X509Certificate> CreateChain(
      const std::vector<CertParams>& params) {
    // Manufacture a chain with the given combinations of signature algorithms.
    // This chain isn't actually a valid chain, but it is good enough for
    // testing the base CertVerifyProc.
    CertificateList certs;
    for (const auto& cert_params : params) {
      certs.push_back(CreateCertificate(cert_params));
      if (!certs.back())
        return nullptr;
    }

    std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
    for (size_t i = 1; i < certs.size(); ++i)
      intermediates.push_back(
          x509_util::DupCryptoBuffer(certs[i]->cert_buffer()));

    return X509Certificate::CreateFromBuffer(
        x509_util::DupCryptoBuffer(certs[0]->cert_buffer()),
        std::move(intermediates));
  }
};

// This is a control test to make sure that the test helper
// VerifyLeaf() works as expected. There is no actual mismatch in the
// algorithms used here.
//
//  Certificate.signatureAlgorithm:  sha1WithRSASignature
//  TBSCertificate.algorithm:        sha1WithRSAEncryption
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, LeafSha1Sha1) {
  int rv = VerifyLeaf({DigestAlgorithm::Sha1, DigestAlgorithm::Sha1});
  ASSERT_THAT(rv, IsError(ERR_CERT_WEAK_SIGNATURE_ALGORITHM));
}

// This is a control test to make sure that the test helper
// VerifyLeaf() works as expected. There is no actual mismatch in the
// algorithms used here.
//
//  Certificate.signatureAlgorithm:  sha256WithRSASignature
//  TBSCertificate.algorithm:        sha256WithRSAEncryption
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, LeafSha256Sha256) {
  int rv = VerifyLeaf({DigestAlgorithm::Sha256, DigestAlgorithm::Sha256});
  ASSERT_THAT(rv, IsOk());
}

// Mismatched signature algorithms in the leaf certificate.
//
//  Certificate.signatureAlgorithm:  sha1WithRSASignature
//  TBSCertificate.algorithm:        sha256WithRSAEncryption
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, LeafSha1Sha256) {
  if (!SupportsImportingMismatchedAlgorithms())
    return;

  int rv = VerifyLeaf({DigestAlgorithm::Sha1, DigestAlgorithm::Sha256});
  ASSERT_THAT(rv, IsError(ERR_CERT_INVALID));
}

// Mismatched signature algorithms in the leaf certificate.
//
//  Certificate.signatureAlgorithm:  sha256WithRSAEncryption
//  TBSCertificate.algorithm:        sha1WithRSASignature
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, LeafSha256Sha1) {
  if (!SupportsImportingMismatchedAlgorithms())
    return;

  int rv = VerifyLeaf({DigestAlgorithm::Sha256, DigestAlgorithm::Sha1});
  ASSERT_THAT(rv, IsError(ERR_CERT_INVALID));
}

// Unrecognized signature algorithm in the leaf certificate.
//
//  Certificate.signatureAlgorithm:  sha256WithRSAEncryption
//  TBSCertificate.algorithm:        ?
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, LeafSha256Unknown) {
  if (!SupportsImportingMismatchedAlgorithms())
    return;

  int rv = VerifyLeaf({DigestAlgorithm::Sha256, kUnknownDigestAlgorithm});
  ASSERT_THAT(rv, IsError(ERR_CERT_INVALID));
}

// Unrecognized signature algorithm in the leaf certificate.
//
//  Certificate.signatureAlgorithm:  ?
//  TBSCertificate.algorithm:        sha256WithRSAEncryption
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, LeafUnknownSha256) {
  if (!SupportsImportingMismatchedAlgorithms())
    return;

  int rv = VerifyLeaf({kUnknownDigestAlgorithm, DigestAlgorithm::Sha256});
  ASSERT_THAT(rv, IsError(ERR_CERT_INVALID));
}

// Mismatched signature algorithms in the intermediate certificate.
//
//  Certificate.signatureAlgorithm:  sha1WithRSASignature
//  TBSCertificate.algorithm:        sha256WithRSAEncryption
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, IntermediateSha1Sha256) {
  if (!SupportsImportingMismatchedAlgorithms())
    return;

  int rv = VerifyIntermediate({DigestAlgorithm::Sha1, DigestAlgorithm::Sha256});
  ASSERT_THAT(rv, IsError(ERR_CERT_INVALID));
}

// Mismatched signature algorithms in the intermediate certificate.
//
//  Certificate.signatureAlgorithm:  sha256WithRSAEncryption
//  TBSCertificate.algorithm:        sha1WithRSASignature
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, IntermediateSha256Sha1) {
  if (!SupportsImportingMismatchedAlgorithms())
    return;

  int rv = VerifyIntermediate({DigestAlgorithm::Sha256, DigestAlgorithm::Sha1});
  ASSERT_THAT(rv, IsError(ERR_CERT_INVALID));
}

// Mismatched signature algorithms in the root certificate.
//
//  Certificate.signatureAlgorithm:  sha256WithRSAEncryption
//  TBSCertificate.algorithm:        sha1WithRSASignature
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, RootSha256Sha1) {
  if (!SupportsImportingMismatchedAlgorithms())
    return;

  int rv = VerifyRoot({DigestAlgorithm::Sha256, DigestAlgorithm::Sha1});
  ASSERT_THAT(rv, IsOk());
}

// Unrecognized signature algorithm in the root certificate.
//
//  Certificate.signatureAlgorithm:  ?
//  TBSCertificate.algorithm:        sha256WithRSAEncryption
TEST_F(CertVerifyProcInspectSignatureAlgorithmsTest, RootUnknownSha256) {
  if (!SupportsImportingMismatchedAlgorithms())
    return;

  int rv = VerifyRoot({kUnknownDigestAlgorithm, DigestAlgorithm::Sha256});
  ASSERT_THAT(rv, IsOk());
}

TEST_P(CertVerifyProcInternalTest, NameConstraintsFailure) {
  if (!SupportsReturningVerifiedChain()) {
    LOG(INFO) << "Skipping this test in this platform.";
    return;
  }

  CertificateList ca_cert_list =
      CreateCertificateListFromFile(GetTestCertsDirectory(), "root_ca_cert.pem",
                                    X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, ca_cert_list.size());
  ScopedTestRoot test_root(ca_cert_list[0].get());

  CertificateList cert_list = CreateCertificateListFromFile(
      GetTestCertsDirectory(), "name_constraint_bad.pem",
      X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, cert_list.size());

  scoped_refptr<X509Certificate> leaf = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(cert_list[0]->cert_buffer()), {});
  ASSERT_TRUE(leaf);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(leaf.get(), "test.example.com", flags, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_NAME_CONSTRAINT_VIOLATION));
  EXPECT_EQ(CERT_STATUS_NAME_CONSTRAINT_VIOLATION,
            verify_result.cert_status & CERT_STATUS_NAME_CONSTRAINT_VIOLATION);
}

TEST(CertVerifyProcTest, TestHasTooLongValidity) {
  struct {
    const char* const file;
    bool is_valid_too_long;
  } tests[] = {
      {"daltonridgeapts.com-chain.pem", false},
      {"start_after_expiry.pem", true},
      {"pre_br_validity_ok.pem", false},
      {"pre_br_validity_bad_121.pem", true},
      {"pre_br_validity_bad_2020.pem", true},
      {"10_year_validity.pem", false},
      {"11_year_validity.pem", true},
      {"39_months_after_2015_04.pem", false},
      {"40_months_after_2015_04.pem", true},
      {"60_months_after_2012_07.pem", false},
      {"61_months_after_2012_07.pem", true},
      {"825_days_after_2018_03_01.pem", false},
      {"826_days_after_2018_03_01.pem", true},
      {"825_days_1_second_after_2018_03_01.pem", true},
      {"39_months_based_on_last_day.pem", false},
  };

  base::FilePath certs_dir = GetTestCertsDirectory();

  for (const auto& test : tests) {
    SCOPED_TRACE(test.file);

    scoped_refptr<X509Certificate> certificate =
        ImportCertFromFile(certs_dir, test.file);
    ASSERT_TRUE(certificate);
    EXPECT_EQ(test.is_valid_too_long,
              CertVerifyProc::HasTooLongValidity(*certificate));
  }
}

TEST_P(CertVerifyProcInternalTest, TestKnownRoot) {
  base::FilePath certs_dir = GetTestCertsDirectory();
  scoped_refptr<X509Certificate> cert_chain = CreateCertificateChainFromFile(
      certs_dir, "daltonridgeapts.com-chain.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_TRUE(cert_chain);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert_chain.get(), "daltonridgeapts.com", flags, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk()) << "This test relies on a real certificate that "
                             << "expires on May 28, 2021. If failing on/after "
                             << "that date, please disable and file a bug "
                             << "against rsleevi.";
  EXPECT_TRUE(verify_result.is_issued_by_known_root);
}

// This tests that on successful certificate verification,
// CertVerifyResult::public_key_hashes is filled with a SHA1 and SHA256 hash
// for each of the certificates in the chain.
TEST_P(CertVerifyProcInternalTest, PublicKeyHashes) {
  if (!SupportsReturningVerifiedChain()) {
    LOG(INFO) << "Skipping this test in this platform.";
    return;
  }

  base::FilePath certs_dir = GetTestCertsDirectory();
  CertificateList certs = CreateCertificateListFromFile(
      certs_dir, "x509_verify_results.chain.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(3U, certs.size());

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[1]->cert_buffer()));
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[2]->cert_buffer()));

  ScopedTestRoot scoped_root(certs[2].get());
  scoped_refptr<X509Certificate> cert_chain = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(certs[0]->cert_buffer()),
      std::move(intermediates));
  ASSERT_TRUE(cert_chain);
  ASSERT_EQ(2U, cert_chain->intermediate_buffers().size());

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert_chain.get(), "127.0.0.1", flags, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());

  EXPECT_EQ(3u, verify_result.public_key_hashes.size());

  // Convert |public_key_hashes| to strings for ease of comparison.
  std::vector<std::string> public_key_hash_strings;
  for (const auto& public_key_hash : verify_result.public_key_hashes)
    public_key_hash_strings.push_back(public_key_hash.ToString());

  std::vector<std::string> expected_public_key_hashes = {
      // Target
      "sha256/jpsUnwFFTO7e+l5zQDYhutkf7uA+dCVsWfRvv0UDX40=",

      // Intermediate
      "sha256/D9u0epgvPYlG9YiVp7V+IMT+xhUpB5BhsS/INjDXc4Y=",

      // Trust anchor
      "sha256/VypP3VWL7OaqTJ7mIBehWYlv8khPuFHpWiearZI2YjI="};

  // |public_key_hashes| does not have an ordering guarantee.
  EXPECT_THAT(expected_public_key_hashes,
              testing::UnorderedElementsAreArray(public_key_hash_strings));
}

// A regression test for http://crbug.com/70293.
// The certificate in question has a key purpose of clientAuth, and also lacks
// the required key usage for serverAuth.
TEST_P(CertVerifyProcInternalTest, WrongKeyPurpose) {
  base::FilePath certs_dir = GetTestCertsDirectory();

  scoped_refptr<X509Certificate> server_cert =
      ImportCertFromFile(certs_dir, "invalid_key_usage_cert.der");
  ASSERT_NE(static_cast<X509Certificate*>(NULL), server_cert.get());

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(server_cert.get(), "jira.aquameta.com", flags, NULL,
                     CertificateList(), &verify_result);

  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_COMMON_NAME_INVALID);

  // TODO(crbug.com/649017): Don't special-case builtin verifier.
  if (verify_proc_type() != CERT_VERIFY_PROC_BUILTIN)
    EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_INVALID);

  // TODO(wtc): fix http://crbug.com/75520 to get all the certificate errors
  // from NSS.
  if (verify_proc_type() != CERT_VERIFY_PROC_NSS &&
      verify_proc_type() != CERT_VERIFY_PROC_ANDROID) {
    // The certificate is issued by an unknown CA.
    EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_AUTHORITY_INVALID);
  }

  // TODO(crbug.com/649017): Don't special-case builtin verifier.
  if (verify_proc_type() == CERT_VERIFY_PROC_BUILTIN) {
    EXPECT_THAT(error, IsError(ERR_CERT_AUTHORITY_INVALID));
  } else {
    EXPECT_THAT(error, IsError(ERR_CERT_INVALID));
  }
}

// Tests that a Netscape Server Gated crypto is accepted in place of a
// serverAuth EKU.
// TODO(crbug.com/843735): Deprecate support for this.
TEST_P(CertVerifyProcInternalTest, Sha1IntermediateUsesServerGatedCrypto) {
  base::FilePath certs_dir =
      GetTestNetDataDirectory()
          .AppendASCII("verify_certificate_chain_unittest")
          .AppendASCII("intermediate-eku-server-gated-crypto");

  scoped_refptr<X509Certificate> cert_chain = CreateCertificateChainFromFile(
      certs_dir, "sha1-chain.pem", X509Certificate::FORMAT_AUTO);

  ASSERT_TRUE(cert_chain);
  ASSERT_FALSE(cert_chain->intermediate_buffers().empty());

  auto root = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(
          cert_chain->intermediate_buffers().back().get()),
      {});

  ScopedTestRoot scoped_root(root.get());

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert_chain.get(), "test.example", flags, NULL,
                     CertificateList(), &verify_result);

  if (AreSHA1IntermediatesAllowed()) {
    EXPECT_THAT(error, IsOk());
    EXPECT_EQ(CERT_STATUS_SHA1_SIGNATURE_PRESENT, verify_result.cert_status);
  } else {
    EXPECT_THAT(error, IsError(ERR_CERT_WEAK_SIGNATURE_ALGORITHM));
    EXPECT_EQ(CERT_STATUS_WEAK_SIGNATURE_ALGORITHM |
                  CERT_STATUS_SHA1_SIGNATURE_PRESENT,
              verify_result.cert_status);
  }
}

// Basic test for returning the chain in CertVerifyResult. Note that the
// returned chain may just be a reflection of the originally supplied chain;
// that is, if any errors occur, the default chain returned is an exact copy
// of the certificate to be verified. The remaining VerifyReturn* tests are
// used to ensure that the actual, verified chain is being returned by
// Verify().
TEST_P(CertVerifyProcInternalTest, VerifyReturnChainBasic) {
  if (!SupportsReturningVerifiedChain()) {
    LOG(INFO) << "Skipping this test in this platform.";
    return;
  }

  base::FilePath certs_dir = GetTestCertsDirectory();
  CertificateList certs = CreateCertificateListFromFile(
      certs_dir, "x509_verify_results.chain.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(3U, certs.size());

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[1]->cert_buffer()));
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[2]->cert_buffer()));

  ScopedTestRoot scoped_root(certs[2].get());

  scoped_refptr<X509Certificate> google_full_chain =
      X509Certificate::CreateFromBuffer(
          x509_util::DupCryptoBuffer(certs[0]->cert_buffer()),
          std::move(intermediates));
  ASSERT_NE(static_cast<X509Certificate*>(NULL), google_full_chain.get());
  ASSERT_EQ(2U, google_full_chain->intermediate_buffers().size());

  CertVerifyResult verify_result;
  EXPECT_EQ(static_cast<X509Certificate*>(NULL),
            verify_result.verified_cert.get());
  int error = Verify(google_full_chain.get(), "127.0.0.1", 0, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  ASSERT_NE(static_cast<X509Certificate*>(NULL),
            verify_result.verified_cert.get());

  EXPECT_NE(google_full_chain, verify_result.verified_cert);
  EXPECT_TRUE(
      x509_util::CryptoBufferEqual(google_full_chain->cert_buffer(),
                                   verify_result.verified_cert->cert_buffer()));
  const auto& return_intermediates =
      verify_result.verified_cert->intermediate_buffers();
  ASSERT_EQ(2U, return_intermediates.size());
  EXPECT_TRUE(x509_util::CryptoBufferEqual(return_intermediates[0].get(),
                                           certs[1]->cert_buffer()));
  EXPECT_TRUE(x509_util::CryptoBufferEqual(return_intermediates[1].get(),
                                           certs[2]->cert_buffer()));
}

// Test that certificates issued for 'intranet' names (that is, containing no
// known public registry controlled domain information) issued by well-known
// CAs are flagged appropriately, while certificates that are issued by
// internal CAs are not flagged.
TEST(CertVerifyProcTest, IntranetHostsRejected) {
  CertificateList cert_list = CreateCertificateListFromFile(
      GetTestCertsDirectory(), "reject_intranet_hosts.pem",
      X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, cert_list.size());
  scoped_refptr<X509Certificate> cert(cert_list[0]);

  CertVerifyResult verify_result;
  int error = 0;

  // Intranet names for public CAs should be flagged:
  CertVerifyResult dummy_result;
  dummy_result.is_issued_by_known_root = true;
  scoped_refptr<CertVerifyProc> verify_proc =
      new MockCertVerifyProc(dummy_result);
  error = verify_proc->Verify(cert.get(), "webmail", std::string(), 0, nullptr,
                              CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_NON_UNIQUE_NAME);

  // However, if the CA is not well known, these should not be flagged:
  dummy_result.Reset();
  dummy_result.is_issued_by_known_root = false;
  verify_proc = base::MakeRefCounted<MockCertVerifyProc>(dummy_result);
  error = verify_proc->Verify(cert.get(), "webmail", std::string(), 0, nullptr,
                              CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_FALSE(verify_result.cert_status & CERT_STATUS_NON_UNIQUE_NAME);
}

// Tests that certificates issued by Symantec's legacy infrastructure
// are rejected according to the policies outlined in
// https://security.googleblog.com/2017/09/chromes-plan-to-distrust-symantec.html
// unless the caller has explicitly disabled that enforcement.
TEST(CertVerifyProcTest, SymantecCertsRejected) {
  constexpr SHA256HashValue kSymantecHashValue = {
      {0xb2, 0xde, 0xf5, 0x36, 0x2a, 0xd3, 0xfa, 0xcd, 0x04, 0xbd, 0x29,
       0x04, 0x7a, 0x43, 0x84, 0x4f, 0x76, 0x70, 0x34, 0xea, 0x48, 0x92,
       0xf8, 0x0e, 0x56, 0xbe, 0xe6, 0x90, 0x24, 0x3e, 0x25, 0x02}};
  constexpr SHA256HashValue kGoogleHashValue = {
      {0xec, 0x72, 0x29, 0x69, 0xcb, 0x64, 0x20, 0x0a, 0xb6, 0x63, 0x8f,
       0x68, 0xac, 0x53, 0x8e, 0x40, 0xab, 0xab, 0x5b, 0x19, 0xa6, 0x48,
       0x56, 0x61, 0x04, 0x2a, 0x10, 0x61, 0xc4, 0x61, 0x27, 0x76}};

  // Test that certificates from the legacy Symantec infrastructure are
  // rejected:
  // - dec_2017.pem : A certificate issued after 2017-12-01, which is rejected
  //                  as of M65
  // - pre_june_2016.pem : A certificate issued prior to 2016-06-01, which is
  //                       rejected as of M66.
  for (const char* rejected_cert : {"dec_2017.pem", "pre_june_2016.pem"}) {
    scoped_refptr<X509Certificate> cert = CreateCertificateChainFromFile(
        GetTestCertsDirectory(), rejected_cert, X509Certificate::FORMAT_AUTO);
    ASSERT_TRUE(cert);

    scoped_refptr<CertVerifyProc> verify_proc;
    int error = 0;

    // Test that a legacy Symantec certificate is rejected.
    CertVerifyResult symantec_result;
    symantec_result.verified_cert = cert;
    symantec_result.public_key_hashes.push_back(HashValue(kSymantecHashValue));
    symantec_result.is_issued_by_known_root = true;
    verify_proc = base::MakeRefCounted<MockCertVerifyProc>(symantec_result);

    CertVerifyResult test_result_1;
    error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), 0,
                                nullptr, CertificateList(), &test_result_1);
    EXPECT_THAT(error, IsError(ERR_CERT_SYMANTEC_LEGACY));
    EXPECT_TRUE(test_result_1.cert_status & CERT_STATUS_SYMANTEC_LEGACY);

    // ... Unless the Symantec cert chains through a whitelisted intermediate.
    CertVerifyResult whitelisted_result;
    whitelisted_result.verified_cert = cert;
    whitelisted_result.public_key_hashes.push_back(
        HashValue(kSymantecHashValue));
    whitelisted_result.public_key_hashes.push_back(HashValue(kGoogleHashValue));
    whitelisted_result.is_issued_by_known_root = true;
    verify_proc = base::MakeRefCounted<MockCertVerifyProc>(whitelisted_result);

    CertVerifyResult test_result_2;
    error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), 0,
                                nullptr, CertificateList(), &test_result_2);
    EXPECT_THAT(error, IsOk());
    EXPECT_FALSE(test_result_2.cert_status & CERT_STATUS_AUTHORITY_INVALID);

    // ... Or the caller disabled enforcement of Symantec policies.
    CertVerifyResult test_result_3;
    error =
        verify_proc->Verify(cert.get(), "127.0.0.1", std::string(),
                            CertVerifier::VERIFY_DISABLE_SYMANTEC_ENFORCEMENT,
                            nullptr, CertificateList(), &test_result_3);
    EXPECT_THAT(error, IsOk());
    EXPECT_FALSE(test_result_3.cert_status & CERT_STATUS_SYMANTEC_LEGACY);
  }

  // Test that certificates from the legacy Symantec infrastructure that
  // should still be accepted (for now) are accepted.
  // - post_june_2016.pem : A certificate issued after 2016-06-01, which is
  //                        not scheduled for distrust until M70.
  for (const char* accepted_cert : {"post_june_2016.pem"}) {
    scoped_refptr<X509Certificate> cert = CreateCertificateChainFromFile(
        GetTestCertsDirectory(), accepted_cert, X509Certificate::FORMAT_AUTO);
    ASSERT_TRUE(cert);

    scoped_refptr<CertVerifyProc> verify_proc;
    int error = 0;

    // Test that a Symantec certificate is accepted.
    CertVerifyResult symantec_result;
    symantec_result.verified_cert = cert;
    symantec_result.public_key_hashes.push_back(HashValue(kSymantecHashValue));
    symantec_result.is_issued_by_known_root = true;
    verify_proc = base::MakeRefCounted<MockCertVerifyProc>(symantec_result);

    CertVerifyResult test_result_1;
    error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), 0,
                                nullptr, CertificateList(), &test_result_1);
    EXPECT_THAT(error, IsOk());
    EXPECT_FALSE(test_result_1.cert_status & CERT_STATUS_SYMANTEC_LEGACY);
  }
}

// While all SHA-1 certificates should be rejected, in the event that there
// emerges some unexpected bug, test that the 'legacy' behaviour works
// correctly - rejecting all SHA-1 certificates from publicly trusted CAs
// that were issued after 1 January 2016, while still allowing those from
// before that date, with SHA-1 in the intermediate, or from an enterprise
// CA.
TEST(CertVerifyProcTest, VerifyRejectsSHA1AfterDeprecationLegacyMode) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(CertVerifyProc::kSHA1LegacyMode);

  CertVerifyResult dummy_result;
  CertVerifyResult verify_result;
  int error = 0;
  scoped_refptr<X509Certificate> cert;

  // Publicly trusted SHA-1 leaf certificates issued before 1 January 2016
  // are accepted.
  verify_result.Reset();
  dummy_result.Reset();
  dummy_result.is_issued_by_known_root = true;
  dummy_result.has_sha1 = true;
  dummy_result.has_sha1_leaf = true;
  scoped_refptr<CertVerifyProc> verify_proc =
      new MockCertVerifyProc(dummy_result);
  cert = CreateCertificateChainFromFile(GetTestCertsDirectory(),
                                        "sha1_dec_2015.pem",
                                        X509Certificate::FORMAT_AUTO);
  ASSERT_TRUE(cert);
  error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), 0, NULL,
                              CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_SHA1_SIGNATURE_PRESENT);

  // Publicly trusted SHA-1 leaf certificates issued on/after 1 January 2016
  // are rejected.
  verify_result.Reset();
  dummy_result.Reset();
  dummy_result.is_issued_by_known_root = true;
  dummy_result.has_sha1 = true;
  dummy_result.has_sha1_leaf = true;
  verify_proc = base::MakeRefCounted<MockCertVerifyProc>(dummy_result);
  cert = CreateCertificateChainFromFile(GetTestCertsDirectory(),
                                        "sha1_jan_2016.pem",
                                        X509Certificate::FORMAT_AUTO);
  ASSERT_TRUE(cert);
  error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), 0, NULL,
                              CertificateList(), &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_WEAK_SIGNATURE_ALGORITHM));
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_WEAK_SIGNATURE_ALGORITHM);

  // Enterprise issued SHA-1 leaf certificates issued on/after 1 January 2016
  // remain accepted.
  verify_result.Reset();
  dummy_result.Reset();
  dummy_result.is_issued_by_known_root = false;
  dummy_result.has_sha1 = true;
  dummy_result.has_sha1_leaf = true;
  verify_proc = base::MakeRefCounted<MockCertVerifyProc>(dummy_result);
  cert = CreateCertificateChainFromFile(GetTestCertsDirectory(),
                                        "sha1_jan_2016.pem",
                                        X509Certificate::FORMAT_AUTO);
  ASSERT_TRUE(cert);
  error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), 0, NULL,
                              CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_SHA1_SIGNATURE_PRESENT);

  // Publicly trusted SHA-1 intermediates issued on/after 1 January 2016 are,
  // unfortunately, accepted. This can arise due to OS path building quirks.
  verify_result.Reset();
  dummy_result.Reset();
  dummy_result.is_issued_by_known_root = true;
  dummy_result.has_sha1 = true;
  dummy_result.has_sha1_leaf = false;
  verify_proc = base::MakeRefCounted<MockCertVerifyProc>(dummy_result);
  cert = CreateCertificateChainFromFile(GetTestCertsDirectory(),
                                        "sha1_jan_2016.pem",
                                        X509Certificate::FORMAT_AUTO);
  ASSERT_TRUE(cert);
  error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), 0, NULL,
                              CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_SHA1_SIGNATURE_PRESENT);
}

// Test that the certificate returned in CertVerifyResult is able to reorder
// certificates that are not ordered from end-entity to root. While this is
// a protocol violation if sent during a TLS handshake, if multiple sources
// of intermediate certificates are combined, it's possible that order may
// not be maintained.
TEST_P(CertVerifyProcInternalTest, VerifyReturnChainProperlyOrdered) {
  if (!SupportsReturningVerifiedChain()) {
    LOG(INFO) << "Skipping this test in this platform.";
    return;
  }

  base::FilePath certs_dir = GetTestCertsDirectory();
  CertificateList certs = CreateCertificateListFromFile(
      certs_dir, "x509_verify_results.chain.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(3U, certs.size());

  // Construct the chain out of order.
  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[2]->cert_buffer()));
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[1]->cert_buffer()));

  ScopedTestRoot scoped_root(certs[2].get());

  scoped_refptr<X509Certificate> google_full_chain =
      X509Certificate::CreateFromBuffer(
          x509_util::DupCryptoBuffer(certs[0]->cert_buffer()),
          std::move(intermediates));
  ASSERT_TRUE(google_full_chain);
  ASSERT_EQ(2U, google_full_chain->intermediate_buffers().size());

  CertVerifyResult verify_result;
  EXPECT_FALSE(verify_result.verified_cert);
  int error = Verify(google_full_chain.get(), "127.0.0.1", 0, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  ASSERT_TRUE(verify_result.verified_cert);

  EXPECT_NE(google_full_chain, verify_result.verified_cert);
  EXPECT_TRUE(
      x509_util::CryptoBufferEqual(google_full_chain->cert_buffer(),
                                   verify_result.verified_cert->cert_buffer()));
  const auto& return_intermediates =
      verify_result.verified_cert->intermediate_buffers();
  ASSERT_EQ(2U, return_intermediates.size());
  EXPECT_TRUE(x509_util::CryptoBufferEqual(return_intermediates[0].get(),
                                           certs[1]->cert_buffer()));
  EXPECT_TRUE(x509_util::CryptoBufferEqual(return_intermediates[1].get(),
                                           certs[2]->cert_buffer()));
}

// Test that Verify() filters out certificates which are not related to
// or part of the certificate chain being verified.
TEST_P(CertVerifyProcInternalTest, VerifyReturnChainFiltersUnrelatedCerts) {
  if (!SupportsReturningVerifiedChain()) {
    LOG(INFO) << "Skipping this test in this platform.";
    return;
  }

  base::FilePath certs_dir = GetTestCertsDirectory();
  CertificateList certs = CreateCertificateListFromFile(
      certs_dir, "x509_verify_results.chain.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(3U, certs.size());
  ScopedTestRoot scoped_root(certs[2].get());

  scoped_refptr<X509Certificate> unrelated_certificate =
      ImportCertFromFile(certs_dir, "duplicate_cn_1.pem");
  scoped_refptr<X509Certificate> unrelated_certificate2 =
      ImportCertFromFile(certs_dir, "aia-cert.pem");
  ASSERT_NE(static_cast<X509Certificate*>(NULL), unrelated_certificate.get());
  ASSERT_NE(static_cast<X509Certificate*>(NULL), unrelated_certificate2.get());

  // Interject unrelated certificates into the list of intermediates.
  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(
      x509_util::DupCryptoBuffer(unrelated_certificate->cert_buffer()));
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[1]->cert_buffer()));
  intermediates.push_back(
      x509_util::DupCryptoBuffer(unrelated_certificate2->cert_buffer()));
  intermediates.push_back(x509_util::DupCryptoBuffer(certs[2]->cert_buffer()));

  scoped_refptr<X509Certificate> google_full_chain =
      X509Certificate::CreateFromBuffer(
          x509_util::DupCryptoBuffer(certs[0]->cert_buffer()),
          std::move(intermediates));
  ASSERT_TRUE(google_full_chain);
  ASSERT_EQ(4U, google_full_chain->intermediate_buffers().size());

  CertVerifyResult verify_result;
  EXPECT_FALSE(verify_result.verified_cert);
  int error = Verify(google_full_chain.get(), "127.0.0.1", 0, NULL,
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
  ASSERT_TRUE(verify_result.verified_cert);

  EXPECT_NE(google_full_chain, verify_result.verified_cert);
  EXPECT_TRUE(
      x509_util::CryptoBufferEqual(google_full_chain->cert_buffer(),
                                   verify_result.verified_cert->cert_buffer()));
  const auto& return_intermediates =
      verify_result.verified_cert->intermediate_buffers();
  ASSERT_EQ(2U, return_intermediates.size());
  EXPECT_TRUE(x509_util::CryptoBufferEqual(return_intermediates[0].get(),
                                           certs[1]->cert_buffer()));
  EXPECT_TRUE(x509_util::CryptoBufferEqual(return_intermediates[1].get(),
                                           certs[2]->cert_buffer()));
}

TEST_P(CertVerifyProcInternalTest, AdditionalTrustAnchors) {
  if (!SupportsAdditionalTrustAnchors()) {
    LOG(INFO) << "Skipping this test in this platform.";
    return;
  }

  // |ca_cert| is the issuer of |cert|.
  CertificateList ca_cert_list =
      CreateCertificateListFromFile(GetTestCertsDirectory(), "root_ca_cert.pem",
                                    X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, ca_cert_list.size());
  scoped_refptr<X509Certificate> ca_cert(ca_cert_list[0]);

  CertificateList cert_list = CreateCertificateListFromFile(
      GetTestCertsDirectory(), "ok_cert.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, cert_list.size());
  scoped_refptr<X509Certificate> cert(cert_list[0]);

  // Verification of |cert| fails when |ca_cert| is not in the trust anchors
  // list.
  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert.get(), "127.0.0.1", flags, NULL, CertificateList(),
                     &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_AUTHORITY_INVALID));
  EXPECT_EQ(CERT_STATUS_AUTHORITY_INVALID, verify_result.cert_status);
  EXPECT_FALSE(verify_result.is_issued_by_additional_trust_anchor);

  // Now add the |ca_cert| to the |trust_anchors|, and verification should pass.
  CertificateList trust_anchors;
  trust_anchors.push_back(ca_cert);
  error = Verify(cert.get(), "127.0.0.1", flags, NULL, trust_anchors,
                 &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_EQ(0U, verify_result.cert_status);
  EXPECT_TRUE(verify_result.is_issued_by_additional_trust_anchor);

  // Clearing the |trust_anchors| makes verification fail again (the cache
  // should be skipped).
  error = Verify(cert.get(), "127.0.0.1", flags, NULL, CertificateList(),
                 &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_AUTHORITY_INVALID));
  EXPECT_EQ(CERT_STATUS_AUTHORITY_INVALID, verify_result.cert_status);
  EXPECT_FALSE(verify_result.is_issued_by_additional_trust_anchor);
}

// Tests that certificates issued by user-supplied roots are not flagged as
// issued by a known root. This should pass whether or not the platform supports
// detecting known roots.
TEST_P(CertVerifyProcInternalTest, IsIssuedByKnownRootIgnoresTestRoots) {
  // Load root_ca_cert.pem into the test root store.
  ScopedTestRoot test_root(
      ImportCertFromFile(GetTestCertsDirectory(), "root_ca_cert.pem").get());

  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));

  // Verification should pass.
  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert.get(), "127.0.0.1", flags, NULL, CertificateList(),
                     &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_EQ(0U, verify_result.cert_status);
  // But should not be marked as a known root.
  EXPECT_FALSE(verify_result.is_issued_by_known_root);
}

// Test that CRLSets are effective in making a certificate appear to be
// revoked.
TEST_P(CertVerifyProcInternalTest, CRLSet) {
  if (!SupportsCRLSet()) {
    LOG(INFO) << "Skipping test as verifier doesn't support CRLSet";
    return;
  }

  CertificateList ca_cert_list =
      CreateCertificateListFromFile(GetTestCertsDirectory(), "root_ca_cert.pem",
                                    X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, ca_cert_list.size());
  ScopedTestRoot test_root(ca_cert_list[0].get());

  CertificateList cert_list = CreateCertificateListFromFile(
      GetTestCertsDirectory(), "ok_cert.pem", X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, cert_list.size());
  scoped_refptr<X509Certificate> cert(cert_list[0]);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(cert.get(), "127.0.0.1", flags, NULL, CertificateList(),
                     &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_EQ(0U, verify_result.cert_status);

  scoped_refptr<CRLSet> crl_set;
  std::string crl_set_bytes;

  // First test blocking by SPKI.
  EXPECT_TRUE(base::ReadFileToString(
      GetTestCertsDirectory().AppendASCII("crlset_by_leaf_spki.raw"),
      &crl_set_bytes));
  ASSERT_TRUE(CRLSet::Parse(crl_set_bytes, &crl_set));

  error = Verify(cert.get(), "127.0.0.1", flags, crl_set.get(),
                 CertificateList(), &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_REVOKED));

  // Second, test revocation by serial number of a cert directly under the
  // root.
  crl_set_bytes.clear();
  EXPECT_TRUE(base::ReadFileToString(
      GetTestCertsDirectory().AppendASCII("crlset_by_root_serial.raw"),
      &crl_set_bytes));
  ASSERT_TRUE(CRLSet::Parse(crl_set_bytes, &crl_set));

  error = Verify(cert.get(), "127.0.0.1", flags, crl_set.get(),
                 CertificateList(), &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_REVOKED));
}

TEST_P(CertVerifyProcInternalTest, CRLSetLeafSerial) {
  if (!SupportsCRLSet()) {
    LOG(INFO) << "Skipping test as verifier doesn't support CRLSet";
    return;
  }

  CertificateList ca_cert_list =
      CreateCertificateListFromFile(GetTestCertsDirectory(), "root_ca_cert.pem",
                                    X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, ca_cert_list.size());
  ScopedTestRoot test_root(ca_cert_list[0].get());

  CertificateList intermediate_cert_list = CreateCertificateListFromFile(
      GetTestCertsDirectory(), "intermediate_ca_cert.pem",
      X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, intermediate_cert_list.size());
  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(
      x509_util::DupCryptoBuffer(intermediate_cert_list[0]->cert_buffer()));

  CertificateList cert_list = CreateCertificateListFromFile(
      GetTestCertsDirectory(), "ok_cert_by_intermediate.pem",
      X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1U, cert_list.size());

  scoped_refptr<X509Certificate> leaf = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(cert_list[0]->cert_buffer()),
      std::move(intermediates));
  ASSERT_TRUE(leaf);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = Verify(leaf.get(), "127.0.0.1", flags, NULL, CertificateList(),
                     &verify_result);
  EXPECT_THAT(error, IsOk());

  // Test revocation by serial number of a certificate not under the root.
  scoped_refptr<CRLSet> crl_set;
  std::string crl_set_bytes;
  ASSERT_TRUE(base::ReadFileToString(
      GetTestCertsDirectory().AppendASCII("crlset_by_intermediate_serial.raw"),
      &crl_set_bytes));
  ASSERT_TRUE(CRLSet::Parse(crl_set_bytes, &crl_set));

  error = Verify(leaf.get(), "127.0.0.1", flags, crl_set.get(),
                 CertificateList(), &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_REVOKED));
}

// Tests that CertVerifyProc implementations apply CRLSet revocations by
// subject.
TEST_P(CertVerifyProcInternalTest, CRLSetRevokedBySubject) {
  if (!SupportsCRLSet()) {
    LOG(INFO) << "Skipping test as verifier doesn't support CRLSet";
    return;
  }

  scoped_refptr<X509Certificate> root(
      ImportCertFromFile(GetTestCertsDirectory(), "root_ca_cert.pem"));
  ASSERT_TRUE(root);

  scoped_refptr<X509Certificate> leaf(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(leaf);

  ScopedTestRoot scoped_root(root.get());

  int flags = 0;
  CertVerifyResult verify_result;

  // Confirm that verifying the certificate chain with an empty CRLSet succeeds.
  scoped_refptr<CRLSet> crl_set = CRLSet::EmptyCRLSetForTesting();
  int error = Verify(leaf.get(), "127.0.0.1", flags, crl_set.get(),
                     CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());

  std::string crl_set_bytes;

  // Revoke the leaf by subject. Verification should now fail.
  ASSERT_TRUE(base::ReadFileToString(
      GetTestCertsDirectory().AppendASCII("crlset_by_leaf_subject_no_spki.raw"),
      &crl_set_bytes));
  ASSERT_TRUE(CRLSet::Parse(crl_set_bytes, &crl_set));

  error = Verify(leaf.get(), "127.0.0.1", flags, crl_set.get(),
                 CertificateList(), &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_REVOKED));

  // Revoke the root by subject. Verification should now fail.
  ASSERT_TRUE(base::ReadFileToString(
      GetTestCertsDirectory().AppendASCII("crlset_by_root_subject_no_spki.raw"),
      &crl_set_bytes));
  ASSERT_TRUE(CRLSet::Parse(crl_set_bytes, &crl_set));

  error = Verify(leaf.get(), "127.0.0.1", flags, crl_set.get(),
                 CertificateList(), &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_REVOKED));

  // Revoke the leaf by subject, but only if the SPKI doesn't match the given
  // one. Verification should pass when using the certificate's actual SPKI.
  ASSERT_TRUE(base::ReadFileToString(
      GetTestCertsDirectory().AppendASCII("crlset_by_root_subject.raw"),
      &crl_set_bytes));
  ASSERT_TRUE(CRLSet::Parse(crl_set_bytes, &crl_set));

  error = Verify(leaf.get(), "127.0.0.1", flags, crl_set.get(),
                 CertificateList(), &verify_result);
  EXPECT_THAT(error, IsOk());
}

// Tests that CRLSets participate in path building functions, and that as
// long as a valid path exists within the verification graph, verification
// succeeds.
//
// In this test, there are two roots (D and E), and three possible paths
// to validate a leaf (A):
// 1. A(B) -> B(C) -> C(D) -> D(D)
// 2. A(B) -> B(C) -> C(E) -> E(E)
// 3. A(B) -> B(F) -> F(E) -> E(E)
//
// Each permutation of revocation is tried:
// 1. Revoking E by SPKI, so that only Path 1 is valid (as E is in Paths 2 & 3)
// 2. Revoking C(D) and F(E) by serial, so that only Path 2 is valid.
// 3. Revoking C by SPKI, so that only Path 3 is valid (as C is in Paths 1 & 2)
TEST_P(CertVerifyProcInternalTest, CRLSetDuringPathBuilding) {
  if (!SupportsCRLSetsInPathBuilding()) {
    LOG(INFO) << "Skipping this test on this platform.";
    return;
  }

  CertificateList path_1_certs;
  ASSERT_TRUE(
      LoadCertificateFiles({"multi-root-A-by-B.pem", "multi-root-B-by-C.pem",
                            "multi-root-C-by-D.pem", "multi-root-D-by-D.pem"},
                           &path_1_certs));

  CertificateList path_2_certs;
  ASSERT_TRUE(
      LoadCertificateFiles({"multi-root-A-by-B.pem", "multi-root-B-by-C.pem",
                            "multi-root-C-by-E.pem", "multi-root-E-by-E.pem"},
                           &path_2_certs));

  CertificateList path_3_certs;
  ASSERT_TRUE(
      LoadCertificateFiles({"multi-root-A-by-B.pem", "multi-root-B-by-F.pem",
                            "multi-root-F-by-E.pem", "multi-root-E-by-E.pem"},
                           &path_3_certs));

  // Add D and E as trust anchors.
  ScopedTestRoot test_root_D(path_1_certs[3].get());  // D-by-D
  ScopedTestRoot test_root_E(path_2_certs[3].get());  // E-by-E

  // Create a chain that contains all the certificate paths possible.
  // CertVerifyProcInternalTest.VerifyReturnChainFiltersUnrelatedCerts already
  // ensures that it's safe to send additional certificates as inputs, and
  // that they're ignored if not necessary.
  // This is to avoid relying on AIA or internal object caches when
  // interacting with the underlying library.
  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  intermediates.push_back(
      x509_util::DupCryptoBuffer(path_1_certs[1]->cert_buffer()));  // B-by-C
  intermediates.push_back(
      x509_util::DupCryptoBuffer(path_1_certs[2]->cert_buffer()));  // C-by-D
  intermediates.push_back(
      x509_util::DupCryptoBuffer(path_2_certs[2]->cert_buffer()));  // C-by-E
  intermediates.push_back(
      x509_util::DupCryptoBuffer(path_3_certs[1]->cert_buffer()));  // B-by-F
  intermediates.push_back(
      x509_util::DupCryptoBuffer(path_3_certs[2]->cert_buffer()));  // F-by-E
  scoped_refptr<X509Certificate> cert = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(path_1_certs[0]->cert_buffer()),
      std::move(intermediates));
  ASSERT_TRUE(cert);

  struct TestPermutations {
    const char* crlset;
    bool expect_valid;
    scoped_refptr<X509Certificate> expected_intermediate;
  } kTests[] = {
      {"multi-root-crlset-D-and-E.raw", false, nullptr},
      {"multi-root-crlset-E.raw", true, path_1_certs[2].get()},
      {"multi-root-crlset-CD-and-FE.raw", true, path_2_certs[2].get()},
      {"multi-root-crlset-C.raw", true, path_3_certs[2].get()},
      {"multi-root-crlset-unrelated.raw", true, nullptr}};

  for (const auto& testcase : kTests) {
    SCOPED_TRACE(testcase.crlset);
    scoped_refptr<CRLSet> crl_set;
    std::string crl_set_bytes;
    EXPECT_TRUE(base::ReadFileToString(
        GetTestCertsDirectory().AppendASCII(testcase.crlset), &crl_set_bytes));
    ASSERT_TRUE(CRLSet::Parse(crl_set_bytes, &crl_set));

    int flags = 0;
    CertVerifyResult verify_result;
    int error = Verify(cert.get(), "127.0.0.1", flags, crl_set.get(),
                       CertificateList(), &verify_result);

    if (!testcase.expect_valid) {
      EXPECT_NE(OK, error);
      EXPECT_NE(0U, verify_result.cert_status);
      continue;
    }

    ASSERT_THAT(error, IsOk());
    ASSERT_EQ(0U, verify_result.cert_status);
    ASSERT_TRUE(verify_result.verified_cert.get());

    if (!testcase.expected_intermediate)
      continue;

    const auto& verified_intermediates =
        verify_result.verified_cert->intermediate_buffers();
    ASSERT_EQ(3U, verified_intermediates.size());

    scoped_refptr<X509Certificate> intermediate =
        X509Certificate::CreateFromBuffer(
            x509_util::DupCryptoBuffer(verified_intermediates[1].get()), {});
    ASSERT_TRUE(intermediate);

    EXPECT_TRUE(testcase.expected_intermediate->EqualsExcludingChain(
        intermediate.get()))
        << "Expected: " << testcase.expected_intermediate->subject().common_name
        << " issued by " << testcase.expected_intermediate->issuer().common_name
        << "; Got: " << intermediate->subject().common_name << " issued by "
        << intermediate->issuer().common_name;
  }
}

TEST(CertVerifyProcTest, RejectsMD2) {
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;
  result.has_md2 = true;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  nullptr /* crl_set */, CertificateList(),
                                  &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_INVALID));
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_INVALID);
}

TEST(CertVerifyProcTest, RejectsMD4) {
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;
  result.has_md4 = true;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  nullptr /* crl_set */, CertificateList(),
                                  &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_INVALID));
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_INVALID);
}

TEST(CertVerifyProcTest, RejectsMD5) {
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;
  result.has_md5 = true;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  nullptr /* crl_set */, CertificateList(),
                                  &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_WEAK_SIGNATURE_ALGORITHM));
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_WEAK_SIGNATURE_ALGORITHM);
}

TEST(CertVerifyProcTest, RejectsPublicSHA1Leaves) {
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;
  result.has_sha1 = true;
  result.has_sha1_leaf = true;
  result.is_issued_by_known_root = true;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  nullptr /* crl_set */, CertificateList(),
                                  &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_WEAK_SIGNATURE_ALGORITHM));
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_WEAK_SIGNATURE_ALGORITHM);
}

TEST(CertVerifyProcTest, RejectsPublicSHA1IntermediatesUnlessAllowed) {
  scoped_refptr<X509Certificate> cert(ImportCertFromFile(
      GetTestCertsDirectory(), "39_months_after_2015_04.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;
  result.has_sha1 = true;
  result.has_sha1_leaf = false;
  result.is_issued_by_known_root = true;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  nullptr /* crl_set */, CertificateList(),
                                  &verify_result);
  if (AreSHA1IntermediatesAllowed()) {
    EXPECT_THAT(error, IsOk());
    EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_SHA1_SIGNATURE_PRESENT);
  } else {
    EXPECT_THAT(error, IsError(ERR_CERT_WEAK_SIGNATURE_ALGORITHM));
    EXPECT_TRUE(verify_result.cert_status &
                CERT_STATUS_WEAK_SIGNATURE_ALGORITHM);
  }
}

TEST(CertVerifyProcTest, RejectsPrivateSHA1UnlessFlag) {
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;
  result.has_sha1 = true;
  result.has_sha1_leaf = true;
  result.is_issued_by_known_root = false;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  // SHA-1 should be rejected by default for private roots...
  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  nullptr /* crl_set */, CertificateList(),
                                  &verify_result);
  EXPECT_THAT(error, IsError(ERR_CERT_WEAK_SIGNATURE_ALGORITHM));
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_SHA1_SIGNATURE_PRESENT);

  // ... unless VERIFY_ENABLE_SHA1_LOCAL_ANCHORS was supplied.
  flags = CertVerifier::VERIFY_ENABLE_SHA1_LOCAL_ANCHORS;
  verify_result.Reset();
  error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                              nullptr /* crl_set */, CertificateList(),
                              &verify_result);
  EXPECT_THAT(error, IsOk());
  EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_SHA1_SIGNATURE_PRESENT);
}

enum ExpectedAlgorithms {
  EXPECT_MD2 = 1 << 0,
  EXPECT_MD4 = 1 << 1,
  EXPECT_MD5 = 1 << 2,
  EXPECT_SHA1 = 1 << 3,
  EXPECT_SHA1_LEAF = 1 << 4,
};

struct WeakDigestTestData {
  const char* root_cert_filename;
  const char* intermediate_cert_filename;
  const char* ee_cert_filename;
  int expected_algorithms;
};

const char* StringOrDefault(const char* str, const char* default_value) {
  if (!str)
    return default_value;
  return str;
}

// GTest 'magic' pretty-printer, so that if/when a test fails, it knows how
// to output the parameter that was passed. Without this, it will simply
// attempt to print out the first twenty bytes of the object, which depending
// on platform and alignment, may result in an invalid read.
void PrintTo(const WeakDigestTestData& data, std::ostream* os) {
  *os << "root: " << StringOrDefault(data.root_cert_filename, "none")
      << "; intermediate: "
      << StringOrDefault(data.intermediate_cert_filename, "none")
      << "; end-entity: " << data.ee_cert_filename;
}

class CertVerifyProcWeakDigestTest
    : public testing::TestWithParam<WeakDigestTestData> {
 public:
  CertVerifyProcWeakDigestTest() = default;
  virtual ~CertVerifyProcWeakDigestTest() = default;
};

// Tests that the CertVerifyProc::Verify() properly surfaces the (weak) hash
// algorithms used in the chain.
TEST_P(CertVerifyProcWeakDigestTest, VerifyDetectsAlgorithm) {
  WeakDigestTestData data = GetParam();
  base::FilePath certs_dir = GetTestCertsDirectory();

  // Build |intermediates| as the full chain (including trust anchor).
  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;

  if (data.intermediate_cert_filename) {
    scoped_refptr<X509Certificate> intermediate_cert =
        ImportCertFromFile(certs_dir, data.intermediate_cert_filename);
    ASSERT_TRUE(intermediate_cert);
    intermediates.push_back(
        x509_util::DupCryptoBuffer(intermediate_cert->cert_buffer()));
  }

  if (data.root_cert_filename) {
    scoped_refptr<X509Certificate> root_cert =
        ImportCertFromFile(certs_dir, data.root_cert_filename);
    ASSERT_TRUE(root_cert);
    intermediates.push_back(
        x509_util::DupCryptoBuffer(root_cert->cert_buffer()));
  }

  scoped_refptr<X509Certificate> ee_cert =
      ImportCertFromFile(certs_dir, data.ee_cert_filename);
  ASSERT_TRUE(ee_cert);

  scoped_refptr<X509Certificate> ee_chain = X509Certificate::CreateFromBuffer(
      x509_util::DupCryptoBuffer(ee_cert->cert_buffer()),
      std::move(intermediates));
  ASSERT_TRUE(ee_chain);

  int flags = 0;
  CertVerifyResult verify_result;

  // Use a mock CertVerifyProc that returns success with a verified_cert of
  // |ee_chain|.
  //
  // This is sufficient for the purposes of this test, as the checking for weak
  // hash algorithms is done by CertVerifyProc::Verify().
  scoped_refptr<CertVerifyProc> proc =
      new MockCertVerifyProc(CertVerifyResult());
  proc->Verify(ee_chain.get(), "127.0.0.1", std::string(), flags, nullptr,
               CertificateList(), &verify_result);
  EXPECT_EQ(!!(data.expected_algorithms & EXPECT_MD2), verify_result.has_md2);
  EXPECT_EQ(!!(data.expected_algorithms & EXPECT_MD4), verify_result.has_md4);
  EXPECT_EQ(!!(data.expected_algorithms & EXPECT_MD5), verify_result.has_md5);
  EXPECT_EQ(!!(data.expected_algorithms & EXPECT_SHA1), verify_result.has_sha1);
  EXPECT_EQ(!!(data.expected_algorithms & EXPECT_SHA1_LEAF),
            verify_result.has_sha1_leaf);
}

// The signature algorithm of the root CA should not matter.
const WeakDigestTestData kVerifyRootCATestData[] = {
    {"weak_digest_md5_root.pem", "weak_digest_sha1_intermediate.pem",
     "weak_digest_sha1_ee.pem", EXPECT_SHA1 | EXPECT_SHA1_LEAF},
    {"weak_digest_md4_root.pem", "weak_digest_sha1_intermediate.pem",
     "weak_digest_sha1_ee.pem", EXPECT_SHA1 | EXPECT_SHA1_LEAF},
    {"weak_digest_md2_root.pem", "weak_digest_sha1_intermediate.pem",
     "weak_digest_sha1_ee.pem", EXPECT_SHA1 | EXPECT_SHA1_LEAF},
};
INSTANTIATE_TEST_CASE_P(VerifyRoot,
                        CertVerifyProcWeakDigestTest,
                        testing::ValuesIn(kVerifyRootCATestData));

// The signature algorithm of intermediates should be properly detected.
const WeakDigestTestData kVerifyIntermediateCATestData[] = {
    {"weak_digest_sha1_root.pem", "weak_digest_md5_intermediate.pem",
     "weak_digest_sha1_ee.pem", EXPECT_MD5 | EXPECT_SHA1 | EXPECT_SHA1_LEAF},
    {"weak_digest_sha1_root.pem", "weak_digest_md4_intermediate.pem",
     "weak_digest_sha1_ee.pem", EXPECT_MD4 | EXPECT_SHA1 | EXPECT_SHA1_LEAF},
    {"weak_digest_sha1_root.pem", "weak_digest_md2_intermediate.pem",
     "weak_digest_sha1_ee.pem", EXPECT_MD2 | EXPECT_SHA1 | EXPECT_SHA1_LEAF},
};

INSTANTIATE_TEST_CASE_P(VerifyIntermediate,
                        CertVerifyProcWeakDigestTest,
                        testing::ValuesIn(kVerifyIntermediateCATestData));

// The signature algorithm of end-entity should be properly detected.
const WeakDigestTestData kVerifyEndEntityTestData[] = {
    {"weak_digest_sha1_root.pem", "weak_digest_sha1_intermediate.pem",
     "weak_digest_md5_ee.pem", EXPECT_MD5 | EXPECT_SHA1},
    {"weak_digest_sha1_root.pem", "weak_digest_sha1_intermediate.pem",
     "weak_digest_md4_ee.pem", EXPECT_MD4 | EXPECT_SHA1},
    {"weak_digest_sha1_root.pem", "weak_digest_sha1_intermediate.pem",
     "weak_digest_md2_ee.pem", EXPECT_MD2 | EXPECT_SHA1},
};

INSTANTIATE_TEST_CASE_P(VerifyEndEntity,
                        CertVerifyProcWeakDigestTest,
                        testing::ValuesIn(kVerifyEndEntityTestData));

// Incomplete chains do not report the status of the intermediate.
// Note: really each of these tests should also expect the digest algorithm of
// the intermediate (included as a comment). However CertVerifyProc::Verify() is
// unable to distinguish that this is an intermediate and not a trust anchor, so
// this intermediate is treated like a trust anchor.
const WeakDigestTestData kVerifyIncompleteIntermediateTestData[] = {
    {NULL, "weak_digest_md5_intermediate.pem", "weak_digest_sha1_ee.pem",
     /*EXPECT_MD5 |*/ EXPECT_SHA1 | EXPECT_SHA1_LEAF},
    {NULL, "weak_digest_md4_intermediate.pem", "weak_digest_sha1_ee.pem",
     /*EXPECT_MD4 |*/ EXPECT_SHA1 | EXPECT_SHA1_LEAF},
    {NULL, "weak_digest_md2_intermediate.pem", "weak_digest_sha1_ee.pem",
     /*EXPECT_MD2 |*/ EXPECT_SHA1 | EXPECT_SHA1_LEAF},
};

INSTANTIATE_TEST_CASE_P(
    MAYBE_VerifyIncompleteIntermediate,
    CertVerifyProcWeakDigestTest,
    testing::ValuesIn(kVerifyIncompleteIntermediateTestData));

// Incomplete chains should report the status of the end-entity.
// Note: really each of these tests should also expect EXPECT_SHA1 (included as
// a comment). However CertVerifyProc::Verify() is unable to distinguish that
// this is an intermediate and not a trust anchor, so this intermediate is
// treated like a trust anchor.
const WeakDigestTestData kVerifyIncompleteEETestData[] = {
    {NULL, "weak_digest_sha1_intermediate.pem", "weak_digest_md5_ee.pem",
     /*EXPECT_SHA1 |*/ EXPECT_MD5},
    {NULL, "weak_digest_sha1_intermediate.pem", "weak_digest_md4_ee.pem",
     /*EXPECT_SHA1 |*/ EXPECT_MD4},
    {NULL, "weak_digest_sha1_intermediate.pem", "weak_digest_md2_ee.pem",
     /*EXPECT_SHA1 |*/ EXPECT_MD2},
};

INSTANTIATE_TEST_CASE_P(VerifyIncompleteEndEntity,
                        CertVerifyProcWeakDigestTest,
                        testing::ValuesIn(kVerifyIncompleteEETestData));

// Differing algorithms between the intermediate and the EE should still be
// reported.
const WeakDigestTestData kVerifyMixedTestData[] = {
    {"weak_digest_sha1_root.pem", "weak_digest_md5_intermediate.pem",
     "weak_digest_md2_ee.pem", EXPECT_MD2 | EXPECT_MD5},
    {"weak_digest_sha1_root.pem", "weak_digest_md2_intermediate.pem",
     "weak_digest_md5_ee.pem", EXPECT_MD2 | EXPECT_MD5},
    {"weak_digest_sha1_root.pem", "weak_digest_md4_intermediate.pem",
     "weak_digest_md2_ee.pem", EXPECT_MD2 | EXPECT_MD4},
};

INSTANTIATE_TEST_CASE_P(VerifyMixed,
                        CertVerifyProcWeakDigestTest,
                        testing::ValuesIn(kVerifyMixedTestData));

// The EE is a trusted certificate. Even though it uses weak hashes, these
// should not be reported.
const WeakDigestTestData kVerifyTrustedEETestData[] = {
    {NULL, NULL, "weak_digest_md5_ee.pem", 0},
    {NULL, NULL, "weak_digest_md4_ee.pem", 0},
    {NULL, NULL, "weak_digest_md2_ee.pem", 0},
    {NULL, NULL, "weak_digest_sha1_ee.pem", 0},
};

INSTANTIATE_TEST_CASE_P(VerifyTrustedEE,
                        CertVerifyProcWeakDigestTest,
                        testing::ValuesIn(kVerifyTrustedEETestData));

// Test fixture for verifying certificate names.
class CertVerifyProcNameTest : public ::testing::Test {
 protected:
  void VerifyCertName(const char* hostname, bool valid) {
    scoped_refptr<X509Certificate> cert(ImportCertFromFile(
        GetTestCertsDirectory(), "subjectAltName_sanity_check.pem"));
    ASSERT_TRUE(cert);
    CertVerifyResult result;
    result.is_issued_by_known_root = false;
    scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

    CertVerifyResult verify_result;
    int error = verify_proc->Verify(cert.get(), hostname, std::string(), 0,
                                    nullptr, CertificateList(), &verify_result);
    if (valid) {
      EXPECT_THAT(error, IsOk());
      EXPECT_FALSE(verify_result.cert_status & CERT_STATUS_COMMON_NAME_INVALID);
    } else {
      EXPECT_THAT(error, IsError(ERR_CERT_COMMON_NAME_INVALID));
      EXPECT_TRUE(verify_result.cert_status & CERT_STATUS_COMMON_NAME_INVALID);
    }
  }
};

// Don't match the common name
TEST_F(CertVerifyProcNameTest, DontMatchCommonName) {
  VerifyCertName("127.0.0.1", false);
}

// Matches the iPAddress SAN (IPv4)
TEST_F(CertVerifyProcNameTest, MatchesIpSanIpv4) {
  VerifyCertName("127.0.0.2", true);
}

// Matches the iPAddress SAN (IPv6)
TEST_F(CertVerifyProcNameTest, MatchesIpSanIpv6) {
  VerifyCertName("FE80:0:0:0:0:0:0:1", true);
}

// Should not match the iPAddress SAN
TEST_F(CertVerifyProcNameTest, DoesntMatchIpSanIpv6) {
  VerifyCertName("[FE80:0:0:0:0:0:0:1]", false);
}

// Compressed form matches the iPAddress SAN (IPv6)
TEST_F(CertVerifyProcNameTest, MatchesIpSanCompressedIpv6) {
  VerifyCertName("FE80::1", true);
}

// IPv6 mapped form should NOT match iPAddress SAN
TEST_F(CertVerifyProcNameTest, DoesntMatchIpSanIPv6Mapped) {
  VerifyCertName("::127.0.0.2", false);
}

// Matches the dNSName SAN
TEST_F(CertVerifyProcNameTest, MatchesDnsSan) {
  VerifyCertName("test.example", true);
}

// Matches the dNSName SAN (trailing . ignored)
TEST_F(CertVerifyProcNameTest, MatchesDnsSanTrailingDot) {
  VerifyCertName("test.example.", true);
}

// Should not match the dNSName SAN
TEST_F(CertVerifyProcNameTest, DoesntMatchDnsSan) {
  VerifyCertName("www.test.example", false);
}

// Should not match the dNSName SAN
TEST_F(CertVerifyProcNameTest, DoesntMatchDnsSanInvalid) {
  VerifyCertName("test..example", false);
}

// Should not match the dNSName SAN
TEST_F(CertVerifyProcNameTest, DoesntMatchDnsSanTwoTrailingDots) {
  VerifyCertName("test.example..", false);
}

// Should not match the dNSName SAN
TEST_F(CertVerifyProcNameTest, DoesntMatchDnsSanLeadingAndTrailingDot) {
  VerifyCertName(".test.example.", false);
}

// Should not match the dNSName SAN
TEST_F(CertVerifyProcNameTest, DoesntMatchDnsSanTrailingDot) {
  VerifyCertName(".test.example", false);
}

// Tests that CertVerifyProc records a histogram correctly when a
// certificate chaining to a private root contains the TLS feature
// extension and does not have a stapled OCSP response.
TEST(CertVerifyProcTest, HasTLSFeatureExtensionUMA) {
  base::HistogramTester histograms;
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "tls_feature_extension.pem"));
  ASSERT_TRUE(cert);
  CertVerifyResult result;
  result.is_issued_by_known_root = false;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  histograms.ExpectTotalCount(kTLSFeatureExtensionHistogram, 0);
  histograms.ExpectTotalCount(kTLSFeatureExtensionOCSPHistogram, 0);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  NULL, CertificateList(), &verify_result);
  EXPECT_EQ(OK, error);
  histograms.ExpectTotalCount(kTLSFeatureExtensionHistogram, 1);
  histograms.ExpectBucketCount(kTLSFeatureExtensionHistogram, true, 1);
  histograms.ExpectTotalCount(kTLSFeatureExtensionOCSPHistogram, 1);
  histograms.ExpectBucketCount(kTLSFeatureExtensionOCSPHistogram, false, 1);
}

// Tests that CertVerifyProc records a histogram correctly when a
// certificate chaining to a private root contains the TLS feature
// extension and does have a stapled OCSP response.
TEST(CertVerifyProcTest, HasTLSFeatureExtensionWithStapleUMA) {
  base::HistogramTester histograms;
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "tls_feature_extension.pem"));
  ASSERT_TRUE(cert);
  CertVerifyResult result;
  result.is_issued_by_known_root = false;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  histograms.ExpectTotalCount(kTLSFeatureExtensionHistogram, 0);
  histograms.ExpectTotalCount(kTLSFeatureExtensionOCSPHistogram, 0);

  int flags = 0;
  CertVerifyResult verify_result;
  int error =
      verify_proc->Verify(cert.get(), "127.0.0.1", "dummy response", flags,
                          nullptr, CertificateList(), &verify_result);
  EXPECT_EQ(OK, error);
  histograms.ExpectTotalCount(kTLSFeatureExtensionHistogram, 1);
  histograms.ExpectBucketCount(kTLSFeatureExtensionHistogram, true, 1);
  histograms.ExpectTotalCount(kTLSFeatureExtensionOCSPHistogram, 1);
  histograms.ExpectBucketCount(kTLSFeatureExtensionOCSPHistogram, true, 1);
}

// Tests that CertVerifyProc records a histogram correctly when a
// certificate chaining to a private root does not contain the TLS feature
// extension.
TEST(CertVerifyProcTest, DoesNotHaveTLSFeatureExtensionUMA) {
  base::HistogramTester histograms;
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(cert);
  CertVerifyResult result;
  result.is_issued_by_known_root = false;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  histograms.ExpectTotalCount(kTLSFeatureExtensionHistogram, 0);
  histograms.ExpectTotalCount(kTLSFeatureExtensionOCSPHistogram, 0);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  NULL, CertificateList(), &verify_result);
  EXPECT_EQ(OK, error);
  histograms.ExpectTotalCount(kTLSFeatureExtensionHistogram, 1);
  histograms.ExpectBucketCount(kTLSFeatureExtensionHistogram, false, 1);
  histograms.ExpectTotalCount(kTLSFeatureExtensionOCSPHistogram, 0);
}

// Tests that CertVerifyProc does not record a histogram when a
// certificate contains the TLS feature extension but chains to a public
// root.
TEST(CertVerifyProcTest, HasTLSFeatureExtensionWithPublicRootUMA) {
  base::HistogramTester histograms;
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "tls_feature_extension.pem"));
  ASSERT_TRUE(cert);
  CertVerifyResult result;
  result.is_issued_by_known_root = true;
  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  histograms.ExpectTotalCount(kTLSFeatureExtensionHistogram, 0);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  NULL, CertificateList(), &verify_result);
  EXPECT_EQ(OK, error);
  histograms.ExpectTotalCount(kTLSFeatureExtensionHistogram, 0);
  histograms.ExpectTotalCount(kTLSFeatureExtensionOCSPHistogram, 0);
}

// Test that trust anchors are appropriately recorded via UMA.
TEST(CertVerifyProcTest, HasTrustAnchorVerifyUMA) {
  base::HistogramTester histograms;
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;

  // Simulate a certificate chain issued by "C=US, O=Google Trust Services LLC,
  // CN=GTS Root R4". This publicly-trusted root was chosen as it was included
  // in 2017 and is not anticipated to be removed from all supported platforms
  // for a few decades.
  // Note: The actual cert in |cert| does not matter for this testing, so long
  // as it's not violating any CertVerifyProc::Verify() policies.
  SHA256HashValue leaf_hash = {{0}};
  SHA256HashValue intermediate_hash = {{1}};
  SHA256HashValue root_hash = {
      {0x98, 0x47, 0xe5, 0x65, 0x3e, 0x5e, 0x9e, 0x84, 0x75, 0x16, 0xe5,
       0xcb, 0x81, 0x86, 0x06, 0xaa, 0x75, 0x44, 0xa1, 0x9b, 0xe6, 0x7f,
       0xd7, 0x36, 0x6d, 0x50, 0x69, 0x88, 0xe8, 0xd8, 0x43, 0x47}};
  result.public_key_hashes.push_back(HashValue(leaf_hash));
  result.public_key_hashes.push_back(HashValue(intermediate_hash));
  result.public_key_hashes.push_back(HashValue(root_hash));

  const base::HistogramBase::Sample kGTSRootR4HistogramID = 486;

  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  histograms.ExpectTotalCount(kTrustAnchorVerifyHistogram, 0);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  NULL, CertificateList(), &verify_result);
  EXPECT_EQ(OK, error);
  histograms.ExpectUniqueSample(kTrustAnchorVerifyHistogram,
                                kGTSRootR4HistogramID, 1);
}

// Test that certificates with multiple trust anchors present result in
// only a single trust anchor being recorded, and that being the most specific
// trust anchor.
TEST(CertVerifyProcTest, LogsOnlyMostSpecificTrustAnchorUMA) {
  base::HistogramTester histograms;
  scoped_refptr<X509Certificate> cert(
      ImportCertFromFile(GetTestCertsDirectory(), "ok_cert.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;

  // Simulate a chain of "C=US, O=Google Trust Services LLC, CN=GTS Root R4"
  // signing "C=US, O=Google Trust Services LLC, CN=GTS Root R3" signing an
  // intermediate and a leaf.
  // Note: The actual cert in |cert| does not matter for this testing, so long
  // as it's not violating any CertVerifyProc::Verify() policies.
  SHA256HashValue leaf_hash = {{0}};
  SHA256HashValue intermediate_hash = {{1}};
  SHA256HashValue gts_root_r3_hash = {
      {0x41, 0x79, 0xed, 0xd9, 0x81, 0xef, 0x74, 0x74, 0x77, 0xb4, 0x96,
       0x26, 0x40, 0x8a, 0xf4, 0x3d, 0xaa, 0x2c, 0xa7, 0xab, 0x7f, 0x9e,
       0x08, 0x2c, 0x10, 0x60, 0xf8, 0x40, 0x96, 0x77, 0x43, 0x48}};
  SHA256HashValue gts_root_r4_hash = {
      {0x98, 0x47, 0xe5, 0x65, 0x3e, 0x5e, 0x9e, 0x84, 0x75, 0x16, 0xe5,
       0xcb, 0x81, 0x86, 0x06, 0xaa, 0x75, 0x44, 0xa1, 0x9b, 0xe6, 0x7f,
       0xd7, 0x36, 0x6d, 0x50, 0x69, 0x88, 0xe8, 0xd8, 0x43, 0x47}};
  result.public_key_hashes.push_back(HashValue(leaf_hash));
  result.public_key_hashes.push_back(HashValue(intermediate_hash));
  result.public_key_hashes.push_back(HashValue(gts_root_r3_hash));
  result.public_key_hashes.push_back(HashValue(gts_root_r4_hash));

  const base::HistogramBase::Sample kGTSRootR3HistogramID = 485;

  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  histograms.ExpectTotalCount(kTrustAnchorVerifyHistogram, 0);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  NULL, CertificateList(), &verify_result);
  EXPECT_EQ(OK, error);

  // Only GTS Root R3 should be recorded.
  histograms.ExpectUniqueSample(kTrustAnchorVerifyHistogram,
                                kGTSRootR3HistogramID, 1);
}

// Test that trust anchors histograms record whether or not
// is_issued_by_known_root was derived from the OS.
TEST(CertVerifyProcTest, HasTrustAnchorVerifyOutOfDateUMA) {
  base::HistogramTester histograms;
  scoped_refptr<X509Certificate> cert(ImportCertFromFile(
      GetTestCertsDirectory(), "39_months_based_on_last_day.pem"));
  ASSERT_TRUE(cert);

  CertVerifyResult result;

  // Simulate a certificate chain that is recognized as trusted (from a known
  // root), but no certificates in the chain are tracked as known trust
  // anchors.
  SHA256HashValue leaf_hash = {{0}};
  SHA256HashValue intermediate_hash = {{1}};
  SHA256HashValue root_hash = {{2}};
  result.public_key_hashes.push_back(HashValue(leaf_hash));
  result.public_key_hashes.push_back(HashValue(intermediate_hash));
  result.public_key_hashes.push_back(HashValue(root_hash));
  result.is_issued_by_known_root = true;

  scoped_refptr<CertVerifyProc> verify_proc = new MockCertVerifyProc(result);

  histograms.ExpectTotalCount(kTrustAnchorVerifyHistogram, 0);
  histograms.ExpectTotalCount(kTrustAnchorVerifyOutOfDateHistogram, 0);

  int flags = 0;
  CertVerifyResult verify_result;
  int error = verify_proc->Verify(cert.get(), "127.0.0.1", std::string(), flags,
                                  NULL, CertificateList(), &verify_result);
  EXPECT_EQ(OK, error);
  const base::HistogramBase::Sample kUnknownRootHistogramID = 0;
  histograms.ExpectUniqueSample(kTrustAnchorVerifyHistogram,
                                kUnknownRootHistogramID, 1);
  histograms.ExpectUniqueSample(kTrustAnchorVerifyOutOfDateHistogram, true, 1);
}

}  // namespace net
