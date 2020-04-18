// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/trial_comparison_cert_verifier.h"

#include <memory>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/location.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service_factory.h"
#include "chrome/browser/ssl/certificate_error_report.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_features.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verify_proc.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/crl_set.h"
#include "net/cert/multi_threaded_cert_verifier.h"
#include "net/cert/x509_util.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"

// Certificate reports are only sent from official builds, but this flag can be
// set by tests.
static bool g_is_fake_official_build_for_cert_verifier_testing = false;

namespace {

bool CheckTrialEligibility(void* profile_id,
                           base::TimeDelta primary_latency,
                           bool is_first_job) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // g_browser_process is valid until after all threads are stopped. So it must
  // be valid if the CheckTrialEligibility task got to run.
  if (!g_browser_process->profile_manager()->IsValidProfile(profile_id))
    return false;
  const Profile* profile = reinterpret_cast<const Profile*>(profile_id);
  const PrefService& prefs = *profile->GetPrefs();

  // Only allow on non-incognito profiles which have SBER2 (Scout) opt-in set.
  // See design doc for more details:
  // https://docs.google.com/document/d/1AM1CD42bC6LHWjKg-Hkid_RLr2DH6OMzstH9-pGSi-g
  bool allowed = !profile->IsOffTheRecord() && safe_browsing::IsScout(prefs) &&
                 safe_browsing::IsExtendedReportingEnabled(prefs);

  if (allowed) {
    // Only record the TrialPrimary histograms for the same set of requests
    // that TrialSecondary histograms will be recorded for, in order to get a
    // direct comparison.
    UMA_HISTOGRAM_CUSTOM_TIMES("Net.CertVerifier_Job_Latency_TrialPrimary",
                               primary_latency,
                               base::TimeDelta::FromMilliseconds(1),
                               base::TimeDelta::FromMinutes(10), 100);
    if (is_first_job) {
      UMA_HISTOGRAM_CUSTOM_TIMES(
          "Net.CertVerifier_First_Job_Latency_TrialPrimary", primary_latency,
          base::TimeDelta::FromMilliseconds(1),
          base::TimeDelta::FromMinutes(10), 100);
    }
  }

  return allowed;
}

void SendTrialVerificationReport(void* profile_id,
                                 const net::CertVerifier::RequestParams& params,
                                 const net::CertVerifyResult& primary_result,
                                 const net::CertVerifyResult& trial_result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!g_browser_process->profile_manager()->IsValidProfile(profile_id))
    return;
  Profile* profile = reinterpret_cast<Profile*>(profile_id);

  CertificateErrorReport report(params.hostname(), *params.certificate(),
                                params.flags(), primary_result, trial_result);

  report.AddNetworkTimeInfo(g_browser_process->network_time_tracker());
  report.AddChromeChannel(chrome::GetChannel());

  std::string serialized_report;
  if (!report.Serialize(&serialized_report))
    return;

  CertificateReportingServiceFactory::GetForBrowserContext(profile)->Send(
      serialized_report);
}

std::unique_ptr<base::Value> TrialVerificationJobResultCallback(
    bool trial_success,
    net::NetLogCaptureMode capture_mode) {
  std::unique_ptr<base::DictionaryValue> results(new base::DictionaryValue());
  results->SetKey("trial_success", base::Value(trial_success));
  return std::move(results);
}

bool CertVerifyResultEqual(const net::CertVerifyResult& a,
                           const net::CertVerifyResult& b) {
  return std::tie(a.cert_status, a.is_issued_by_known_root) ==
             std::tie(b.cert_status, b.is_issued_by_known_root) &&
         (!!a.verified_cert == !!b.verified_cert) &&
         (!a.verified_cert ||
          a.verified_cert->EqualsIncludingChain(b.verified_cert.get()));
}

}  // namespace

