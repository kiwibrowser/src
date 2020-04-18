// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/debug_recording_session_factory.h"

#include <utility>

#include "base/files/file_path.h"
#include "services/audio/public/cpp/debug_recording_session.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

std::unique_ptr<media::AudioDebugRecordingSession>
CreateAudioDebugRecordingSession(
    const base::FilePath& debug_recording_file_path,
    std::unique_ptr<service_manager::Connector> connector) {
  DCHECK(connector);

  return std::make_unique<audio::DebugRecordingSession>(
      debug_recording_file_path, std::move(connector));
}

}  // namespace audio
