// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/test/test_device_status_listener.h"

#include <memory>

namespace download {
namespace test {

class FakeBatteryStatusListener : public BatteryStatusListener {
 public:
  FakeBatteryStatusListener() : BatteryStatusListener(base::TimeDelta()) {}
  ~FakeBatteryStatusListener() override = default;

  // BatteryStatusListener implementation.
  int GetBatteryPercentageInternal() override { return 100; }

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeBatteryStatusListener);
};

TestDeviceStatusListener::TestDeviceStatusListener()
    : DeviceStatusListener(base::TimeDelta(), /* startup_delay */
                           base::TimeDelta(), /* online_delay */
                           std::make_unique<FakeBatteryStatusListener>()),
      weak_ptr_factory_(this) {}

TestDeviceStatusListener::~TestDeviceStatusListener() {
  // Mark |listening_| to false to bypass the remove observer calls in the base
  // class.
  Stop();
}

void TestDeviceStatusListener::NotifyObserver(
    const DeviceStatus& device_status) {
  status_ = device_status;
  DCHECK(observer_);
  observer_->OnDeviceStatusChanged(status_);
}

void TestDeviceStatusListener::SetDeviceStatus(const DeviceStatus& status) {
  status_ = status;
}

void TestDeviceStatusListener::Start(DeviceStatusListener::Observer* observer) {
  listening_ = true;
  observer_ = observer;

  // Simulates the delay after start up.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&TestDeviceStatusListener::StartAfterDelay,
                            weak_ptr_factory_.GetWeakPtr()));
}

void TestDeviceStatusListener::StartAfterDelay() {
  is_valid_state_ = true;
  NotifyObserver(status_);
}

void TestDeviceStatusListener::Stop() {
  status_ = DeviceStatus();
  observer_ = nullptr;
  listening_ = false;
}

}  // namespace test
}  // namespace download
