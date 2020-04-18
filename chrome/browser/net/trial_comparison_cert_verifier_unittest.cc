// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/trial_comparison_cert_verifier.h"

#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service_factory.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service_test_utils.h"
#include "chrome/browser/safe_browsing/test_safe_browsing_service.h"
#include "chrome/browser/ssl/cert_logger.pb.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/cert/cert_verify_proc.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/socket_test_util.h"
#include "net/test/cert_test_util.h"
#include "net/test/gtest_util.h"
#include "net/test/test_data_directory.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using certificate_reporting_test_utils::CertificateReportingServiceTestHelper;
using certificate_reporting_test_utils::ReportExpectation;
using certificate_reporting_test_utils::RetryStatus;
using net::test::IsError;

namespace {

MATCHER_P(CertChainMatches, expected_cert, "") {
  net::CertificateList actual_certs =
      net::X509Certificate::CreateCertificateListFromBytes(
          arg.data(), arg.size(),
          net::X509Certificate::FORMAT_PEM_CERT_SEQUENCE);
  if (actual_certs.empty()) {
    *result_listener << "failed to parse arg";
    return false;
  }
  std::vector<std::string> actual_der_certs;
  for (const auto& cert : actual_certs) {
    actual_der_certs.emplace_back(
        net::x509_util::CryptoBufferAsStringPiece(cert->cert_buffer()));
  }

  std::vector<std::string> expected_der_certs;
  expected_der_certs.emplace_back(
      net::x509_util::CryptoBufferAsStringPiece(expected_cert->cert_buffer()));
  for (const auto& buffer : expected_cert->intermediate_buffers()) {
    expected_der_certs.emplace_back(
        net::x509_util::CryptoBufferAsStringPiece(buffer.get()));
  }

  return actual_der_certs == expected_der_certs;
}

// Mock CertVerifyProc that sets the CertVerifyResult to a given value for
// all certificates that are Verify()'d
class MockCertVerifyProc : public net::CertVerifyProc {
 public:
  explicit MockCertVerifyProc(const int result_error,
                              const net::CertVerifyResult& result)
      : result_error_(result_error), result_(result) {}

  void WaitForVerifyCall() {
    verify_called_.WaitForResult();
    // Ensure MultiThreadedCertVerifier OnJobCompleted task has a chance to run.
    content::RunAllTasksUntilIdle();
  }

  // CertVerifyProc implementation:
  bool SupportsAdditionalTrustAnchors() const override { return false; }
  bool SupportsOCSPStapling() const override { return false; }

 protected:
  ~MockCertVerifyProc() override = default;

 private:
  int VerifyInternal(net::X509Certificate* cert,
                     const std::string& hostname,
                     const std::string& ocsp_response,
                     int flags,
                     net::CRLSet* crl_set,
                     const net::CertificateList& additional_trust_anchors,
                     net::CertVerifyResult* verify_result) override;

  const int result_error_;
  const net::CertVerifyResult result_;
  net::TestClosure verify_called_;

  DISALLOW_COPY_AND_ASSIGN(MockCertVerifyProc);
};

int MockCertVerifyProc::VerifyInternal(
    net::X509Certificate* cert,
    const std::string& hostname,
    const std::string& ocsp_response,
    int flags,
    net::CRLSet* crl_set,
    const net::CertificateList& additional_trust_anchors,
    net::CertVerifyResult* verify_result) {
  *verify_result = result_;
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)
      ->PostTask(FROM_HERE, verify_called_.closure());
  return result_error_;
}

// Mock CertVerifyProc that causes a failure if it is called.
class NotCalledCertVerifyProc : public net::CertVerifyProc {
 public:
  NotCalledCertVerifyProc() = default;

  // CertVerifyProc implementation:
  bool SupportsAdditionalTrustAnchors() const override { return false; }
  bool SupportsOCSPStapling() const override { return false; }

 protected:
  ~NotCalledCertVerifyProc() override = default;

