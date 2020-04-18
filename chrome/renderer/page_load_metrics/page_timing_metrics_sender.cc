// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/page_load_metrics/page_timing_metrics_sender.h"

#include <utility>

#include "base/callback.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/common/page_load_metrics/page_load_metrics_constants.h"
#include "chrome/renderer/page_load_metrics/page_timing_sender.h"

namespace page_load_metrics {

namespace {
const int kInitialTimerDelayMillis = 50;
const base::Feature kPageLoadMetricsTimerDelayFeature{
    "PageLoadMetricsTimerDelay", base::FEATURE_DISABLED_BY_DEFAULT};
}  // namespace

PageTimingMetricsSender::PageTimingMetricsSender(
    std::unique_ptr<PageTimingSender> sender,
    std::unique_ptr<base::Timer> timer,
    mojom::PageLoadTimingPtr initial_timing)
    : sender_(std::move(sender)),
      timer_(std::move(timer)),
      last_timing_(std::move(initial_timing)),
      metadata_(mojom::PageLoadMetadata::New()),
      new_features_(mojom::PageLoadFeatures::New()),
      buffer_timer_delay_ms_(kBufferTimerDelayMillis) {
  buffer_timer_delay_ms_ = base::GetFieldTrialParamByFeatureAsInt(
      kPageLoadMetricsTimerDelayFeature, "BufferTimerDelayMillis",
      kBufferTimerDelayMillis /* default value */);
  if (!IsEmpty(*last_timing_)) {
    EnsureSendTimer();
  }
}

// On destruction, we want to send any data we have if we have a timer
// currently running (and thus are talking to a browser process)
PageTimingMetricsSender::~PageTimingMetricsSender() {
  if (timer_->IsRunning()) {
    timer_->Stop();
    SendNow();
  }
}

void PageTimingMetricsSender::DidObserveLoadingBehavior(
    blink::WebLoadingBehaviorFlag behavior) {
  if (behavior & metadata_->behavior_flags)
    return;
  metadata_->behavior_flags |= behavior;
  EnsureSendTimer();
}

void PageTimingMetricsSender::DidObserveNewFeatureUsage(
    blink::mojom::WebFeature feature) {
  int32_t feature_id = static_cast<int32_t>(feature);
  if (features_sent_.test(feature_id))
    return;
  features_sent_.set(feature_id);
  new_features_->features.push_back(feature);
  EnsureSendTimer();
}

void PageTimingMetricsSender::DidObserveNewCssPropertyUsage(int css_property,
                                                            bool is_animated) {
  if (is_animated && !animated_css_properties_sent_.test(css_property)) {
    animated_css_properties_sent_.set(css_property);
    new_features_->animated_css_properties.push_back(css_property);
    EnsureSendTimer();
  } else if (!is_animated && !css_properties_sent_.test(css_property)) {
    css_properties_sent_.set(css_property);
    new_features_->css_properties.push_back(css_property);
    EnsureSendTimer();
  }
}

void PageTimingMetricsSender::Send(mojom::PageLoadTimingPtr timing) {
  if (last_timing_->Equals(*timing))
    return;

  // We want to make sure that each PageTimingMetricsSender is associated
  // with a distinct page navigation. Because we reset the object on commit,
  // we can trash last_timing_ on a provisional load before SendNow() fires.
  if (!last_timing_->navigation_start.is_null() &&
      last_timing_->navigation_start != timing->navigation_start) {
    return;
  }

  last_timing_ = std::move(timing);
  EnsureSendTimer();
}

void PageTimingMetricsSender::EnsureSendTimer() {
  if (!timer_->IsRunning()) {
    // Send the first IPC eagerly to make sure the receiving side knows we're
    // sending metrics as soon as possible.
    int delay_ms =
        have_sent_ipc_ ? buffer_timer_delay_ms_ : kInitialTimerDelayMillis;
    timer_->Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(delay_ms),
        base::Bind(&PageTimingMetricsSender::SendNow, base::Unretained(this)));
  }
}

void PageTimingMetricsSender::SendNow() {
  have_sent_ipc_ = true;
  sender_->SendTiming(last_timing_, metadata_, std::move(new_features_));
  new_features_ = mojom::PageLoadFeatures::New();
}

}  // namespace page_load_metrics
