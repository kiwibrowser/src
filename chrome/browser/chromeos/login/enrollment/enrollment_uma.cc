// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/enrollment/enrollment_uma.h"

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"

namespace {

const char* const kMetricEnrollment = "Enterprise.Enrollment";
const char* const kMetricEnrollmentForced = "Enterprise.EnrollmentForced";
const char* const kMetricEnrollmentRecovery = "Enterprise.EnrollmentRecovery";

}  // namespace

namespace chromeos {

void EnrollmentUMA(policy::MetricEnrollment sample,
                   policy::EnrollmentConfig::Mode mode) {
  switch (mode) {
    case policy::EnrollmentConfig::MODE_MANUAL:
    case policy::EnrollmentConfig::MODE_MANUAL_REENROLLMENT:
    case policy::EnrollmentConfig::MODE_LOCAL_ADVERTISED:
    case policy::EnrollmentConfig::MODE_SERVER_ADVERTISED:
    case policy::EnrollmentConfig::MODE_ATTESTATION:
      base::UmaHistogramSparse(kMetricEnrollment, sample);
      break;
    case policy::EnrollmentConfig::MODE_LOCAL_FORCED:
    case policy::EnrollmentConfig::MODE_SERVER_FORCED:
    case policy::EnrollmentConfig::MODE_ATTESTATION_LOCAL_FORCED:
    case policy::EnrollmentConfig::MODE_ATTESTATION_SERVER_FORCED:
    case policy::EnrollmentConfig::MODE_ATTESTATION_MANUAL_FALLBACK:
      base::UmaHistogramSparse(kMetricEnrollmentForced, sample);
      break;
    case policy::EnrollmentConfig::MODE_RECOVERY:
      base::UmaHistogramSparse(kMetricEnrollmentRecovery, sample);
      break;
    case policy::EnrollmentConfig::MODE_OFFLINE_DEMO:
      // MODE_OFFLINE_DEMO is currently NOTREACHED(), since the normal
      // enrollment flow (which invokes this function) shouldn't use this mode.
      // TODO(mukai, agawronska): decide what needs to be done here.
      // https://crbug.com/835904
      NOTREACHED();
      break;
    case policy::EnrollmentConfig::MODE_NONE:
      NOTREACHED();
      break;
  }
}

}  // namespace chromeos