 private:
  int VerifyInternal(net::X509Certificate* cert,
                     const std::string& hostname,
                     const std::string& ocsp_response,
                     int flags,
                     net::CRLSet* crl_set,
                     const net::CertificateList& additional_trust_anchors,
                     net::CertVerifyResult* verify_result) override;

  DISALLOW_COPY_AND_ASSIGN(NotCalledCertVerifyProc);
};

int NotCalledCertVerifyProc::VerifyInternal(
    net::X509Certificate* cert,
    const std::string& hostname,
    const std::string& ocsp_response,
    int flags,
    net::CRLSet* crl_set,
    const net::CertificateList& additional_trust_anchors,
    net::CertVerifyResult* verify_result) {
  ADD_FAILURE() << "NotCalledCertVerifyProc was called!";
  return net::ERR_UNEXPECTED;
}

void NotCalledCallback(int error) {
  ADD_FAILURE() << "NotCalledCallback was called with error code " << error;
}

}  // namespace

class TrialComparisonCertVerifierTest : public testing::Test {
 public:
  TrialComparisonCertVerifierTest()
      // UI and IO message loops run on the same thread for the test. (Makes
      // the test logic simpler, though doesn't fully exercise the
      // ThreadCheckers.)
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    cert_chain_1_ = CreateCertificateChainFromFile(
        net::GetTestCertsDirectory(), "multi-root-chain1.pem",
        net::X509Certificate::FORMAT_AUTO);
    ASSERT_TRUE(cert_chain_1_);
    leaf_cert_1_ = net::X509Certificate::CreateFromBuffer(
        net::x509_util::DupCryptoBuffer(cert_chain_1_->cert_buffer()), {});
    ASSERT_TRUE(leaf_cert_1_);
    cert_chain_2_ = CreateCertificateChainFromFile(
        net::GetTestCertsDirectory(), "multi-root-chain2.pem",
        net::X509Certificate::FORMAT_AUTO);
    ASSERT_TRUE(cert_chain_2_);

    reporting_service_test_helper_ =
        base::MakeRefCounted<CertificateReportingServiceTestHelper>();
    CertificateReportingServiceFactory::GetInstance()
        ->SetReportEncryptionParamsForTesting(
            reporting_service_test_helper()->server_public_key(),
            reporting_service_test_helper()->server_public_key_version());
    CertificateReportingServiceFactory::GetInstance()
        ->SetURLLoaderFactoryForTesting(reporting_service_test_helper_);
    reporting_service_test_helper()->SetFailureMode(
        certificate_reporting_test_utils::REPORTS_SUCCESSFUL);

    sb_service_ = base::MakeRefCounted<safe_browsing::TestSafeBrowsingService>(
        // Doesn't matter, just need to choose one.
        safe_browsing::V4FeatureList::V4UsageStatus::V4_DISABLED);
    TestingBrowserProcess::GetGlobal()->SetSafeBrowsingService(
        sb_service_.get());
    g_browser_process->safe_browsing_service()->Initialize();

    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
    ASSERT_TRUE(g_browser_process->profile_manager());
    profile_ = profile_manager_->CreateTestingProfile("profile1");

    // Enable feature and SBER pref.
    TrialComparisonCertVerifier::SetFakeOfficialBuildForTesting();
    scoped_feature_ = std::make_unique<base::test::ScopedFeatureList>();
    scoped_feature_->InitAndEnableFeature(
        features::kCertDualVerificationTrialFeature);
    safe_browsing::SetExtendedReportingPref(pref_service(), true);

