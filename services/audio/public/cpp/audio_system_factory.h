// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_FACTORY_H_
#define SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_FACTORY_H_

#include <memory>

namespace media {
class AudioSystem;
}

namespace service_manager {
class Connector;
}

namespace audio {

// Creates an instance of AudioSystem which will be bound to the thread it's
// used on for the first time.
std::unique_ptr<media::AudioSystem> CreateAudioSystem(
    std::unique_ptr<service_manager::Connector> connector);

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_FACTORY_H_
