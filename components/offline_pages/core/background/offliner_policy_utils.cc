// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/offliner_policy_utils.h"

#include "components/offline_pages/core/background/offliner_policy.h"
#include "components/offline_pages/core/background/save_page_request.h"

namespace offline_pages {

// static function to check request expiration or cleanup status.
OfflinerPolicyUtils::RequestExpirationStatus
OfflinerPolicyUtils::CheckRequestExpirationStatus(
    const SavePageRequest* request,
    const OfflinerPolicy* policy) {
  DCHECK(request);
  DCHECK(policy);

  if (base::Time::Now() - request->creation_time() >=
      base::TimeDelta::FromSeconds(
          policy->GetRequestExpirationTimeInSeconds())) {
    return RequestExpirationStatus::EXPIRED;
  }
  if (request->started_attempt_count() >= policy->GetMaxStartedTries())
    return RequestExpirationStatus::START_COUNT_EXCEEDED;

  if (request->completed_attempt_count() >= policy->GetMaxCompletedTries())
    return RequestExpirationStatus::COMPLETION_COUNT_EXCEEDED;

  return RequestExpirationStatus::VALID;
}

}  // namespace offline_pages
