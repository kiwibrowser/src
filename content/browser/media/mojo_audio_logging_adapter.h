// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_MOJO_AUDIO_LOGGING_ADAPTER_H_
#define CONTENT_BROWSER_MEDIA_MOJO_AUDIO_LOGGING_ADAPTER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "content/common/content_export.h"
#include "media/audio/audio_logging.h"
#include "media/mojo/interfaces/audio_logging.mojom.h"

namespace content {

// This class wraps a media::mojom::AudioLogPtr into a media::AudioLog.
// TODO(crbug.com/812557): Move this class to the audio service once the audio
// service is in charge of creating and owning the AudioManager.
class CONTENT_EXPORT MojoAudioLogAdapter : public media::AudioLog {
 public:
  explicit MojoAudioLogAdapter(media::mojom::AudioLogPtr audio_log);
  ~MojoAudioLogAdapter() override;

  // media::AudioLog implementation.
  void OnCreated(const media::AudioParameters& params,
                 const std::string& device_id) override;
  void OnStarted() override;
  void OnStopped() override;
  void OnClosed() override;
  void OnError() override;
  void OnSetVolume(double volume) override;
  void OnLogMessage(const std::string& message) override;

 private:
  media::mojom::AudioLogPtr audio_log_;

  DISALLOW_COPY_AND_ASSIGN(MojoAudioLogAdapter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_MOJO_AUDIO_LOGGING_ADAPTER_H_
