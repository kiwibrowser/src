// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/session_certificate_policy_cache_impl.h"

#import "base/mac/bind_objc_block.h"
#import "ios/testing/wait_util.h"
#include "ios/web/public/certificate_policy_cache.h"
#import "ios/web/public/crw_session_certificate_policy_cache_storage.h"
#include "ios/web/public/test/fakes/test_browser_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "ios/web/public/web_thread.h"
#include "net/cert/x509_certificate.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::WaitUntilConditionOrTimeout;
using testing::kWaitForJSCompletionTimeout;

namespace {
// Synchronously checks |cache| for the specified cert and returns the judgment.
web::CertPolicy::Judgment GetJudgmenet(
    const scoped_refptr<web::CertificatePolicyCache>& cache,
    const scoped_refptr<net::X509Certificate>& cert,
    const std::string& host,
    const net::CertStatus status) {
  // Post a task to the IO thread and wait for a reply
  __block web::CertPolicy::Judgment judgement =
      web::CertPolicy::Judgment::UNKNOWN;
  __block bool completed = false;
  web::WebThread::PostTask(web::WebThread::IO, FROM_HERE, base::BindBlockArc(^{
                             completed = true;
                             judgement =
                                 cache->QueryPolicy(cert.get(), host, status);
                           }));
  EXPECT_TRUE(WaitUntilConditionOrTimeout(1.0, ^{
    return completed;
  }));
  return judgement;
}

}  // namespace

// Test fixture to test SessionCertificatePolicyCacheImpl class.
class SessionCertificatePolicyCacheImplTest : public PlatformTest {
 protected:
  SessionCertificatePolicyCacheImplTest()
      : thread_bundle_(web::TestWebThreadBundle::Options::REAL_IO_THREAD),
        cert_(net::ImportCertFromFile(net::GetTestCertsDirectory(),
                                      "ok_cert.pem")),
        host_("test.com"),
        status_(net::CERT_STATUS_REVOKED) {
    cache_.RegisterAllowedCertificate(cert_, host_, status_);
  }

  web::TestWebThreadBundle thread_bundle_;
  web::SessionCertificatePolicyCacheImpl cache_;
  scoped_refptr<net::X509Certificate> cert_;
  std::string host_;
  net::CertStatus status_;
};

// Tests that registering an allowed certificate will successfully create an
// accurate CRWSessionCertificateStorage in the allowed certs set.
TEST_F(SessionCertificatePolicyCacheImplTest, RegisterAllowedCert) {
  // Verify that the cert information is added to the cache.
  EXPECT_EQ(1U, cache_.GetAllowedCerts().count);
  CRWSessionCertificateStorage* cert_storage =
      [cache_.GetAllowedCerts() anyObject];
  EXPECT_EQ(cert_.get(), cert_storage.certificate);
  EXPECT_EQ(host_, cert_storage.host);
  EXPECT_EQ(status_, cert_storage.status);
}

// Tests that ClearAllowedCerts() successfully removes all previously added
// certs.
TEST_F(SessionCertificatePolicyCacheImplTest, ClearAllowedCerts) {
  // Clear the certs and verify that it has been removed.
  EXPECT_EQ(1U, cache_.GetAllowedCerts().count);
  cache_.ClearAllowedCertificates();
  EXPECT_EQ(0U, cache_.GetAllowedCerts().count);
}

// Tests that UpdateCertificatePolicyCache() successfully transfers the allowed
// certificate information to a CertificatePolicyCache.
TEST_F(SessionCertificatePolicyCacheImplTest, UpdateCertificatePolicyCache) {
  // Create a CertificatePolicyCache.
  web::TestBrowserState browser_state;
  scoped_refptr<web::CertificatePolicyCache> cache =
      web::BrowserState::GetCertificatePolicyCache(&browser_state);
  EXPECT_EQ(web::CertPolicy::Judgment::UNKNOWN,
            GetJudgmenet(cache, cert_, host_, status_));
  // Add allowed certificates to the CertificatePolicyCache and verify that it's
  // now allowed by the CertificatePolicyCache.
  cache_.UpdateCertificatePolicyCache(cache);
  EXPECT_EQ(web::CertPolicy::Judgment::ALLOWED,
            GetJudgmenet(cache, cert_, host_, status_));
}
