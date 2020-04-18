// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_METRICS_METRICS_HELPER_H_
#define CHROME_BROWSER_VR_METRICS_METRICS_HELPER_H_

#include <string>

#include "base/optional.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "chrome/browser/vr/assets_component_update_status.h"
#include "chrome/browser/vr/assets_load_status.h"
#include "chrome/browser/vr/mode.h"

namespace base {
class Version;
}  // namespace base

namespace vr {

// Helper to collect VR UMA metrics.
//
// For thread-safety, all functions must be called in sequence.
class MetricsHelper {
 public:
  MetricsHelper();
  ~MetricsHelper();

  void OnComponentReady(const base::Version& version);
  void OnEnter(Mode mode);
  void OnRegisteredComponent();
  void OnComponentUpdated(AssetsComponentUpdateStatus status,
                          const base::Optional<base::Version>& version);
  void OnAssetsLoaded(AssetsLoadStatus status,
                      const base::Version& component_version);

 private:
  base::Optional<base::TimeTicks>& GetEnterTime(Mode mode);
  void LogLatencyIfWaited(Mode mode, const base::TimeTicks& now);

  base::Optional<base::TimeTicks> enter_vr_time_;
  base::Optional<base::TimeTicks> enter_vr_browsing_time_;
  base::Optional<base::TimeTicks> enter_web_vr_time_;
  base::Optional<base::TimeTicks> component_register_time_;
  bool logged_ready_duration_on_component_register_ = false;
  bool component_ready_ = false;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_METRICS_METRICS_HELPER_H_
