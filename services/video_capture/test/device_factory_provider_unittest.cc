// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "services/video_capture/public/mojom/constants.mojom.h"
#include "services/video_capture/public/mojom/device_factory.mojom.h"
#include "services/video_capture/test/device_factory_provider_test.h"
#include "services/video_capture/test/mock_producer.h"

using testing::_;
using testing::Exactly;
using testing::Invoke;
using testing::InvokeWithoutArgs;

namespace video_capture {

// This alias ensures test output is easily attributed to this service's tests.
// TODO(rockot/chfremer): Consider just renaming the type.
using VideoCaptureServiceDeviceFactoryProviderTest = DeviceFactoryProviderTest;

// Tests that an answer arrives from the service when calling
// GetDeviceInfos().
TEST_F(VideoCaptureServiceDeviceFactoryProviderTest,
       GetDeviceInfosCallbackArrives) {
  base::RunLoop wait_loop;
  EXPECT_CALL(device_info_receiver_, Run(_))
      .Times(Exactly(1))
      .WillOnce(InvokeWithoutArgs([&wait_loop]() { wait_loop.Quit(); }));

  factory_->GetDeviceInfos(device_info_receiver_.Get());
  wait_loop.Run();
}

TEST_F(VideoCaptureServiceDeviceFactoryProviderTest,
       FakeDeviceFactoryEnumeratesOneDevice) {
  base::RunLoop wait_loop;
  size_t num_devices_enumerated = 0;
  EXPECT_CALL(device_info_receiver_, Run(_))
      .Times(Exactly(1))
      .WillOnce(
          Invoke([&wait_loop, &num_devices_enumerated](
                     const std::vector<media::VideoCaptureDeviceInfo>& infos) {
            num_devices_enumerated = infos.size();
            wait_loop.Quit();
          }));

  factory_->GetDeviceInfos(device_info_receiver_.Get());
  wait_loop.Run();
  ASSERT_EQ(1u, num_devices_enumerated);
}

// Tests that an added virtual device will be returned in the callback
// when calling GetDeviceInfos.
TEST_F(VideoCaptureServiceDeviceFactoryProviderTest,
       VirtualDeviceEnumeratedAfterAdd) {
  base::RunLoop wait_loop;
  const std::string virtual_device_id = "/virtual/device";
  media::VideoCaptureDeviceInfo info;
  info.descriptor.device_id = virtual_device_id;
  mojom::SharedMemoryVirtualDevicePtr virtual_device_proxy;
  mojom::ProducerPtr producer_proxy;
  MockProducer producer(mojo::MakeRequest(&producer_proxy));
  EXPECT_CALL(device_info_receiver_, Run(_))
      .Times(Exactly(1))
      .WillOnce(
          Invoke([&wait_loop, virtual_device_id](
                     const std::vector<media::VideoCaptureDeviceInfo>& infos) {
            bool virtual_device_enumerated = false;
            for (const auto& info : infos) {
              if (info.descriptor.device_id == virtual_device_id) {
                virtual_device_enumerated = true;
                break;
              }
            }
            EXPECT_TRUE(virtual_device_enumerated);
            wait_loop.Quit();
          }));
  factory_->AddSharedMemoryVirtualDevice(
      info, std::move(producer_proxy),
      mojo::MakeRequest(&virtual_device_proxy));
  factory_->GetDeviceInfos(device_info_receiver_.Get());
  wait_loop.Run();
}

// Tests that VideoCaptureDeviceFactory::CreateDevice() returns an error
// code when trying to create a device for an invalid descriptor.
TEST_F(VideoCaptureServiceDeviceFactoryProviderTest,
       ErrorCodeOnCreateDeviceForInvalidDescriptor) {
  const std::string invalid_device_id = "invalid";
  base::RunLoop wait_loop;
  mojom::DevicePtr fake_device_proxy;
  base::MockCallback<mojom::DeviceFactory::CreateDeviceCallback>
      create_device_proxy_callback;
  EXPECT_CALL(create_device_proxy_callback,
              Run(mojom::DeviceAccessResultCode::ERROR_DEVICE_NOT_FOUND))
      .Times(1)
      .WillOnce(InvokeWithoutArgs([&wait_loop]() { wait_loop.Quit(); }));
  factory_->GetDeviceInfos(device_info_receiver_.Get());
  factory_->CreateDevice(invalid_device_id,
                         mojo::MakeRequest(&fake_device_proxy),
                         create_device_proxy_callback.Get());
  wait_loop.Run();
}

// Test that CreateDevice() will succeed when trying to create a device
// for an added virtual device.
TEST_F(VideoCaptureServiceDeviceFactoryProviderTest,
       CreateDeviceSuccessForVirtualDevice) {
  base::RunLoop wait_loop;
  const std::string virtual_device_id = "/virtual/device";
  media::VideoCaptureDeviceInfo info;
  info.descriptor.device_id = virtual_device_id;
  mojom::DevicePtr device_proxy;
  mojom::SharedMemoryVirtualDevicePtr virtual_device_proxy;
  mojom::ProducerPtr producer_proxy;
  MockProducer producer(mojo::MakeRequest(&producer_proxy));
  base::MockCallback<mojom::DeviceFactory::CreateDeviceCallback>
      create_device_proxy_callback;
  EXPECT_CALL(create_device_proxy_callback,
              Run(mojom::DeviceAccessResultCode::SUCCESS))
      .Times(1)
      .WillOnce(InvokeWithoutArgs([&wait_loop]() { wait_loop.Quit(); }));
  factory_->AddSharedMemoryVirtualDevice(
      info, std::move(producer_proxy),
      mojo::MakeRequest(&virtual_device_proxy));
  factory_->CreateDevice(virtual_device_id, mojo::MakeRequest(&device_proxy),
                         create_device_proxy_callback.Get());
  wait_loop.Run();
}

// Tests that the service requests to be closed when the only client disconnects
// after not having done anything other than obtaining a connection to the
// fake device factory.
TEST_F(VideoCaptureServiceDeviceFactoryProviderTest,
       ServiceQuitsWhenSingleClientDisconnected) {
  base::RunLoop wait_loop;
  EXPECT_CALL(*service_state_observer_, OnServiceStopped(_))
      .WillOnce(Invoke([&wait_loop](const service_manager::Identity& identity) {
        if (identity.name() == mojom::kServiceName)
          wait_loop.Quit();
      }));

  // Exercise: Disconnect from service by discarding our references to it.
  factory_.reset();
  factory_provider_.reset();

  wait_loop.Run();
}

// Tests that the service requests to be closed when the all clients disconnect
// after not having done anything other than obtaining a connection to the
// fake device factory.
TEST_F(VideoCaptureServiceDeviceFactoryProviderTest,
       ServiceQuitsWhenAllClientsDisconnected) {
  // Bind another client to the DeviceFactoryProvider interface.
  mojom::DeviceFactoryProviderPtr unused_provider;
  connector()->BindInterface(mojom::kServiceName, &unused_provider);

  base::RunLoop wait_loop;
  EXPECT_CALL(*service_state_observer_, OnServiceStopped(_))
      .WillOnce(Invoke([&wait_loop](const service_manager::Identity& identity) {
        if (identity.name() == mojom::kServiceName)
          wait_loop.Quit();
      }));

  // Exercise: Disconnect from service by discarding our references to it.
  factory_.reset();
  factory_provider_.reset();
  unused_provider.reset();

  wait_loop.Run();
}

}  // namespace video_capture