    // Initialize CertificateReportingService
    ASSERT_TRUE(service());
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    if (TestingBrowserProcess::GetGlobal()->safe_browsing_service()) {
      TestingBrowserProcess::GetGlobal()->safe_browsing_service()->ShutDown();
      TestingBrowserProcess::GetGlobal()->SetSafeBrowsingService(nullptr);
    }
    net::URLRequestFilter::GetInstance()->ClearHandlers();
  }

  TestingProfile* profile() { return profile_; }
  sync_preferences::TestingPrefServiceSyncable* pref_service() {
    return profile_->GetTestingPrefService();
  }

  CertificateReportingServiceTestHelper* reporting_service_test_helper() {
    return reporting_service_test_helper_.get();
  }

  CertificateReportingService* service() const {
    return CertificateReportingServiceFactory::GetForBrowserContext(profile_);
  }

 protected:
  scoped_refptr<net::X509Certificate> cert_chain_1_;
  scoped_refptr<net::X509Certificate> cert_chain_2_;
  scoped_refptr<net::X509Certificate> leaf_cert_1_;
  base::HistogramTester histograms_;
  std::unique_ptr<base::test::ScopedFeatureList> scoped_feature_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  scoped_refptr<safe_browsing::SafeBrowsingService> sb_service_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  TestingProfile* profile_;

  scoped_refptr<CertificateReportingServiceTestHelper>
      reporting_service_test_helper_;
};

TEST_F(TrialComparisonCertVerifierTest, NotOptedIn) {
  // Disable SBER pref.
  safe_browsing::SetExtendedReportingPref(pref_service(), false);

  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = cert_chain_1_;
  TrialComparisonCertVerifier verifier(
      profile(),
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, dummy_result),
      base::MakeRefCounted<NotCalledCertVerifyProc>());
  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  // Wait for CheckTrialEligibility task to finish.
  content::RunAllTasksUntilIdle();

  // Expect no report.
  reporting_service_test_helper()->ExpectNoRequests(service());

  // Primary verifier should have ran, trial verifier should not have.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 0);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               0);
  histograms_.ExpectTotalCount("Net.CertVerifier_TrialComparisonResult", 0);
}

TEST_F(TrialComparisonCertVerifierTest, NotScoutOptIn) {
  // Disable SBER pref.
  safe_browsing::SetExtendedReportingPref(pref_service(), false);
  // Set the old, non-Scout SBER pref.
  pref_service()->SetBoolean(prefs::kSafeBrowsingExtendedReportingEnabled,
                             true);

  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = cert_chain_1_;
  TrialComparisonCertVerifier verifier(
      profile(),
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, dummy_result),
      base::MakeRefCounted<NotCalledCertVerifyProc>());
  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  // Wait for CheckTrialEligibility task to finish.
  content::RunAllTasksUntilIdle();

  // Expect no report.
  reporting_service_test_helper()->ExpectNoRequests(service());

  // Primary verifier should have ran, trial verifier should not have.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 0);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               0);
  histograms_.ExpectTotalCount("Net.CertVerifier_TrialComparisonResult", 0);
}

TEST_F(TrialComparisonCertVerifierTest, FeatureDisabled) {
  // Disable feature.
  scoped_feature_.reset();

  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = cert_chain_1_;
  TrialComparisonCertVerifier verifier(
      profile(),
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, dummy_result),
      base::MakeRefCounted<NotCalledCertVerifyProc>());
  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  // Wait for CheckTrialEligibility task to finish.
  content::RunAllTasksUntilIdle();

  // Expect no report.
  reporting_service_test_helper()->ExpectNoRequests(service());

  // Primary verifier should have ran, trial verifier should not have.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 0);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               0);
  histograms_.ExpectTotalCount("Net.CertVerifier_TrialComparisonResult", 0);
}

TEST_F(TrialComparisonCertVerifierTest, SameResult) {
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, dummy_result);
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, dummy_result);

  TrialComparisonCertVerifier verifier(profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  verify_proc2->WaitForVerifyCall();

  // Expect no report.
  reporting_service_test_helper()->ExpectNoRequests(service());

  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               1);
  histograms_.ExpectUniqueSample("Net.CertVerifier_TrialComparisonResult",
                                 TrialComparisonCertVerifier::kEqual, 1);
}

