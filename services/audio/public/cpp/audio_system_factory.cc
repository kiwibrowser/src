// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_system_factory.h"

#include "services/audio/public/cpp/audio_system_to_service_adapter.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

std::unique_ptr<media::AudioSystem> CreateAudioSystem(
    std::unique_ptr<service_manager::Connector> connector) {
  constexpr auto service_disconnect_timeout = base::TimeDelta::FromSeconds(1);
  return std::make_unique<audio::AudioSystemToServiceAdapter>(
      std::move(connector), service_disconnect_timeout);
}

}  // namespace audio
