// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/idle/idle.h"

#include "base/logging.h"

namespace ui {

void CalculateIdleTime(IdleTimeCallback notify) {
}

int CalculateIdleTime() {
  // TODO(android): https://crbug.com/743296.
  NOTIMPLEMENTED();
  return 0;
}

bool CheckIdleStateIsLocked() {
  // TODO(android): https://crbug.com/743296.
  NOTIMPLEMENTED();
  return false;
}

IdleState CalculateIdleState(int idle_threshold) {
  // TODO(crbug.com/878979): implementation pending.
  NOTIMPLEMENTED();
  return IdleState::IDLE_STATE_UNKNOWN;
}

}  // namespace ui
