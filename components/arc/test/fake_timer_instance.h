// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TEST_FAKE_TIMER_INSTANCE_H_
#define COMPONENTS_ARC_TEST_FAKE_TIMER_INSTANCE_H_

#include <vector>

#include "components/arc/common/timer.mojom.h"
#include "components/arc/timer/create_timer_request.h"

namespace arc {

class FakeTimerInstance : public mojom::TimerInstance {
 public:
  FakeTimerInstance();
  ~FakeTimerInstance() override;

  // mojom::TimerInstance overrides:
  void Init(mojom::TimerHostPtr host_ptr, InitCallback callback) override;

  // Calls mojom::TimerHost::CreateTimers.
  void CallCreateTimers(std::vector<CreateTimerRequest> arc_timer_requests,
                        mojom::TimerHost::CreateTimersCallback callback);

 private:
  mojom::TimerHostPtr host_ptr_;

  DISALLOW_COPY_AND_ASSIGN(FakeTimerInstance);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_FAKE_TIMER_INSTANCE_H_