TEST_F(TrialComparisonCertVerifierTest, Incognito) {
  net::CertVerifyResult dummy_result;
  dummy_result.verified_cert = cert_chain_1_;
  TrialComparisonCertVerifier verifier(
      profile()->GetOffTheRecordProfile(),  // Use an incognito Profile.
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, dummy_result),
      base::MakeRefCounted<NotCalledCertVerifyProc>());
  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  // Wait for CheckTrialEligibility task to finish.
  content::RunAllTasksUntilIdle();

  // Primary verifier should have ran, trial verifier should not have, control
  // histogram also should not be recorded for incognito.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 0);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               0);
  histograms_.ExpectTotalCount("Net.CertVerifier_TrialComparisonResult", 0);
}

TEST_F(TrialComparisonCertVerifierTest, PrimaryVerifierErrorSecondaryOk) {
  // Primary verifier returns an error status.
  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  primary_result.cert_status = net::CERT_STATUS_REVOKED;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::ERR_CERT_REVOKED,
                                               primary_result);

  net::CertVerifyResult secondary_result;
  secondary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, secondary_result);

  TrialComparisonCertVerifier verifier(profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::ERR_CERT_REVOKED));

  verify_proc2->WaitForVerifyCall();

  // Expect a report.
  std::vector<std::string> full_reports;
  reporting_service_test_helper()->WaitForRequestsDestroyed(
      ReportExpectation::Successful({{"127.0.0.1", RetryStatus::NOT_RETRIED}}),
      &full_reports);

  ASSERT_EQ(1U, full_reports.size());

  chrome_browser_ssl::CertLoggerRequest report;
  ASSERT_TRUE(report.ParseFromString(full_reports[0]));

  ASSERT_EQ(1, report.cert_error_size());
  EXPECT_EQ(chrome_browser_ssl::CertLoggerRequest::ERR_CERT_REVOKED,
            report.cert_error()[0]);
  EXPECT_EQ(0, report.cert_status_size());

  ASSERT_TRUE(report.has_features_info());
  ASSERT_TRUE(report.features_info().has_trial_verification_info());
  const chrome_browser_ssl::TrialVerificationInfo& trial_info =
      report.features_info().trial_verification_info();
  ASSERT_EQ(0, trial_info.cert_error_size());
  EXPECT_EQ(0, trial_info.cert_status_size());

  EXPECT_THAT(report.unverified_cert_chain(), CertChainMatches(leaf_cert_1_));
  EXPECT_THAT(report.cert_chain(), CertChainMatches(cert_chain_1_));
  EXPECT_THAT(trial_info.cert_chain(), CertChainMatches(cert_chain_1_));

  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               1);
  histograms_.ExpectUniqueSample(
      "Net.CertVerifier_TrialComparisonResult",
      TrialComparisonCertVerifier::kPrimaryErrorSecondaryValid, 1);
}

TEST_F(TrialComparisonCertVerifierTest, PrimaryVerifierOkSecondaryError) {
  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, primary_result);

  // Trial verifier returns an error status.
  net::CertVerifyResult secondary_result;
  secondary_result.cert_status = net::CERT_STATUS_REVOKED;
  secondary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::ERR_CERT_REVOKED,
                                               secondary_result);

  TrialComparisonCertVerifier verifier(profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  verify_proc2->WaitForVerifyCall();

  // Expect a report.
  std::vector<std::string> full_reports;
  reporting_service_test_helper()->WaitForRequestsDestroyed(
      ReportExpectation::Successful({{"127.0.0.1", RetryStatus::NOT_RETRIED}}),
      &full_reports);

  ASSERT_EQ(1U, full_reports.size());

  chrome_browser_ssl::CertLoggerRequest report;
  ASSERT_TRUE(report.ParseFromString(full_reports[0]));

  EXPECT_EQ(0, report.cert_error_size());
  EXPECT_EQ(0, report.cert_status_size());

  ASSERT_TRUE(report.has_features_info());
  ASSERT_TRUE(report.features_info().has_trial_verification_info());
  const chrome_browser_ssl::TrialVerificationInfo& trial_info =
      report.features_info().trial_verification_info();
  ASSERT_EQ(1, trial_info.cert_error_size());
  EXPECT_EQ(chrome_browser_ssl::CertLoggerRequest::ERR_CERT_REVOKED,
            trial_info.cert_error()[0]);
  EXPECT_EQ(0, trial_info.cert_status_size());

  EXPECT_THAT(report.unverified_cert_chain(), CertChainMatches(leaf_cert_1_));
  EXPECT_THAT(report.cert_chain(), CertChainMatches(cert_chain_1_));
  EXPECT_THAT(trial_info.cert_chain(), CertChainMatches(cert_chain_1_));

  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               1);
  histograms_.ExpectUniqueSample(
      "Net.CertVerifier_TrialComparisonResult",
      TrialComparisonCertVerifier::kPrimaryValidSecondaryError, 1);
}

