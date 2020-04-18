// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_RESOURCE_SCHEDULER_PARAMS_MANAGER_H_
#define SERVICES_NETWORK_RESOURCE_SCHEDULER_PARAMS_MANAGER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>

#include "base/component_export.h"
#include "base/sequence_checker.h"
#include "net/nqe/effective_connection_type.h"

namespace network {

// Parses and stores the resource scheduler parameters based on the set of
// enabled experiments and/or estimated network quality.
class COMPONENT_EXPORT(NETWORK_SERVICE) ResourceSchedulerParamsManager {
 public:
  // A struct that stores the resource scheduler parameters that vary with
  // network quality.
  struct COMPONENT_EXPORT(NETWORK_SERVICE) ParamsForNetworkQuality {
    ParamsForNetworkQuality();

    ParamsForNetworkQuality(size_t max_delayable_requests,
                            double non_delayable_weight,
                            bool delay_requests_on_multiplexed_connections);

    // The maximum number of delayable requests allowed.
    size_t max_delayable_requests;

    // The weight of a non-delayable request when counting the effective number
    // of non-delayable requests in-flight.
    double non_delayable_weight;

    // True if requests to servers that support prioritization (e.g.,
    // H2/SPDY/QUIC) should be delayed similar to other HTTP 1.1 requests.
    bool delay_requests_on_multiplexed_connections;
  };

  ResourceSchedulerParamsManager();
  ResourceSchedulerParamsManager(const ResourceSchedulerParamsManager& other);

  // Mapping from the observed Effective Connection Type (ECT) to
  // ParamsForNetworkQuality.
  typedef std::map<net::EffectiveConnectionType, ParamsForNetworkQuality>
      ParamsForNetworkQualityContainer;

  // Constructor to be used when ParamsForNetworkQualityContainer need to be
  // overwritten.
  explicit ResourceSchedulerParamsManager(
      const ParamsForNetworkQualityContainer&
          params_for_network_quality_container);

  ~ResourceSchedulerParamsManager();

  // Returns the parameters for resource loading based on
  // |effective_connection_type|. Virtual for testing.
  ParamsForNetworkQuality GetParamsForEffectiveConnectionType(
      net::EffectiveConnectionType effective_connection_type) const;

 private:
  // Reads the experiments params for DelayRequestsOnMultiplexedConnections
  // finch experiment, modifies |result| based on the experiment params, and
  // returns the modified |result|.
  static ParamsForNetworkQualityContainer
  GetParamsForDelayRequestsOnMultiplexedConnections(
      ParamsForNetworkQualityContainer result);

  // Reads experiment parameters and populates
  // |params_for_network_quality_container_|. It looks for configuration
  // parameters with sequential numeric suffixes, and stops looking after the
  // first failure to find an experimetal parameter. A sample configuration is
  // given below:
  // "EffectiveConnectionType1": "Slow-2G",
  // "MaxDelayableRequests1": "6",
  // "NonDelayableWeight1": "2.0",
  // "EffectiveConnectionType2": "3G",
  // "MaxDelayableRequests2": "12",
  // "NonDelayableWeight2": "3.0",
  // This config implies that when Effective Connection Type (ECT) is Slow-2G,
  // then the maximum number of non-delayable requests should be
  // limited to 6, and the non-delayable request weight should be set to 2.
  // When ECT is 3G, it should be limited to 12. For all other values of ECT,
  // the default values are used.
  static ParamsForNetworkQualityContainer GetParamsForNetworkQualityContainer();

  // The number of delayable requests in-flight for different ranges of the
  // network quality.
  const ParamsForNetworkQualityContainer params_for_network_quality_container_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace network

#endif  // SERVICES_NETWORK_RESOURCE_SCHEDULER_PARAMS_MANAGER_H_
