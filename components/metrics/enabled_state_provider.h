// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_ENABLED_STATE_PROVIDER_H_
#define COMPONENTS_METRICS_ENABLED_STATE_PROVIDER_H_

namespace metrics {

// An interface that provides whether metrics should be reported.
class EnabledStateProvider {
 public:
  virtual ~EnabledStateProvider() {}

  // Indicates that the user has provided consent to collect and report metrics.
  virtual bool IsConsentGiven() const = 0;

  // Should collection and reporting be enabled. This should depend on consent
  // being given.
  virtual bool IsReportingEnabled() const;
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_ENABLED_STATE_PROVIDER_H_
