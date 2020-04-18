// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/service/vr_device_manager.h"
#include "chrome/browser/vr/service/vr_service_impl.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "device/vr/test/fake_vr_device.h"
#include "device/vr/test/fake_vr_device_provider.h"
#include "device/vr/test/fake_vr_service_client.h"
#include "device/vr/vr_device_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

namespace {

class VRDeviceManagerForTesting : public VRDeviceManager {
 public:
  explicit VRDeviceManagerForTesting(ProviderList providers)
      : VRDeviceManager(std::move(providers)) {}
  ~VRDeviceManagerForTesting() override = default;

  size_t NumberOfConnectedServices() {
    return VRDeviceManager::NumberOfConnectedServices();
  }

  // Expose this test-only method as public for tests.
  using VRDeviceManager::GetDevice;
};

class VRServiceImplForTesting : public VRServiceImpl {
 public:
  VRServiceImplForTesting() : VRServiceImpl() {}
  ~VRServiceImplForTesting() override = default;

  int GetNumberOfConnectedDisplayHosts() {
    return NumberOfConnectedDisplayHosts();
  }
};

}  // namespace

class VRDeviceManagerTest : public testing::Test {
 public:
  void onDisplaySynced() {}

 protected:
  VRDeviceManagerTest() = default;
  ~VRDeviceManagerTest() override = default;

  void SetUp() override {
    std::vector<std::unique_ptr<device::VRDeviceProvider>> providers;
    provider_ = new device::FakeVRDeviceProvider();
    providers.emplace_back(
        std::unique_ptr<device::FakeVRDeviceProvider>(provider_));
    new VRDeviceManagerForTesting(std::move(providers));
  }

  void TearDown() override { EXPECT_FALSE(VRDeviceManager::HasInstance()); }

  std::unique_ptr<VRServiceImplForTesting> BindService() {
    device::mojom::VRServiceClientPtr proxy;
    device::FakeVRServiceClient client(mojo::MakeRequest(&proxy));
    auto service = base::WrapUnique(new VRServiceImplForTesting());
    service->SetClient(
        std::move(proxy),
        base::BindRepeating(&VRDeviceManagerTest::onDisplaySynced,
                            base::Unretained(this)));
    return service;
  }

  VRDeviceManagerForTesting* DeviceManager() {
    EXPECT_TRUE(VRDeviceManager::HasInstance());
    return static_cast<VRDeviceManagerForTesting*>(
        VRDeviceManager::GetInstance());
  }

  size_t ServiceCount() { return DeviceManager()->NumberOfConnectedServices(); }

  device::FakeVRDeviceProvider* Provider() {
    EXPECT_TRUE(VRDeviceManager::HasInstance());
    return provider_;
  }

 private:
  device::FakeVRDeviceProvider* provider_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(VRDeviceManagerTest);
};

TEST_F(VRDeviceManagerTest, InitializationTest) {
  EXPECT_FALSE(Provider()->Initialized());

  // Calling GetDevices should initialize the service if it hasn't been
  // initialized yet or the providesr have been released.
  // The mojom::VRService should initialize each of it's providers upon it's own
  // initialization. And SetClient method in VRService class will invoke
  // GetVRDevices too.
  auto service = BindService();
  EXPECT_TRUE(Provider()->Initialized());
}

TEST_F(VRDeviceManagerTest, GetNoDevicesTest) {
  auto service = BindService();
  // Calling GetVRDevices should initialize the providers.
  EXPECT_TRUE(Provider()->Initialized());

  // GetDeviceByIndex should return nullptr if an invalid index in queried.
  device::VRDevice* queried_device = DeviceManager()->GetDevice(1);
  EXPECT_EQ(nullptr, queried_device);
}

TEST_F(VRDeviceManagerTest, GetDevicesTest) {
  device::FakeVRDevice* device1 = new device::FakeVRDevice();
  Provider()->AddDevice(base::WrapUnique(device1));
  // VRDeviceManager will query devices as a side effect.
  auto service_1 = BindService();
  // Should have successfully returned one device.
  EXPECT_EQ(device1, DeviceManager()->GetDevice(device1->GetId()));

  device::FakeVRDevice* device2 = new device::FakeVRDevice();
  Provider()->AddDevice(base::WrapUnique(device2));
  auto service_2 = BindService();
  // Querying the WebVRDevice index should return the correct device.
  EXPECT_EQ(device1, DeviceManager()->GetDevice(device1->GetId()));
  EXPECT_EQ(device2, DeviceManager()->GetDevice(device2->GetId()));
}

// Ensure that services are registered with the device manager as they are
// created and removed from the device manager as their connections are closed.
TEST_F(VRDeviceManagerTest, DeviceManagerRegistration) {
  EXPECT_EQ(0u, ServiceCount());
  auto service_1 = BindService();
  EXPECT_EQ(1u, ServiceCount());
  auto service_2 = BindService();
  EXPECT_EQ(2u, ServiceCount());
  service_1.reset();
  EXPECT_EQ(1u, ServiceCount());
  service_2.reset();
  EXPECT_FALSE(VRDeviceManager::HasInstance());
}

// Ensure that devices added and removed are propagated to the service after
// initialization.
TEST_F(VRDeviceManagerTest, AddRemoveDevices) {
  auto service = BindService();
  EXPECT_EQ(1u, ServiceCount());
  EXPECT_TRUE(Provider()->Initialized());
  device::FakeVRDevice* device = new device::FakeVRDevice();
  Provider()->AddDevice(base::WrapUnique(device));
  EXPECT_EQ(1, service->GetNumberOfConnectedDisplayHosts());
  Provider()->RemoveDevice(device->GetId());
  EXPECT_EQ(0, service->GetNumberOfConnectedDisplayHosts());
}

}  // namespace vr
