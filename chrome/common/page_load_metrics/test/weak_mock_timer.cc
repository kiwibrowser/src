// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/page_load_metrics/test/weak_mock_timer.h"

namespace page_load_metrics {
namespace test {

WeakMockTimer::WeakMockTimer()
    : MockTimer(false /* retain_user_task */, false /* is_repeating */) {}

WeakMockTimerProvider::WeakMockTimerProvider() {}
WeakMockTimerProvider::~WeakMockTimerProvider() {}

base::MockTimer* WeakMockTimerProvider::GetMockTimer() const {
  return timer_.get();
}

void WeakMockTimerProvider::SetMockTimer(base::WeakPtr<WeakMockTimer> timer) {
  timer_ = timer;
}

}  // namespace test
}  // namespace page_load_metrics
