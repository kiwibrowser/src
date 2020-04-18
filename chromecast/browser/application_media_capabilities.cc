// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/application_media_capabilities.h"

#include <utility>

#include "chromecast/base/bitstream_audio_codecs.h"

namespace chromecast {
namespace shell {

ApplicationMediaCapabilities::ApplicationMediaCapabilities()
    : supported_bitstream_audio_codecs_(kBitstreamAudioCodecNone) {}

ApplicationMediaCapabilities::~ApplicationMediaCapabilities() = default;

void ApplicationMediaCapabilities::AddBinding(
    mojom::ApplicationMediaCapabilitiesRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ApplicationMediaCapabilities::SetSupportedBitstreamAudioCodecs(
    int codecs) {
  supported_bitstream_audio_codecs_ = codecs;
  observers_.ForAllPtrs(
      [codecs](mojom::ApplicationMediaCapabilitiesObserver* observer) {
        observer->OnSupportedBitstreamAudioCodecsChanged(codecs);
      });
}

void ApplicationMediaCapabilities::AddObserver(
    mojom::ApplicationMediaCapabilitiesObserverPtr observer) {
  observer->OnSupportedBitstreamAudioCodecsChanged(
      supported_bitstream_audio_codecs_);
  observers_.AddPtr(std::move(observer));
}

}  // namespace shell
}  // namespace chromecast
