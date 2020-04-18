// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/test/webrtc_stats_report_obtainer.h"

namespace content {

WebRTCStatsReportObtainer::CallbackWrapper::CallbackWrapper(
    scoped_refptr<WebRTCStatsReportObtainer> obtainer)
    : obtainer_(std::move(obtainer)) {}

WebRTCStatsReportObtainer::CallbackWrapper::~CallbackWrapper() {}

void WebRTCStatsReportObtainer::CallbackWrapper::OnStatsDelivered(
    std::unique_ptr<blink::WebRTCStatsReport> report) {
  obtainer_->report_ = std::move(report);
  obtainer_->run_loop_.Quit();
}

WebRTCStatsReportObtainer::WebRTCStatsReportObtainer() {}

WebRTCStatsReportObtainer::~WebRTCStatsReportObtainer() {}

std::unique_ptr<blink::WebRTCStatsReportCallback>
WebRTCStatsReportObtainer::GetStatsCallbackWrapper() {
  return std::unique_ptr<blink::WebRTCStatsReportCallback>(
      new CallbackWrapper(this));
}

blink::WebRTCStatsReport* WebRTCStatsReportObtainer::report() const {
  return report_.get();
}

blink::WebRTCStatsReport* WebRTCStatsReportObtainer::WaitForReport() {
  run_loop_.Run();
  return report_.get();
}

}  // namespace content
