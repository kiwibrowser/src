// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gcm/engine/gcm_registration_request_handler.h"

#include "base/metrics/histogram_macros.h"
#include "google_apis/gcm/base/gcm_util.h"

namespace gcm {

namespace {

// Request constants.
const char kSenderKey[] = "sender";

}  // namespace

GCMRegistrationRequestHandler::GCMRegistrationRequestHandler(
    const std::string& senders)
    : senders_(senders) {
}

GCMRegistrationRequestHandler::~GCMRegistrationRequestHandler() {}

void GCMRegistrationRequestHandler::BuildRequestBody(std::string* body){
  BuildFormEncoding(kSenderKey, senders_, body);
}

void GCMRegistrationRequestHandler::ReportUMAs(
    RegistrationRequest::Status status,
    int retry_count,
    base::TimeDelta complete_time) {
  UMA_HISTOGRAM_ENUMERATION("GCM.RegistrationRequestStatus",
                            status,
                            RegistrationRequest::STATUS_COUNT);

  // Other UMAs are only reported when the request succeeds.
  if (status != RegistrationRequest::SUCCESS)
    return;

  UMA_HISTOGRAM_COUNTS("GCM.RegistrationRetryCount", retry_count);
  UMA_HISTOGRAM_TIMES("GCM.RegistrationCompleteTime", complete_time);
}

}  // namespace gcm
