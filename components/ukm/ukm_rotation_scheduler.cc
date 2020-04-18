// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/ukm_rotation_scheduler.h"

#include "base/metrics/histogram_macros.h"

namespace ukm {

UkmRotationScheduler::UkmRotationScheduler(
    const base::Closure& upload_callback,
    const base::Callback<base::TimeDelta(void)>& upload_interval_callback)
    : metrics::MetricsRotationScheduler(upload_callback,
                                        upload_interval_callback) {}

UkmRotationScheduler::~UkmRotationScheduler() = default;

void UkmRotationScheduler::LogMetricsInitSequence(InitSequence sequence) {
  UMA_HISTOGRAM_ENUMERATION("UKM.InitSequence", sequence,
                            INIT_SEQUENCE_ENUM_SIZE);
}

}  // namespace ukm
