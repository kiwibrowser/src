// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/media_controller.h"

namespace ash {

MediaController::MediaController() = default;

MediaController::~MediaController() = default;

void MediaController::BindRequest(mojom::MediaControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MediaController::AddObserver(MediaCaptureObserver* observer) {
  observers_.AddObserver(observer);
}

void MediaController::RemoveObserver(MediaCaptureObserver* observer) {
  observers_.RemoveObserver(observer);
}

void MediaController::SetClient(mojom::MediaClientAssociatedPtrInfo client) {
  client_.Bind(std::move(client));
}

void MediaController::NotifyCaptureState(
    const std::vector<mojom::MediaCaptureState>& capture_states) {
  for (auto& observer : observers_)
    observer.OnMediaCaptureChanged(capture_states);
}

void MediaController::HandleMediaNextTrack() {
  if (client_)
    client_->HandleMediaNextTrack();
}

void MediaController::HandleMediaPlayPause() {
  if (client_)
    client_->HandleMediaPlayPause();
}

void MediaController::HandleMediaPrevTrack() {
  if (client_)
    client_->HandleMediaPrevTrack();
}

void MediaController::RequestCaptureState() {
  if (client_)
    client_->RequestCaptureState();
}

void MediaController::SuspendMediaSessions() {
  if (client_)
    client_->SuspendMediaSessions();
}

}  // namespace ash
