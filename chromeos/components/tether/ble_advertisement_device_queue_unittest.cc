// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_advertisement_device_queue.h"

#include <memory>

#include "chromeos/components/tether/ble_constants.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace chromeos {

namespace tether {

typedef BleAdvertisementDeviceQueue::PrioritizedDeviceId PrioritizedDeviceId;

class BleAdvertisementDeviceQueueTest : public testing::Test {
 protected:
  BleAdvertisementDeviceQueueTest()
      : test_devices_(cryptauth::CreateRemoteDeviceRefListForTest(5)) {}

  void SetUp() override {
    device_queue_ = std::make_unique<BleAdvertisementDeviceQueue>();
  }

  std::unique_ptr<BleAdvertisementDeviceQueue> device_queue_;

  const cryptauth::RemoteDeviceRefList test_devices_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BleAdvertisementDeviceQueueTest);
};

TEST_F(BleAdvertisementDeviceQueueTest, TestEmptyQueue) {
  EXPECT_EQ(0u, device_queue_->GetSize());
  std::vector<std::string> to_advertise =
      device_queue_->GetDeviceIdsToWhichToAdvertise();
  EXPECT_TRUE(to_advertise.empty());
}

TEST_F(BleAdvertisementDeviceQueueTest, TestSingleDevice) {
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[0].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH)}));
  EXPECT_EQ(1u, device_queue_->GetSize());

  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId()));
}

TEST_F(BleAdvertisementDeviceQueueTest, TestSingleDevice_MoveToEnd) {
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[0].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH)}));
  EXPECT_EQ(1u, device_queue_->GetSize());

  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[0].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId()));
}

TEST_F(BleAdvertisementDeviceQueueTest, TestTwoDevices) {
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[0].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH)}));
  EXPECT_EQ(2u, device_queue_->GetSize());

  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId(),
                          test_devices_[1].GetDeviceId()));
}

TEST_F(BleAdvertisementDeviceQueueTest, TestTwoDevices_MoveToEnd) {
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[0].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH)}));
  EXPECT_EQ(2u, device_queue_->GetSize());

  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId(),
                          test_devices_[1].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[0].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[0].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[1].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId(),
                          test_devices_[1].GetDeviceId()));
}

TEST_F(BleAdvertisementDeviceQueueTest, TestThreeDevices) {
  // Note: These tests need to be rewritten if |kMaxConcurrentAdvertisements| is
  // ever changed.
  ASSERT_GT(3u, kMaxConcurrentAdvertisements);

  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[0].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[2].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH)}));
  EXPECT_EQ(3u, device_queue_->GetSize());

  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId(),
                          test_devices_[1].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[0].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[2].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[1].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[2].GetDeviceId(),
                          test_devices_[0].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[2].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId(),
                          test_devices_[1].GetDeviceId()));
}

TEST_F(BleAdvertisementDeviceQueueTest, TestAddingDevices) {
  // Note: These tests need to be rewritten if |kMaxConcurrentAdvertisements| is
  // ever changed.
  ASSERT_GT(3u, kMaxConcurrentAdvertisements);

  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[0].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH)}));
  EXPECT_EQ(2u, device_queue_->GetSize());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId(),
                          test_devices_[1].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[0].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[0].GetDeviceId()));

  // Device 0 has been unregistered; devices 3 and 4 have been registered.
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[2].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[3].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[4].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH)}));
  EXPECT_EQ(4u, device_queue_->GetSize());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[2].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[2].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[3].GetDeviceId()));

  device_queue_->MoveDeviceToEnd(test_devices_[1].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[3].GetDeviceId(),
                          test_devices_[4].GetDeviceId()));
}

TEST_F(BleAdvertisementDeviceQueueTest, TestMultiplePriorityLevels) {
  // Note: These tests need to be rewritten if |kMaxConcurrentAdvertisements| is
  // ever changed.
  ASSERT_GT(3u, kMaxConcurrentAdvertisements);

  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[0].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[2].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_MEDIUM),
       PrioritizedDeviceId(test_devices_[3].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_MEDIUM),
       PrioritizedDeviceId(test_devices_[4].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_LOW)}));
  EXPECT_EQ(5u, device_queue_->GetSize());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[0].GetDeviceId(),
                          test_devices_[1].GetDeviceId()));

  // Moving a high-priority device to the end of the queue should still keep it
  // before all other priority levels.
  device_queue_->MoveDeviceToEnd(test_devices_[0].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[0].GetDeviceId()));

  // Device 0 has been unregistered; device 2 has moved to low-priority.
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_HIGH),
       PrioritizedDeviceId(test_devices_[2].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_LOW),
       PrioritizedDeviceId(test_devices_[3].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_MEDIUM),
       PrioritizedDeviceId(test_devices_[4].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_LOW)}));
  EXPECT_EQ(4u, device_queue_->GetSize());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[3].GetDeviceId()));

  // Moving a high-priority device to the end of the queue should still keep it
  // before all other priority levels.
  device_queue_->MoveDeviceToEnd(test_devices_[1].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[3].GetDeviceId()));

  // Likewise, moving a medium-priority device to the end of the queue should
  // still keep it before all low-priority devices.
  device_queue_->MoveDeviceToEnd(test_devices_[3].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId(),
                          test_devices_[3].GetDeviceId()));

  // Now, all devices are priority medium. Since device 3 was already at the
  // medium priority, it should still be in front of the other devices that
  // were just added.
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_MEDIUM),
       PrioritizedDeviceId(test_devices_[2].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_MEDIUM),
       PrioritizedDeviceId(test_devices_[3].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_MEDIUM),
       PrioritizedDeviceId(test_devices_[4].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_MEDIUM)}));
  EXPECT_EQ(4u, device_queue_->GetSize());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[3].GetDeviceId(),
                          test_devices_[1].GetDeviceId()));

  // Since all devices are the same priority, moving one to the end of the queue
  // should bring a new one to the front.
  device_queue_->MoveDeviceToEnd(test_devices_[1].GetDeviceId());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[3].GetDeviceId(),
                          test_devices_[2].GetDeviceId()));

  // Leave only one low-priority device left.
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      {PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                           ConnectionPriority::CONNECTION_PRIORITY_LOW)}));
  EXPECT_EQ(1u, device_queue_->GetSize());
  EXPECT_THAT(device_queue_->GetDeviceIdsToWhichToAdvertise(),
              ElementsAre(test_devices_[1].GetDeviceId()));

  // Empty the queue.
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(
      std::vector<PrioritizedDeviceId>()));
  EXPECT_EQ(0u, device_queue_->GetSize());
  EXPECT_TRUE(device_queue_->GetDeviceIdsToWhichToAdvertise().empty());
}

TEST_F(BleAdvertisementDeviceQueueTest, TestSettingSameDevices) {
  std::vector<PrioritizedDeviceId> prioritized_devices = {
      PrioritizedDeviceId(test_devices_[0].GetDeviceId(),
                          ConnectionPriority::CONNECTION_PRIORITY_HIGH)};
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(prioritized_devices));

  // Setting the same devices again should return false.
  EXPECT_FALSE(device_queue_->SetPrioritizedDeviceIds(prioritized_devices));
  EXPECT_FALSE(device_queue_->SetPrioritizedDeviceIds(prioritized_devices));

  prioritized_devices.push_back(
      PrioritizedDeviceId(test_devices_[1].GetDeviceId(),
                          ConnectionPriority::CONNECTION_PRIORITY_HIGH));
  EXPECT_TRUE(device_queue_->SetPrioritizedDeviceIds(prioritized_devices));
}

}  // namespace tether

}  // namespace cryptauth
