// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/user_activity_forwarder.h"

#include <cmath>
#include <cstdint>

#include "ui/base/user_activity/user_activity_detector.h"

namespace aura {

UserActivityForwarder::UserActivityForwarder(
    ui::mojom::UserActivityMonitorPtr monitor,
    ui::UserActivityDetector* detector)
    : monitor_(std::move(monitor)), binding_(this), detector_(detector) {
  DCHECK(detector_);

  // Round UserActivityDetector's notification interval up to the nearest
  // second (the granularity exposed by UserActivityMonitor).
  const uint32_t kNotifyIntervalSec = static_cast<uint32_t>(
      ceil(ui::UserActivityDetector::kNotifyIntervalMs / 1000.0));
  ui::mojom::UserActivityObserverPtr observer;
  binding_.Bind(mojo::MakeRequest(&observer));
  monitor_->AddUserActivityObserver(kNotifyIntervalSec, std::move(observer));
}

UserActivityForwarder::~UserActivityForwarder() {}

void UserActivityForwarder::OnUserActivity() {
  detector_->HandleExternalUserActivity();
}

}  // namespace aura
