// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_TRIAL_COMPARISON_CERT_VERIFIER_H_
#define CHROME_BROWSER_NET_TRIAL_COMPARISON_CERT_VERIFIER_H_

#include "base/containers/unique_ptr_adapters.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "net/base/net_export.h"
#include "net/cert/cert_verifier.h"

namespace net {
class CertVerifyProc;
}

class TrialComparisonCertVerifier : public net::CertVerifier {
 public:
  // These values are persisted to logs. Entries should not be renumbered and
  // numeric values should never be reused.
  enum TrialComparisonResult {
    kInvalid = 0,
    kEqual = 1,
    kPrimaryValidSecondaryError = 2,
    kPrimaryErrorSecondaryValid = 3,
    kBothValidDifferentDetails = 4,
    kBothErrorDifferentDetails = 5,
    kMaxValue = kBothErrorDifferentDetails
  };

  TrialComparisonCertVerifier(
      void* profile_id,
      scoped_refptr<net::CertVerifyProc> primary_verify_proc,
      scoped_refptr<net::CertVerifyProc> trial_verify_proc);

  ~TrialComparisonCertVerifier() override;

  // This method can be called by tests to fake an official build (reports are
  // only sent from official builds).
  static void SetFakeOfficialBuildForTesting();

  // CertVerifier implementation
  int Verify(const RequestParams& params,
             net::CRLSet* crl_set,
             net::CertVerifyResult* verify_result,
             const net::CompletionCallback& callback,
             std::unique_ptr<Request>* out_req,
             const net::NetLogWithSource& net_log) override;

  bool SupportsOCSPStapling() override;

 private:
  class TrialVerificationJob;

  void OnPrimaryVerifierComplete(const RequestParams& params,
                                 scoped_refptr<net::CRLSet> crl_set,
                                 const net::NetLogWithSource& net_log,
                                 int primary_error,
                                 const net::CertVerifyResult& primary_result,
                                 base::TimeDelta latency,
                                 bool is_first_job);
  void OnTrialVerifierComplete(const RequestParams& params,
                               scoped_refptr<net::CRLSet> crl_set,
                               const net::NetLogWithSource& net_log,
                               int trial_error,
                               const net::CertVerifyResult& trial_result,
                               base::TimeDelta latency,
                               bool is_first_job);
  void MaybeDoTrialVerification(const RequestParams& params,
                                scoped_refptr<net::CRLSet> crl_set,
                                const net::NetLogWithSource& net_log,
                                int primary_error,
                                const net::CertVerifyResult& primary_result,
                                void* profile_id,
                                bool trial_allowed);

  void RemoveJob(TrialVerificationJob* job_ptr);

  // The profile this verifier is associated with. Stored as a void* to avoid
  // accidentally using it on IO thread.
  void* profile_id_;

  std::unique_ptr<net::CertVerifier> primary_verifier_;
  std::unique_ptr<net::CertVerifier> trial_verifier_;

  std::set<std::unique_ptr<TrialVerificationJob>, base::UniquePtrComparator>
      jobs_;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<TrialComparisonCertVerifier> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrialComparisonCertVerifier);
};

#endif  // CHROME_BROWSER_NET_TRIAL_COMPARISON_CERT_VERIFIER_H_
