// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/async_scripts_run_info.h"

#include "base/metrics/histogram_macros.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/renderer/scripts_run_info.h"

namespace extensions {

AsyncScriptsRunInfo::AsyncScriptsRunInfo(UserScript::RunLocation location)
    : run_location_(location) {}

AsyncScriptsRunInfo::~AsyncScriptsRunInfo() {}

void AsyncScriptsRunInfo::WillExecute(const base::TimeTicks& timestamp) {
  if (!last_completed_time_.is_null()) {
    switch (run_location_) {
      case UserScript::DOCUMENT_END:
        UMA_HISTOGRAM_TIMES(
            "Extensions.TimeYieldedBetweenContentScriptRuns.DocumentEnd",
            timestamp - last_completed_time_);
        break;
      case UserScript::DOCUMENT_IDLE:
        UMA_HISTOGRAM_TIMES(
            "Extensions.TimeYieldedBetweenContentScriptRuns.DocumentIdle",
            timestamp - last_completed_time_);
        break;
      // Currently document_start scripts are not async.
      default:
        break;
    }
  }
}

void AsyncScriptsRunInfo::OnCompleted(const base::TimeTicks& timestamp,
                                      base::Optional<base::TimeDelta> elapsed) {
  last_completed_time_ = timestamp;
  if (elapsed) {
    ScriptsRunInfo::LogLongInjectionTaskTime(run_location_, *elapsed);
  }
}

}  // namespace extensions
