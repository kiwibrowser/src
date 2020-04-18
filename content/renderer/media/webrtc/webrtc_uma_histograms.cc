// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_uma_histograms.h"

#include "base/metrics/histogram_macros.h"

namespace content {

void LogUserMediaRequestWithNoResult(MediaStreamRequestState state) {
  UMA_HISTOGRAM_ENUMERATION("WebRTC.UserMediaRequest.NoResultState",
                            state,
                            NUM_MEDIA_STREAM_REQUEST_WITH_NO_RESULT);
}

void LogUserMediaRequestResult(MediaStreamRequestResult result) {
  UMA_HISTOGRAM_ENUMERATION("WebRTC.UserMediaRequest.Result2", result,
                            NUM_MEDIA_REQUEST_RESULTS);
}

void UpdateWebRTCMethodCount(blink::WebRTCAPIName api_name) {
  DVLOG(3) << "Incrementing WebRTC.webkitApiCount for "
           << static_cast<int>(api_name);
  UMA_HISTOGRAM_ENUMERATION("WebRTC.webkitApiCount", api_name,
                            blink::WebRTCAPIName::kInvalidName);
  PerSessionWebRTCAPIMetrics::GetInstance()->LogUsageOnlyOnce(api_name);
}

PerSessionWebRTCAPIMetrics::~PerSessionWebRTCAPIMetrics() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// static
PerSessionWebRTCAPIMetrics* PerSessionWebRTCAPIMetrics::GetInstance() {
  return base::Singleton<PerSessionWebRTCAPIMetrics>::get();
}

void PerSessionWebRTCAPIMetrics::IncrementStreamCounter() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ++num_streams_;
}

void PerSessionWebRTCAPIMetrics::DecrementStreamCounter() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (--num_streams_ == 0) {
    ResetUsage();
  }
}

PerSessionWebRTCAPIMetrics::PerSessionWebRTCAPIMetrics() : num_streams_(0) {
  ResetUsage();
}

void PerSessionWebRTCAPIMetrics::LogUsage(blink::WebRTCAPIName api_name) {
  DVLOG(3) << "Incrementing WebRTC.webkitApiCountPerSession for "
           << static_cast<int>(api_name);
  UMA_HISTOGRAM_ENUMERATION("WebRTC.webkitApiCountPerSession", api_name,
                            blink::WebRTCAPIName::kInvalidName);
}

void PerSessionWebRTCAPIMetrics::LogUsageOnlyOnce(
    blink::WebRTCAPIName api_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!has_used_api_[static_cast<int>(api_name)]) {
    has_used_api_[static_cast<int>(api_name)] = true;
    LogUsage(api_name);
  }
}

void PerSessionWebRTCAPIMetrics::ResetUsage() {
  for (bool& has_used_api : has_used_api_)
    has_used_api = false;
}

}  // namespace content
