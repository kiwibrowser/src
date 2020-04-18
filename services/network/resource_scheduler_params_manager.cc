// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/resource_scheduler_params_manager.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/optional.h"
#include "base/strings/string_number_conversions.h"
#include "net/nqe/network_quality_estimator.h"
#include "services/network/public/cpp/features.h"

namespace {

// The maximum number of delayable requests to allow to be in-flight at any
// point in time (across all hosts).
static const size_t kDefaultMaxNumDelayableRequestsPerClient = 10;

}  // namespace

namespace network {

ResourceSchedulerParamsManager::ParamsForNetworkQuality::
    ParamsForNetworkQuality()
    : ResourceSchedulerParamsManager::ParamsForNetworkQuality(
          kDefaultMaxNumDelayableRequestsPerClient,
          0.0,
          false) {}

ResourceSchedulerParamsManager::ParamsForNetworkQuality::
    ParamsForNetworkQuality(size_t max_delayable_requests,
                            double non_delayable_weight,
                            bool delay_requests_on_multiplexed_connections)
    : max_delayable_requests(max_delayable_requests),
      non_delayable_weight(non_delayable_weight),
      delay_requests_on_multiplexed_connections(
          delay_requests_on_multiplexed_connections) {}

ResourceSchedulerParamsManager::ResourceSchedulerParamsManager()
    : ResourceSchedulerParamsManager(
          GetParamsForDelayRequestsOnMultiplexedConnections(
              GetParamsForNetworkQualityContainer())) {}

ResourceSchedulerParamsManager::ResourceSchedulerParamsManager(
    const ParamsForNetworkQualityContainer&
        params_for_network_quality_container)
    : params_for_network_quality_container_(
          params_for_network_quality_container) {}

ResourceSchedulerParamsManager::ResourceSchedulerParamsManager(
    const ResourceSchedulerParamsManager& other)
    : params_for_network_quality_container_(
          other.params_for_network_quality_container_) {}

ResourceSchedulerParamsManager::~ResourceSchedulerParamsManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

ResourceSchedulerParamsManager::ParamsForNetworkQuality
ResourceSchedulerParamsManager::GetParamsForEffectiveConnectionType(
    net::EffectiveConnectionType effective_connection_type) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ParamsForNetworkQualityContainer::const_iterator iter =
      params_for_network_quality_container_.find(effective_connection_type);
  if (iter != params_for_network_quality_container_.end())
    return iter->second;
  return ParamsForNetworkQuality(kDefaultMaxNumDelayableRequestsPerClient, 0.0,
                                 false);
}

// static
ResourceSchedulerParamsManager::ParamsForNetworkQualityContainer
ResourceSchedulerParamsManager::
    GetParamsForDelayRequestsOnMultiplexedConnections(
        ResourceSchedulerParamsManager::ParamsForNetworkQualityContainer
            result) {
  if (!base::FeatureList::IsEnabled(
          features::kDelayRequestsOnMultiplexedConnections)) {
    return result;
  }

  base::Optional<net::EffectiveConnectionType> max_effective_connection_type =
      net::GetEffectiveConnectionTypeForName(
          base::GetFieldTrialParamValueByFeature(
              features::kDelayRequestsOnMultiplexedConnections,
              "MaxEffectiveConnectionType"));

  if (!max_effective_connection_type)
    return result;

  for (int ect = net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G;
       ect <= max_effective_connection_type.value(); ++ect) {
    net::EffectiveConnectionType effective_connection_type =
        static_cast<net::EffectiveConnectionType>(ect);
    ParamsForNetworkQualityContainer::iterator iter =
        result.find(effective_connection_type);
    if (iter != result.end()) {
      iter->second.delay_requests_on_multiplexed_connections = true;
    } else {
      result.emplace(std::make_pair(
          effective_connection_type,
          ParamsForNetworkQuality(kDefaultMaxNumDelayableRequestsPerClient, 0.0,
                                  true)));
    }
  }
  return result;
}

// static
ResourceSchedulerParamsManager::ParamsForNetworkQualityContainer
ResourceSchedulerParamsManager::GetParamsForNetworkQualityContainer() {
  static const char kMaxDelayableRequestsBase[] = "MaxDelayableRequests";
  static const char kEffectiveConnectionTypeBase[] = "EffectiveConnectionType";
  static const char kNonDelayableWeightBase[] = "NonDelayableWeight";

  ResourceSchedulerParamsManager::ParamsForNetworkQualityContainer result;
  // Set the default params for networks with ECT Slow2G and 2G. These params
  // can still be overridden using the field trial.
  result.emplace(std::make_pair(net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G,
                                ParamsForNetworkQuality(8, 3.0, false)));
  result.emplace(std::make_pair(net::EFFECTIVE_CONNECTION_TYPE_2G,
                                ParamsForNetworkQuality(8, 3.0, false)));

  for (int config_param_index = 1; config_param_index <= 20;
       ++config_param_index) {
    size_t max_delayable_requests;

    if (!base::StringToSizeT(base::GetFieldTrialParamValueByFeature(
                                 features::kThrottleDelayable,
                                 kMaxDelayableRequestsBase +
                                     base::IntToString(config_param_index)),
                             &max_delayable_requests)) {
      return result;
    }

    base::Optional<net::EffectiveConnectionType> effective_connection_type =
        net::GetEffectiveConnectionTypeForName(
            base::GetFieldTrialParamValueByFeature(
                features::kThrottleDelayable,
                kEffectiveConnectionTypeBase +
                    base::IntToString(config_param_index)));
    DCHECK(effective_connection_type.has_value());

    double non_delayable_weight = base::GetFieldTrialParamByFeatureAsDouble(
        features::kThrottleDelayable,
        kNonDelayableWeightBase + base::IntToString(config_param_index), 0.0);

    // Check if the entry is already present. This will happen if the default
    // params are being overridden by the field trial.
    ParamsForNetworkQualityContainer::iterator iter =
        result.find(effective_connection_type.value());
    if (iter != result.end()) {
      iter->second.max_delayable_requests = max_delayable_requests;
      iter->second.non_delayable_weight = non_delayable_weight;
    } else {
      result.emplace(
          std::make_pair(effective_connection_type.value(),
                         ParamsForNetworkQuality(max_delayable_requests,
                                                 non_delayable_weight, false)));
    }
  }
  // There should not have been more than 20 params indices specified.
  NOTREACHED();
  return result;
}

}  // namespace network
