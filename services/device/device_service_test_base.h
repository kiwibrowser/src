// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_DEVICE_SERVICE_TEST_BASE_H_
#define SERVICES_DEVICE_DEVICE_SERVICE_TEST_BASE_H_

#include "base/macros.h"
#include "base/threading/thread.h"
#include "services/service_manager/public/cpp/service_test.h"

namespace device {

const char kTestGeolocationApiKey[] = "FakeApiKeyForTest";

// Base class responsible to setup Device Service for test.
class DeviceServiceTestBase : public service_manager::test::ServiceTest {
 public:
  DeviceServiceTestBase();
  ~DeviceServiceTestBase() override;

 protected:
  base::Thread file_thread_;
  base::Thread io_thread_;

 private:
  // service_manager::test::ServiceTest:
  std::unique_ptr<service_manager::Service> CreateService() override;

  DISALLOW_COPY_AND_ASSIGN(DeviceServiceTestBase);
};

}  // namespace device

#endif  // SERVICES_DEVICE_DEVICE_SERVICE_TEST_BASE_H_
