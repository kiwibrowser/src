// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TRANSLATE_TRANSLATE_RANKER_METRICS_PROVIDER_H_
#define CHROME_BROWSER_TRANSLATE_TRANSLATE_RANKER_METRICS_PROVIDER_H_

#include "components/metrics/metrics_provider.h"

namespace translate {

// Provides metrics related to the translate ranker.
class TranslateRankerMetricsProvider : public metrics::MetricsProvider {
 public:
  TranslateRankerMetricsProvider() : logging_enabled_(false) {}

  ~TranslateRankerMetricsProvider() override {}

  // From metrics::MetricsProvider...
  void ProvideCurrentSessionData(
      metrics::ChromeUserMetricsExtension* uma_proto) override;
  void OnRecordingEnabled() override;
  void OnRecordingDisabled() override;

 private:
  // Updates the logging state of all ranker instances.
  void UpdateLoggingState();

  // The current state of logging.
  bool logging_enabled_;

  DISALLOW_COPY_AND_ASSIGN(TranslateRankerMetricsProvider);
};

}  // namespace translate

#endif  // CHROME_BROWSER_TRANSLATE_TRANSLATE_RANKER_METRICS_PROVIDER_H_
