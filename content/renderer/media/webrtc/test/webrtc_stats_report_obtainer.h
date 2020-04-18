// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_TEST_WEBRTC_STATS_REPORT_OBTAINER_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_TEST_WEBRTC_STATS_REPORT_OBTAINER_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "third_party/blink/public/platform/web_rtc_stats.h"

namespace content {

// The obtainer is a test-only helper class capable of waiting for a GetStats()
// callback to be called. It takes ownership of and exposes the resulting
// blink::WebRTCStatsReport.
// While WaitForReport() is waiting for the report, tasks posted on the current
// thread are executed (see base::RunLoop::Run()) making it safe to wait on the
// same thread that the stats report callback occurs on without blocking the
// callback.
class WebRTCStatsReportObtainer
    : public base::RefCountedThreadSafe<WebRTCStatsReportObtainer> {
 public:
  WebRTCStatsReportObtainer();

  std::unique_ptr<blink::WebRTCStatsReportCallback> GetStatsCallbackWrapper();

  blink::WebRTCStatsReport* report() const;
  blink::WebRTCStatsReport* WaitForReport();

 private:
  friend class base::RefCountedThreadSafe<WebRTCStatsReportObtainer>;
  friend class CallbackWrapper;
  virtual ~WebRTCStatsReportObtainer();

  // A separate class is needed for OnStatsDelivered() because ownership of the
  // callback has to be passed to GetStats().
  class CallbackWrapper : public blink::WebRTCStatsReportCallback {
   public:
    CallbackWrapper(scoped_refptr<WebRTCStatsReportObtainer> obtainer);
    ~CallbackWrapper() override;

    void OnStatsDelivered(
        std::unique_ptr<blink::WebRTCStatsReport> report) override;

   private:
    scoped_refptr<WebRTCStatsReportObtainer> obtainer_;
  };

  base::RunLoop run_loop_;
  std::unique_ptr<blink::WebRTCStatsReport> report_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_TEST_WEBRTC_STATS_REPORT_OBTAINER_H_
