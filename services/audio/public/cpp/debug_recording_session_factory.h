// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_DEBUG_RECORDING_SESSION_FACTORY_H_
#define SERVICES_AUDIO_PUBLIC_CPP_DEBUG_RECORDING_SESSION_FACTORY_H_

#include <memory>

namespace base {
class FilePath;
}

namespace media {
class AudioDebugRecordingSession;
}

namespace service_manager {
class Connector;
}

namespace audio {

std::unique_ptr<media::AudioDebugRecordingSession>
CreateAudioDebugRecordingSession(
    const base::FilePath& debug_recording_file_path,
    std::unique_ptr<service_manager::Connector> connector);

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_DEBUG_RECORDING_SESSION_FACTORY_H_