class TrialComparisonCertVerifier::TrialVerificationJob {
 public:
  TrialVerificationJob(const net::CertVerifier::RequestParams& params,
                       const net::NetLogWithSource& source_net_log,
                       TrialComparisonCertVerifier* cert_verifier,
                       int primary_error,
                       const net::CertVerifyResult& primary_result,
                       void* profile_id)
      : params_(params),
        net_log_(net::NetLogWithSource::Make(
            source_net_log.net_log(),
            net::NetLogSourceType::TRIAL_CERT_VERIFIER_JOB)),
        cert_verifier_(cert_verifier),
        primary_error_(primary_error),
        primary_result_(primary_result),
        profile_id_(profile_id) {
    net_log_.BeginEvent(net::NetLogEventType::TRIAL_CERT_VERIFIER_JOB);
    source_net_log.AddEvent(
        net::NetLogEventType::TRIAL_CERT_VERIFIER_JOB_COMPARISON_STARTED,
        net_log_.source().ToEventParametersCallback());
  }

  ~TrialVerificationJob() {
    if (cert_verifier_) {
      net_log_.AddEvent(net::NetLogEventType::CANCELLED);
      net_log_.EndEvent(net::NetLogEventType::TRIAL_CERT_VERIFIER_JOB);
    }
  }

  int Start(net::CertVerifier* verifier, scoped_refptr<net::CRLSet> crl_set) {
    // Unretained is safe because trial_request_ will cancel the callback on
    // destruction.
    int rv = verifier->Verify(
        params_, crl_set.get(), &trial_result_,
        base::AdaptCallbackForRepeating(base::BindOnce(
            &TrialVerificationJob::OnJobCompleted, base::Unretained(this))),
        &trial_request_, net_log_);
    if (rv != net::ERR_IO_PENDING)
      CompareTrialResults(rv);
    return rv;
  }

  void CompareTrialResults(int trial_result_error) {
    cert_verifier_ = nullptr;
    bool errors_equal = trial_result_error == primary_error_;
    bool details_equal = CertVerifyResultEqual(trial_result_, primary_result_);
    bool trial_success = errors_equal && details_equal;

    net_log_.EndEvent(net::NetLogEventType::TRIAL_CERT_VERIFIER_JOB,
                      base::BindRepeating(&TrialVerificationJobResultCallback,
                                          trial_success));

    TrialComparisonResult result_code = kInvalid;
    if (trial_success) {
      result_code = kEqual;
    } else if (errors_equal) {
      if (primary_error_ == net::OK)
        result_code = kBothValidDifferentDetails;
      else
        result_code = kBothErrorDifferentDetails;
    } else if (primary_error_ == net::OK) {
      result_code = kPrimaryValidSecondaryError;
    } else {
      result_code = kPrimaryErrorSecondaryValid;
    }
    UMA_HISTOGRAM_ENUMERATION("Net.CertVerifier_TrialComparisonResult",
                              result_code);

    if (trial_success)
      return;

    if (base::GetFieldTrialParamByFeatureAsBool(
            features::kCertDualVerificationTrialFeature, "uma_only", false)) {
      return;
    }

    content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)
        ->PostTask(FROM_HERE,
                   base::BindOnce(&SendTrialVerificationReport, profile_id_,
                                  params_, primary_result_, trial_result_));
  }

  void OnJobCompleted(int trial_result_error) {
    // cert_verifier_ is cleared by CompareTrialResults, save a copy now.
    TrialComparisonCertVerifier* cert_verifier = cert_verifier_;

    CompareTrialResults(trial_result_error);

    // |this| is deleted after RemoveJob returns.
    cert_verifier->RemoveJob(this);
  }

 private:
  const net::CertVerifier::RequestParams params_;
  net::CertVerifyResult trial_result_;
  std::unique_ptr<net::CertVerifier::Request> trial_request_;

  const net::NetLogWithSource net_log_;
  TrialComparisonCertVerifier* cert_verifier_;  // Non-owned.

  // Saved results of the primary verification.
  int primary_error_;
  const net::CertVerifyResult primary_result_;

  void* profile_id_;

  DISALLOW_COPY_AND_ASSIGN(TrialVerificationJob);
};

