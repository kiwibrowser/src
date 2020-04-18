// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/idle/idle.h"

#include "base/logging.h"

namespace ui {

void CalculateIdleTime(IdleTimeCallback notify) {
  // TODO(fuchsia): https://crbug.com/743296.
  NOTIMPLEMENTED();
  notify.Run(0);
}

bool CheckIdleStateIsLocked() {
  // TODO(fuchsia): https://crbug.com/743296.
  NOTIMPLEMENTED();
  return false;
}

}  // namespace ui
