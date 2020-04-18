// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_SERVICES_MANAGER_METRICS_SERVICES_MANAGER_CLIENT_H_
#define COMPONENTS_METRICS_SERVICES_MANAGER_METRICS_SERVICES_MANAGER_CLIENT_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/metrics/field_trial.h"

namespace metrics {
class MetricsServiceClient;
}

namespace network {
class SharedURLLoaderFactory;
}

namespace rappor {
class RapporServiceImpl;
}

namespace variations {
class VariationsService;
}

namespace metrics_services_manager {

// MetricsServicesManagerClient is an interface that allows
// MetricsServicesManager to interact with its embedder.
class MetricsServicesManagerClient {
 public:
  virtual ~MetricsServicesManagerClient() {}

  // Methods that create the various services in the context of the embedder.
  virtual std::unique_ptr<rappor::RapporServiceImpl>
  CreateRapporServiceImpl() = 0;
  virtual std::unique_ptr<variations::VariationsService>
  CreateVariationsService() = 0;
  virtual std::unique_ptr<metrics::MetricsServiceClient>
  CreateMetricsServiceClient() = 0;
  virtual std::unique_ptr<const base::FieldTrial::EntropyProvider>
  CreateEntropyProvider() = 0;

  // Returns the URL loader factory which the metrics services should use.
  virtual scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactory() = 0;

  // Returns whether metrics reporting is enabled.
  virtual bool IsMetricsReportingEnabled() = 0;

  // Returns whether metrics consent is given.
  virtual bool IsMetricsConsentGiven() = 0;

  // Returns whether there are any Incognito browsers/tabs open.
  virtual bool IsIncognitoSessionActive() = 0;

  // Update the running state of metrics services managed by the embedder, for
  // example, crash reporting.
  virtual void UpdateRunningServices(bool may_record, bool may_upload) {}

  // If the user has forced metrics collection on via the override flag.
  virtual bool IsMetricsReportingForceEnabled();
};

}  // namespace metrics_services_manager

#endif  // COMPONENTS_METRICS_SERVICES_MANAGER_METRICS_SERVICES_MANAGER_CLIENT_H_
