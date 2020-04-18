// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/test/mock_controller.h"

namespace download {
namespace test {

MockController::MockController() = default;
MockController::~MockController() = default;

void MockController::Initialize(const base::Closure& callback) {
  init_callback_ = callback;
}

void MockController::TriggerInitCompleted() {
  init_callback_.Run();
}

}  // namespace test
}  // namespace download
