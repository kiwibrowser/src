// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_WEB_RTC_STATS_REPORT_CALLBACK_RESOLVER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_WEB_RTC_STATS_REPORT_CALLBACK_RESOLVER_H_

#include <memory>

#include "third_party/blink/public/platform/web_rtc_stats.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_stats_report.h"

namespace blink {

class WebRTCStatsReportCallbackResolver : public WebRTCStatsReportCallback {
 public:
  // Takes ownership of |resolver|.
  static std::unique_ptr<WebRTCStatsReportCallback> Create(
      ScriptPromiseResolver*);
  ~WebRTCStatsReportCallbackResolver() override;

 private:
  explicit WebRTCStatsReportCallbackResolver(ScriptPromiseResolver*);

  void OnStatsDelivered(std::unique_ptr<WebRTCStatsReport>) override;

  Persistent<ScriptPromiseResolver> resolver_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_WEB_RTC_STATS_REPORT_CALLBACK_RESOLVER_H_
