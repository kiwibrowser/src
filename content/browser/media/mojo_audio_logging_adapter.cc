// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/mojo_audio_logging_adapter.h"

#include <utility>

#include "content/browser/media/media_internals.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

MojoAudioLogAdapter::MojoAudioLogAdapter(media::mojom::AudioLogPtr audio_log)
    : audio_log_(std::move(audio_log)) {}

MojoAudioLogAdapter::~MojoAudioLogAdapter() = default;

void MojoAudioLogAdapter::OnCreated(const media::AudioParameters& params,
                                    const std::string& device_id) {
  audio_log_->OnCreated(params, device_id);
}

void MojoAudioLogAdapter::OnStarted() {
  audio_log_->OnStarted();
}

void MojoAudioLogAdapter::OnStopped() {
  audio_log_->OnStopped();
}

void MojoAudioLogAdapter::OnClosed() {
  audio_log_->OnClosed();
}

void MojoAudioLogAdapter::OnError() {
  audio_log_->OnError();
}

void MojoAudioLogAdapter::OnSetVolume(double volume) {
  audio_log_->OnSetVolume(volume);
}

void MojoAudioLogAdapter::OnLogMessage(const std::string& message) {
  audio_log_->OnLogMessage(message);
}

}  // namespace content
