// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/receiver_list.h"

#include "osp_base/error.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {

TEST(ReceiverListTest, AddReceivers) {
  ReceiverList list;

  EXPECT_TRUE(list.receivers().empty());

  const ServiceInfo receiver1{
      "id1", "name1", 1, {{192, 168, 1, 10}, 12345}, {}};
  list.OnReceiverAdded(receiver1);

  ASSERT_EQ(1u, list.receivers().size());
  EXPECT_EQ(receiver1, list.receivers()[0]);

  const ServiceInfo receiver2{
      "id2", "name2", 1, {{192, 168, 1, 11}, 12345}, {}};
  list.OnReceiverAdded(receiver2);

  ASSERT_EQ(2u, list.receivers().size());
  EXPECT_EQ(receiver1, list.receivers()[0]);
  EXPECT_EQ(receiver2, list.receivers()[1]);

  list.OnReceiverAdded(receiver1);

  // No duplicate checking.
  ASSERT_EQ(3u, list.receivers().size());
  EXPECT_EQ(receiver1, list.receivers()[0]);
  EXPECT_EQ(receiver2, list.receivers()[1]);
  EXPECT_EQ(receiver1, list.receivers()[2]);
}

TEST(ReceiverListTest, ChangeReceivers) {
  ReceiverList list;
  const ServiceInfo receiver1{
      "id1", "name1", 1, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo receiver2{
      "id2", "name2", 1, {{192, 168, 1, 11}, 12345}, {}};
  const ServiceInfo receiver3{
      "id3", "name3", 1, {{192, 168, 1, 12}, 12345}, {}};
  const ServiceInfo receiver1_alt_name{
      "id1", "name1 alt", 1, {{192, 168, 1, 10}, 12345}, {}};
  list.OnReceiverAdded(receiver1);
  list.OnReceiverAdded(receiver2);

  EXPECT_TRUE(list.OnReceiverChanged(receiver1_alt_name).ok());
  EXPECT_FALSE(list.OnReceiverChanged(receiver3).ok());

  ASSERT_EQ(2u, list.receivers().size());
  EXPECT_EQ(receiver1_alt_name, list.receivers()[0]);
  EXPECT_EQ(receiver2, list.receivers()[1]);
}

TEST(ReceiverListTest, RemoveReceivers) {
  ReceiverList list;
  const ServiceInfo receiver1{
      "id1", "name1", 1, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo receiver2{
      "id2", "name2", 1, {{192, 168, 1, 11}, 12345}, {}};
  EXPECT_FALSE(list.OnReceiverRemoved(receiver1).ok());
  list.OnReceiverAdded(receiver1);
  EXPECT_FALSE(list.OnReceiverRemoved(receiver2).ok());
  list.OnReceiverAdded(receiver2);
  list.OnReceiverAdded(receiver1);

  EXPECT_TRUE(list.OnReceiverRemoved(receiver1).ok());

  ASSERT_EQ(1u, list.receivers().size());
  EXPECT_EQ(receiver2, list.receivers()[0]);
}

TEST(ReceiverListTest, RemoveAllReceivers) {
  ReceiverList list;
  const ServiceInfo receiver1{
      "id1", "name1", 1, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo receiver2{
      "id2", "name2", 1, {{192, 168, 1, 11}, 12345}, {}};
  EXPECT_FALSE(list.OnAllReceiversRemoved().ok());
  list.OnReceiverAdded(receiver1);
  list.OnReceiverAdded(receiver2);

  EXPECT_TRUE(list.OnAllReceiversRemoved().ok());
  ASSERT_TRUE(list.receivers().empty());
}

}  // namespace openscreen