TEST_F(TrialComparisonCertVerifierTest,
       BothVerifiersOk_DifferentVerifiedChains) {
  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, primary_result);

  // Trial verifier returns a different verified cert chain.
  net::CertVerifyResult secondary_result;
  secondary_result.verified_cert = cert_chain_2_;
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, secondary_result);

  TrialComparisonCertVerifier verifier(profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  verify_proc2->WaitForVerifyCall();

  // Expect a report.
  std::vector<std::string> full_reports;
  reporting_service_test_helper()->WaitForRequestsDestroyed(
      ReportExpectation::Successful({{"127.0.0.1", RetryStatus::NOT_RETRIED}}),
      &full_reports);

  ASSERT_EQ(1U, full_reports.size());

  chrome_browser_ssl::CertLoggerRequest report;
  ASSERT_TRUE(report.ParseFromString(full_reports[0]));

  EXPECT_EQ(0, report.cert_error_size());
  EXPECT_EQ(0, report.cert_status_size());

  ASSERT_TRUE(report.has_features_info());
  ASSERT_TRUE(report.features_info().has_trial_verification_info());
  const chrome_browser_ssl::TrialVerificationInfo& trial_info =
      report.features_info().trial_verification_info();
  EXPECT_EQ(0, trial_info.cert_error_size());
  EXPECT_EQ(0, trial_info.cert_status_size());
  EXPECT_EQ(0, trial_info.verify_flags_size());

  EXPECT_THAT(report.unverified_cert_chain(), CertChainMatches(leaf_cert_1_));
  EXPECT_THAT(report.cert_chain(), CertChainMatches(cert_chain_1_));
  EXPECT_THAT(trial_info.cert_chain(), CertChainMatches(cert_chain_2_));

  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               1);
  histograms_.ExpectUniqueSample(
      "Net.CertVerifier_TrialComparisonResult",
      TrialComparisonCertVerifier::kBothValidDifferentDetails, 1);
}

