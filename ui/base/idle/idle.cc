// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/idle/idle.h"

#include "base/bind.h"

namespace ui {
namespace {

void CalculateIdleStateCallback(int idle_threshold,
                                IdleCallback notify,
                                int idle_time) {
  if (idle_time >= idle_threshold)
    notify.Run(IDLE_STATE_IDLE);
  else
    notify.Run(IDLE_STATE_ACTIVE);
}

}  // namespace

void CalculateIdleState(int idle_threshold, IdleCallback notify) {
  if (CheckIdleStateIsLocked()) {
    notify.Run(IDLE_STATE_LOCKED);
    return;
  }

  CalculateIdleTime(base::Bind(&CalculateIdleStateCallback,
                               idle_threshold,
                               notify));
}

}  // namespace ui
