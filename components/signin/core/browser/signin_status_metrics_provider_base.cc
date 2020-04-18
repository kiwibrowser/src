// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/signin_status_metrics_provider_base.h"

#include "base/metrics/histogram_macros.h"

SigninStatusMetricsProviderBase::SigninStatusMetricsProviderBase()
    : signin_status_(UNKNOWN_SIGNIN_STATUS) {}

SigninStatusMetricsProviderBase::~SigninStatusMetricsProviderBase() {}

void SigninStatusMetricsProviderBase::RecordSigninStatusHistogram(
    SigninStatus signin_status) {
  UMA_HISTOGRAM_ENUMERATION("UMA.ProfileSignInStatus", signin_status,
                            SIGNIN_STATUS_MAX);
}

void SigninStatusMetricsProviderBase::UpdateSigninStatus(
    SigninStatus new_status) {
  // The recorded sign-in status value can't be changed once it's recorded as
  // error until the next UMA upload.
  if (signin_status_ == ERROR_GETTING_SIGNIN_STATUS)
    return;
  signin_status_ = new_status;
}

void SigninStatusMetricsProviderBase::ResetSigninStatus() {
  signin_status_ = UNKNOWN_SIGNIN_STATUS;
}