TEST_F(TrialComparisonCertVerifierTest, BothVerifiersOkDifferentCertStatus) {
  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  primary_result.cert_status =
      net::CERT_STATUS_IS_EV | net::CERT_STATUS_REV_CHECKING_ENABLED;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, primary_result);

  net::CertVerifyResult secondary_result;
  secondary_result.verified_cert = cert_chain_1_;
  secondary_result.cert_status = net::CERT_STATUS_CT_COMPLIANCE_FAILED;
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, secondary_result);

  TrialComparisonCertVerifier verifier(profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1",
      net::CertVerifier::VERIFY_ENABLE_SHA1_LOCAL_ANCHORS |
          net::CertVerifier::VERIFY_REV_CHECKING_ENABLED,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  verify_proc2->WaitForVerifyCall();

  // Expect a report.
  std::vector<std::string> full_reports;
  reporting_service_test_helper()->WaitForRequestsDestroyed(
      ReportExpectation::Successful({{"127.0.0.1", RetryStatus::NOT_RETRIED}}),
      &full_reports);

  ASSERT_EQ(1U, full_reports.size());

  chrome_browser_ssl::CertLoggerRequest report;
  ASSERT_TRUE(report.ParseFromString(full_reports[0]));

  EXPECT_EQ(0, report.cert_error_size());
  ASSERT_EQ(2, report.cert_status_size());
  EXPECT_EQ(chrome_browser_ssl::CertLoggerRequest::STATUS_IS_EV,
            report.cert_status()[0]);
  EXPECT_EQ(chrome_browser_ssl::CertLoggerRequest::STATUS_REV_CHECKING_ENABLED,
            report.cert_status()[1]);

  ASSERT_TRUE(report.has_features_info());
  ASSERT_TRUE(report.features_info().has_trial_verification_info());
  const chrome_browser_ssl::TrialVerificationInfo& trial_info =
      report.features_info().trial_verification_info();
  EXPECT_EQ(0, trial_info.cert_error_size());
  ASSERT_EQ(1, trial_info.cert_status_size());
  EXPECT_EQ(chrome_browser_ssl::CertLoggerRequest::STATUS_CT_COMPLIANCE_FAILED,
            trial_info.cert_status()[0]);

  EXPECT_THAT(
      trial_info.verify_flags(),
      testing::UnorderedElementsAre(chrome_browser_ssl::TrialVerificationInfo::
                                        VERIFY_REV_CHECKING_ENABLED,
                                    chrome_browser_ssl::TrialVerificationInfo::
                                        VERIFY_ENABLE_SHA1_LOCAL_ANCHORS));

  EXPECT_THAT(report.unverified_cert_chain(), CertChainMatches(leaf_cert_1_));
  EXPECT_THAT(report.cert_chain(), CertChainMatches(cert_chain_1_));
  EXPECT_THAT(trial_info.cert_chain(), CertChainMatches(cert_chain_1_));

  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               1);
  histograms_.ExpectUniqueSample(
      "Net.CertVerifier_TrialComparisonResult",
      TrialComparisonCertVerifier::kBothValidDifferentDetails, 1);
}

TEST_F(TrialComparisonCertVerifierTest, Coalescing) {
  // Primary verifier returns an error status.
  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  primary_result.cert_status = net::CERT_STATUS_REVOKED;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::ERR_CERT_REVOKED,
                                               primary_result);

  // Trial verifier has ok status.
  net::CertVerifyResult secondary_result;
  secondary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, secondary_result);

  TrialComparisonCertVerifier verifier(profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);

  // Start first verification request.
  net::CertVerifyResult result_1;
  std::unique_ptr<net::CertVerifier::Request> request_1;
  net::TestCompletionCallback callback_1;
  int error = verifier.Verify(params, nullptr /* crl_set */, &result_1,
                              callback_1.callback(), &request_1,
                              net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request_1);

  // Start second verification request with same params.
  net::CertVerifyResult result_2;
  std::unique_ptr<net::CertVerifier::Request> request_2;
  net::TestCompletionCallback callback_2;
  error = verifier.Verify(params, nullptr /* crl_set */, &result_2,
                          callback_2.callback(), &request_2,
                          net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request_2);

  // Both callbacks should be called with same error code.
  error = callback_1.WaitForResult();
  EXPECT_THAT(error, IsError(net::ERR_CERT_REVOKED));
  error = callback_2.WaitForResult();
  EXPECT_THAT(error, IsError(net::ERR_CERT_REVOKED));

  // Trial verifier should run.
  verify_proc2->WaitForVerifyCall();

  // Expect a single report.
  std::vector<std::string> full_reports;
  reporting_service_test_helper()->WaitForRequestsDestroyed(
      ReportExpectation::Successful({{"127.0.0.1", RetryStatus::NOT_RETRIED}}),
      &full_reports);

  ASSERT_EQ(1U, full_reports.size());

  chrome_browser_ssl::CertLoggerRequest report;
  ASSERT_TRUE(report.ParseFromString(full_reports[0]));

  ASSERT_EQ(1, report.cert_error_size());
  EXPECT_EQ(chrome_browser_ssl::CertLoggerRequest::ERR_CERT_REVOKED,
            report.cert_error()[0]);
  EXPECT_EQ(0, report.cert_status_size());

  ASSERT_TRUE(report.has_features_info());
  ASSERT_TRUE(report.features_info().has_trial_verification_info());
  const chrome_browser_ssl::TrialVerificationInfo& trial_info =
      report.features_info().trial_verification_info();
  ASSERT_EQ(0, trial_info.cert_error_size());
  EXPECT_EQ(0, trial_info.cert_status_size());

  EXPECT_THAT(report.unverified_cert_chain(), CertChainMatches(leaf_cert_1_));
  EXPECT_THAT(report.cert_chain(), CertChainMatches(cert_chain_1_));
  EXPECT_THAT(trial_info.cert_chain(), CertChainMatches(cert_chain_1_));

  // Only one verification should be done by primary verifier.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  // Only one verification should be done by secondary verifier.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               1);
  histograms_.ExpectUniqueSample(
      "Net.CertVerifier_TrialComparisonResult",
      TrialComparisonCertVerifier::kPrimaryErrorSecondaryValid, 1);
}

