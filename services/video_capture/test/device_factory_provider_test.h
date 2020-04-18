// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_VIDEO_CAPTURE_TEST_DEVICE_FACTORY_PROVIDER_TEST_H_
#define SERVICES_VIDEO_CAPTURE_VIDEO_CAPTURE_TEST_DEVICE_FACTORY_PROVIDER_TEST_H_

#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/mojom/service_manager.mojom.h"
#include "services/video_capture/public/mojom/device_factory_provider.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace video_capture {

class ServiceManagerListenerImpl
    : public service_manager::mojom::ServiceManagerListener {
 public:
  ServiceManagerListenerImpl(
      service_manager::mojom::ServiceManagerListenerRequest request,
      base::RunLoop* loop);
  ~ServiceManagerListenerImpl() override;

  // mojom::ServiceManagerListener implementation.
  void OnInit(std::vector<service_manager::mojom::RunningServiceInfoPtr>
                  instances) override {
    loop_->Quit();
  }
  void OnServiceCreated(
      service_manager::mojom::RunningServiceInfoPtr instance) override {}
  void OnServiceStarted(const service_manager::Identity& identity,
                        uint32_t pid) override {}
  void OnServiceFailedToStart(
      const service_manager::Identity& identity) override {}
  void OnServicePIDReceived(const service_manager::Identity& identity,
                            uint32_t pid) override {}

  MOCK_METHOD1(OnServiceStopped,
               void(const service_manager::Identity& identity));

 private:
  mojo::Binding<service_manager::mojom::ServiceManagerListener> binding_;
  base::RunLoop* loop_;
};

// Basic test fixture that sets up a connection to the fake device factory.
class DeviceFactoryProviderTest : public service_manager::test::ServiceTest {
 public:
  DeviceFactoryProviderTest();
  ~DeviceFactoryProviderTest() override;

  void SetUp() override;

 protected:
  mojom::DeviceFactoryProviderPtr factory_provider_;
  mojom::DeviceFactoryPtr factory_;
  base::MockCallback<mojom::DeviceFactory::GetDeviceInfosCallback>
      device_info_receiver_;
  std::unique_ptr<ServiceManagerListenerImpl> service_state_observer_;
};

}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_VIDEO_CAPTURE_TEST_DEVICE_FACTORY_PROVIDER_TEST_H_
