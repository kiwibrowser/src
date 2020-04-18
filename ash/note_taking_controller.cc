// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/note_taking_controller.h"

namespace ash {

NoteTakingController::NoteTakingController() : binding_(this) {}

NoteTakingController::~NoteTakingController() = default;

void NoteTakingController::BindRequest(
    mojom::NoteTakingControllerRequest request) {
  binding_.Bind(std::move(request));
}

void NoteTakingController::SetClient(
    mojom::NoteTakingControllerClientPtr client) {
  DCHECK(!client_);
  client_ = std::move(client);
  client_.set_connection_error_handler(base::Bind(
      &NoteTakingController::OnClientConnectionLost, base::Unretained(this)));
}

bool NoteTakingController::CanCreateNote() const {
  return static_cast<bool>(client_);
}

void NoteTakingController::CreateNote() {
  DCHECK(client_);
  client_->CreateNote();
}

void NoteTakingController::OnClientConnectionLost() {
  client_.reset();
  binding_.Close();
}

void NoteTakingController::FlushMojoForTesting() {
  if (client_)
    client_.FlushForTesting();
}

}  // namespace ash