TEST_F(TrialComparisonCertVerifierTest, CancelledDuringPrimaryVerification) {
  // Primary verifier returns an error status.
  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  primary_result.cert_status = net::CERT_STATUS_REVOKED;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::ERR_CERT_REVOKED,
                                               primary_result);

  // Trial verifier has ok status.
  net::CertVerifyResult secondary_result;
  secondary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, secondary_result);

  TrialComparisonCertVerifier verifier(profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error = verifier.Verify(params, nullptr /* crl_set */, &result,
                              base::BindRepeating(&NotCalledCallback), &request,
                              net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  // Delete the request, cancelling it.
  request.reset();

  // The callback to the main verifier does not run. However, the verification
  // still completes in the background and triggers the trial verification.

  // Trial verifier should still run.
  verify_proc2->WaitForVerifyCall();

  // Expect a report.
  std::vector<std::string> full_reports;
  reporting_service_test_helper()->WaitForRequestsDestroyed(
      ReportExpectation::Successful({{"127.0.0.1", RetryStatus::NOT_RETRIED}}),
      &full_reports);

  ASSERT_EQ(1U, full_reports.size());

  chrome_browser_ssl::CertLoggerRequest report;
  ASSERT_TRUE(report.ParseFromString(full_reports[0]));

  ASSERT_EQ(1, report.cert_error_size());
  EXPECT_EQ(chrome_browser_ssl::CertLoggerRequest::ERR_CERT_REVOKED,
            report.cert_error()[0]);
  EXPECT_EQ(0, report.cert_status_size());

  ASSERT_TRUE(report.has_features_info());
  ASSERT_TRUE(report.features_info().has_trial_verification_info());
  const chrome_browser_ssl::TrialVerificationInfo& trial_info =
      report.features_info().trial_verification_info();
  ASSERT_EQ(0, trial_info.cert_error_size());
  EXPECT_EQ(0, trial_info.cert_status_size());

  EXPECT_THAT(report.unverified_cert_chain(), CertChainMatches(leaf_cert_1_));
  EXPECT_THAT(report.cert_chain(), CertChainMatches(cert_chain_1_));
  EXPECT_THAT(trial_info.cert_chain(), CertChainMatches(cert_chain_1_));

  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               1);
  histograms_.ExpectUniqueSample(
      "Net.CertVerifier_TrialComparisonResult",
      TrialComparisonCertVerifier::kPrimaryErrorSecondaryValid, 1);
}

TEST_F(TrialComparisonCertVerifierTest, DeletedDuringPrimaryVerification) {
  // Primary verifier returns an error status.
  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  primary_result.cert_status = net::CERT_STATUS_REVOKED;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::ERR_CERT_REVOKED,
                                               primary_result);

  auto verifier = std::make_unique<TrialComparisonCertVerifier>(
      profile(), verify_proc1, base::MakeRefCounted<NotCalledCertVerifyProc>());

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error = verifier->Verify(params, nullptr /* crl_set */, &result,
                               base::BindRepeating(&NotCalledCallback),
                               &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  // Delete the TrialComparisonCertVerifier.
  verifier.reset();

  // The callback to the main verifier does not run. The verification task
  // still completes in the background, but since the CertVerifier has been
  // deleted, the result is ignored.

  // Wait for any tasks to finish.
  content::RunAllTasksUntilIdle();

  // Expect no report.
  reporting_service_test_helper()->ExpectNoRequests(service());

  // Histograms should not be recorded.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 0);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 0);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               0);
  histograms_.ExpectTotalCount("Net.CertVerifier_TrialComparisonResult", 0);
}

