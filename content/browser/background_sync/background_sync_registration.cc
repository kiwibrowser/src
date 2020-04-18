// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_sync/background_sync_registration.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

namespace content {

const BackgroundSyncRegistration::RegistrationId
    BackgroundSyncRegistration::kInvalidRegistrationId = -1;

const BackgroundSyncRegistration::RegistrationId
    BackgroundSyncRegistration::kInitialId = 0;

bool BackgroundSyncRegistration::Equals(
    const BackgroundSyncRegistration& other) const {
  return options_.Equals(other.options_);
}

bool BackgroundSyncRegistration::IsValid() const {
  return id_ != kInvalidRegistrationId;
}

bool BackgroundSyncRegistration::IsFiring() const {
  switch (sync_state_) {
    case blink::mojom::BackgroundSyncState::FIRING:
    case blink::mojom::BackgroundSyncState::REREGISTERED_WHILE_FIRING:
      return true;
    case blink::mojom::BackgroundSyncState::PENDING:
      return false;
  }
  NOTREACHED();
  return false;
}

}  // namespace content
