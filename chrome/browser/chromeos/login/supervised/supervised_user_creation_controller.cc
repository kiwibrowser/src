// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_controller.h"

namespace chromeos {

// static
const int SupervisedUserCreationController::kDummyAvatarIndex = -111;

SupervisedUserCreationController::StatusConsumer::~StatusConsumer() {}

// static
SupervisedUserCreationController*
    SupervisedUserCreationController::current_controller_ = NULL;

SupervisedUserCreationController::SupervisedUserCreationController(
    SupervisedUserCreationController::StatusConsumer* consumer)
    : consumer_(consumer) {
  DCHECK(!current_controller_) << "More than one controller exist.";
  current_controller_ = this;
}

SupervisedUserCreationController::~SupervisedUserCreationController() {
  current_controller_ = NULL;
}

}  // namespace chromeos
