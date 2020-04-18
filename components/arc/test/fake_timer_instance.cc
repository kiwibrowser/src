// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "components/arc/test/fake_timer_instance.h"

namespace arc {

FakeTimerInstance::FakeTimerInstance() = default;

FakeTimerInstance::~FakeTimerInstance() = default;

void FakeTimerInstance::Init(mojom::TimerHostPtr host_ptr,
                             InitCallback callback) {
  host_ptr_ = std::move(host_ptr);
  std::move(callback).Run();
}

void FakeTimerInstance::CallCreateTimers(
    std::vector<CreateTimerRequest> arc_timer_requests,
    mojom::TimerHost::CreateTimersCallback callback) {
  host_ptr_->CreateTimers(std::move(arc_timer_requests), std::move(callback));
}

}  // namespace arc
