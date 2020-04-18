// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_rtc_answer_options.h"

#include "third_party/blink/renderer/platform/peerconnection/rtc_answer_options_platform.h"

namespace blink {

WebRTCAnswerOptions::WebRTCAnswerOptions(RTCAnswerOptionsPlatform* options)
    : private_(options) {}

void WebRTCAnswerOptions::Assign(const WebRTCAnswerOptions& other) {
  private_ = other.private_;
}

void WebRTCAnswerOptions::Reset() {
  private_.Reset();
}

bool WebRTCAnswerOptions::VoiceActivityDetection() const {
  DCHECK(!private_.IsNull());
  return private_->VoiceActivityDetection();
}

}  // namespace blink
