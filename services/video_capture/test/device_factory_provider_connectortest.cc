// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "media/base/media_switches.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "services/video_capture/public/mojom/constants.mojom.h"
#include "services/video_capture/public/mojom/device_factory_provider.mojom.h"
#include "services/video_capture/service_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace video_capture {

using testing::Exactly;
using testing::_;
using testing::Invoke;
using testing::InvokeWithoutArgs;

// Test fixture that creates a video_capture::ServiceImpl and sets up a
// local service_manager::Connector through which client code can connect to
// it.
class DeviceFactoryProviderConnectorTest : public ::testing::Test {
 public:
  DeviceFactoryProviderConnectorTest() {}
  ~DeviceFactoryProviderConnectorTest() override {}

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kUseFakeDeviceForMediaStream);
    std::unique_ptr<ServiceImpl> service_impl = std::make_unique<ServiceImpl>();
    service_impl_ = service_impl.get();
    connector_factory_ =
        service_manager::TestConnectorFactory::CreateForUniqueService(
            std::move(service_impl));
    connector_ = connector_factory_->CreateConnector();
    connector_->BindInterface(mojom::kServiceName, &factory_provider_);
    factory_provider_->SetShutdownDelayInSeconds(0.0f);
    factory_provider_->ConnectToDeviceFactory(mojo::MakeRequest(&factory_));
  }

 protected:
  ServiceImpl* service_impl_;
  mojom::DeviceFactoryProviderPtr factory_provider_;
  mojom::DeviceFactoryPtr factory_;
  base::MockCallback<mojom::DeviceFactory::GetDeviceInfosCallback>
      device_info_receiver_;
  std::unique_ptr<service_manager::Connector> connector_;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
};

TEST_F(DeviceFactoryProviderConnectorTest, ServiceNotQuitWhenClientConnected) {
  base::RunLoop wait_loop;

  mojom::DeviceFactoryProviderPtr leftover_provider_;
  connector_->BindInterface(mojom::kServiceName, &leftover_provider_);
  service_impl_->SetFactoryProviderClientDisconnectedObserver(
      wait_loop.QuitClosure());

  factory_.reset();
  factory_provider_.reset();

  wait_loop.Run();

  // Verify that the original provider is not bound while the
  // |leftover_provider| is still bound.
  EXPECT_FALSE(factory_provider_.is_bound());
  EXPECT_TRUE(leftover_provider_.is_bound());
}

}  // namespace video_capture
