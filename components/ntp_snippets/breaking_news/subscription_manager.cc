// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/subscription_manager.h"

#include "base/metrics/field_trial_params.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/ntp_snippets_constants.h"

namespace ntp_snippets {

namespace {

// Variation parameter for chrome-push-subscription backend.
const char kPushSubscriptionBackendParam[] = "push_subscription_backend";

// Variation parameter for chrome-push-unsubscription backend.
const char kPushUnsubscriptionBackendParam[] = "push_unsubscription_backend";

}  // namespace

GURL GetPushUpdatesSubscriptionEndpoint(version_info::Channel channel) {
  std::string endpoint = base::GetFieldTrialParamValueByFeature(
      kBreakingNewsPushFeature, kPushSubscriptionBackendParam);
  if (!endpoint.empty()) {
    return GURL{endpoint};
  }

  switch (channel) {
    case version_info::Channel::STABLE:
    case version_info::Channel::BETA:
      return GURL{kPushUpdatesSubscriptionServer};

    case version_info::Channel::DEV:
    case version_info::Channel::CANARY:
    case version_info::Channel::UNKNOWN:
      return GURL{kPushUpdatesSubscriptionStagingServer};
  }
  NOTREACHED();
  return GURL{kPushUpdatesSubscriptionStagingServer};
}

GURL GetPushUpdatesUnsubscriptionEndpoint(version_info::Channel channel) {
  std::string endpoint = base::GetFieldTrialParamValueByFeature(
      kBreakingNewsPushFeature, kPushUnsubscriptionBackendParam);
  if (!endpoint.empty()) {
    return GURL{endpoint};
  }

  switch (channel) {
    case version_info::Channel::STABLE:
    case version_info::Channel::BETA:
      return GURL{kPushUpdatesUnsubscriptionServer};

    case version_info::Channel::DEV:
    case version_info::Channel::CANARY:
    case version_info::Channel::UNKNOWN:
      return GURL{kPushUpdatesUnsubscriptionStagingServer};
  }
  NOTREACHED();
  return GURL{kPushUpdatesUnsubscriptionStagingServer};
}

}  // namespace ntp_snippets