TrialComparisonCertVerifier::TrialComparisonCertVerifier(
    void* profile_id,
    scoped_refptr<net::CertVerifyProc> primary_verify_proc,
    scoped_refptr<net::CertVerifyProc> trial_verify_proc)
    : profile_id_(profile_id),
      primary_verifier_(
          net::MultiThreadedCertVerifier::CreateForDualVerificationTrial(
              primary_verify_proc,
              // Unretained is safe since the callback won't be called after
              // |primary_verifier_| is destroyed.
              base::BindRepeating(
                  &TrialComparisonCertVerifier::OnPrimaryVerifierComplete,
                  base::Unretained(this)),
              true /* should_record_histograms */)),
      trial_verifier_(
          net::MultiThreadedCertVerifier::CreateForDualVerificationTrial(
              trial_verify_proc,
              // Unretained is safe since the callback won't be called after
              // |trial_verifier_| is destroyed.
              base::BindRepeating(
                  &TrialComparisonCertVerifier::OnTrialVerifierComplete,
                  base::Unretained(this)),
              false /* should_record_histograms */)),
      weak_ptr_factory_(this) {}

TrialComparisonCertVerifier::~TrialComparisonCertVerifier() = default;

// static
void TrialComparisonCertVerifier::SetFakeOfficialBuildForTesting() {
  g_is_fake_official_build_for_cert_verifier_testing = true;
}

int TrialComparisonCertVerifier::Verify(const RequestParams& params,
                                        net::CRLSet* crl_set,
                                        net::CertVerifyResult* verify_result,
                                        const net::CompletionCallback& callback,
                                        std::unique_ptr<Request>* out_req,
                                        const net::NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  return primary_verifier_->Verify(params, crl_set, verify_result, callback,
                                   out_req, net_log);
}

bool TrialComparisonCertVerifier::SupportsOCSPStapling() {
  return primary_verifier_->SupportsOCSPStapling();
}

void TrialComparisonCertVerifier::OnPrimaryVerifierComplete(
    const RequestParams& params,
    scoped_refptr<net::CRLSet> crl_set,
    const net::NetLogWithSource& net_log,
    int primary_error,
    const net::CertVerifyResult& primary_result,
    base::TimeDelta primary_latency,
    bool is_first_job) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  bool is_official_build = g_is_fake_official_build_for_cert_verifier_testing;
#if defined(OFFICIAL_BUILD) && defined(GOOGLE_CHROME_BUILD)
  is_official_build = true;
#endif
  if (!is_official_build || !base::FeatureList::IsEnabled(
                                features::kCertDualVerificationTrialFeature)) {
    return;
  }

  base::PostTaskAndReplyWithResult(
      content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)
          .get(),
      FROM_HERE,
      base::BindOnce(CheckTrialEligibility, profile_id_, primary_latency,
                     is_first_job),
      base::BindOnce(&TrialComparisonCertVerifier::MaybeDoTrialVerification,
                     weak_ptr_factory_.GetWeakPtr(), params, std::move(crl_set),
                     net_log, primary_error, primary_result, profile_id_));
}

void TrialComparisonCertVerifier::OnTrialVerifierComplete(
    const RequestParams& params,
    scoped_refptr<net::CRLSet> crl_set,
    const net::NetLogWithSource& net_log,
    int trial_error,
    const net::CertVerifyResult& trial_result,
    base::TimeDelta latency,
    bool is_first_job) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  UMA_HISTOGRAM_CUSTOM_TIMES("Net.CertVerifier_Job_Latency_TrialSecondary",
                             latency, base::TimeDelta::FromMilliseconds(1),
                             base::TimeDelta::FromMinutes(10), 100);
  if (is_first_job) {
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "Net.CertVerifier_First_Job_Latency_TrialSecondary", latency,
        base::TimeDelta::FromMilliseconds(1), base::TimeDelta::FromMinutes(10),
        100);
  }
}

void TrialComparisonCertVerifier::MaybeDoTrialVerification(
    const RequestParams& params,
    scoped_refptr<net::CRLSet> crl_set,
    const net::NetLogWithSource& net_log,
    int primary_error,
    const net::CertVerifyResult& primary_result,
    void* profile_id,
    bool trial_allowed) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!trial_allowed)
    return;

  std::unique_ptr<TrialVerificationJob> job =
      std::make_unique<TrialVerificationJob>(
          params, net_log, this, primary_error, primary_result, profile_id);

  if (job->Start(trial_verifier_.get(), std::move(crl_set)) ==
      net::ERR_IO_PENDING) {
    jobs_.insert(std::move(job));
  }
}

void TrialComparisonCertVerifier::RemoveJob(TrialVerificationJob* job_ptr) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto it = jobs_.find(job_ptr);
  DCHECK(it != jobs_.end());
  jobs_.erase(it);
}
