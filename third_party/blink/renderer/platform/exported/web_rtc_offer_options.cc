// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_rtc_offer_options.h"

#include "third_party/blink/renderer/platform/peerconnection/rtc_offer_options_platform.h"

namespace blink {

WebRTCOfferOptions::WebRTCOfferOptions(RTCOfferOptionsPlatform* options)
    : private_(options) {}

WebRTCOfferOptions::WebRTCOfferOptions(int32_t offer_to_receive_audio,
                                       int32_t offer_to_receive_video,
                                       bool voice_activity_detection,
                                       bool ice_restart)
    : private_(RTCOfferOptionsPlatform::Create(offer_to_receive_audio,
                                               offer_to_receive_video,
                                               voice_activity_detection,
                                               ice_restart)) {}

void WebRTCOfferOptions::Assign(const WebRTCOfferOptions& other) {
  private_ = other.private_;
}

void WebRTCOfferOptions::Reset() {
  private_.Reset();
}

int32_t WebRTCOfferOptions::OfferToReceiveVideo() const {
  DCHECK(!IsNull());
  return private_->OfferToReceiveVideo();
}

int32_t WebRTCOfferOptions::OfferToReceiveAudio() const {
  DCHECK(!IsNull());
  return private_->OfferToReceiveAudio();
}

bool WebRTCOfferOptions::VoiceActivityDetection() const {
  DCHECK(!IsNull());
  return private_->VoiceActivityDetection();
}

bool WebRTCOfferOptions::IceRestart() const {
  DCHECK(!IsNull());
  return private_->IceRestart();
}

}  // namespace blink