TEST_F(TrialComparisonCertVerifierTest, DeletedDuringTrialVerification) {
  // Primary verifier returns an error status.
  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  primary_result.cert_status = net::CERT_STATUS_REVOKED;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::ERR_CERT_REVOKED,
                                               primary_result);

  // Trial verifier has ok status.
  net::CertVerifyResult secondary_result;
  secondary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, secondary_result);

  auto verifier = std::make_unique<TrialComparisonCertVerifier>(
      profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier->Verify(params, nullptr /* crl_set */, &result,
                       callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  // Wait for primary verifier to finish.
  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::ERR_CERT_REVOKED));

  // Delete the TrialComparisonCertVerifier.
  verifier.reset();

  // The callback to the trial verifier does not run. The verification task
  // still completes in the background, but since the CertVerifier has been
  // deleted, the result is ignored.

  // Wait for any tasks to finish.
  content::RunAllTasksUntilIdle();

  // Expect no report.
  reporting_service_test_helper()->ExpectNoRequests(service());

  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  // Histograms for trial verifier should not be recorded.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               0);
  histograms_.ExpectTotalCount("Net.CertVerifier_TrialComparisonResult", 0);
}

TEST_F(TrialComparisonCertVerifierTest,
       PrimaryVerifierOkSecondaryErrorUmaOnly) {
  // Enable feature with uma_only flag.
  scoped_feature_.reset();
  scoped_feature_ = std::make_unique<base::test::ScopedFeatureList>();
  scoped_feature_->InitAndEnableFeatureWithParameters(
      features::kCertDualVerificationTrialFeature, {{"uma_only", "true"}});

  net::CertVerifyResult primary_result;
  primary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc1 =
      base::MakeRefCounted<MockCertVerifyProc>(net::OK, primary_result);

  // Trial verifier returns an error status.
  net::CertVerifyResult secondary_result;
  secondary_result.cert_status = net::CERT_STATUS_REVOKED;
  secondary_result.verified_cert = cert_chain_1_;
  scoped_refptr<MockCertVerifyProc> verify_proc2 =
      base::MakeRefCounted<MockCertVerifyProc>(net::ERR_CERT_REVOKED,
                                               secondary_result);

  TrialComparisonCertVerifier verifier(profile(), verify_proc1, verify_proc2);

  net::CertVerifier::RequestParams params(
      leaf_cert_1_, "127.0.0.1", 0 /* flags */,
      std::string() /* ocsp_response */, {} /* additional_trust_anchors */);
  net::CertVerifyResult result;
  net::TestCompletionCallback callback;
  std::unique_ptr<net::CertVerifier::Request> request;
  int error =
      verifier.Verify(params, nullptr /* crl_set */, &result,
                      callback.callback(), &request, net::NetLogWithSource());
  ASSERT_THAT(error, IsError(net::ERR_IO_PENDING));
  EXPECT_TRUE(request);

  error = callback.WaitForResult();
  EXPECT_THAT(error, IsError(net::OK));

  verify_proc2->WaitForVerifyCall();

  // Wait for any tasks to finish.
  content::RunAllTasksUntilIdle();

  // Expect no report.
  reporting_service_test_helper()->ExpectNoRequests(service());

  // Should still have UMA logs.
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialPrimary", 1);
  histograms_.ExpectTotalCount("Net.CertVerifier_Job_Latency_TrialSecondary",
                               1);
  histograms_.ExpectUniqueSample(
      "Net.CertVerifier_TrialComparisonResult",
      TrialComparisonCertVerifier::kPrimaryValidSecondaryError, 1);
}
