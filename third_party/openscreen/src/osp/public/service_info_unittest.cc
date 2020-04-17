// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/service_info.h"

#include <utility>

#include "osp_base/error.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {

TEST(ServiceInfoTest, Compare) {
  const ServiceInfo receiver1{
      "id1", "name1", 1, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo receiver2{
      "id2", "name2", 1, {{192, 168, 1, 11}, 12346}, {}};
  const ServiceInfo receiver1_alt_id{
      "id3", "name1", 1, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo receiver1_alt_name{
      "id1", "name2", 1, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo receiver1_alt_interface{
      "id1", "name1", 2, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo receiver1_alt_ip{
      "id3", "name1", 1, {{192, 168, 1, 12}, 12345}, {}};
  const ServiceInfo receiver1_alt_port{
      "id3", "name1", 1, {{192, 168, 1, 10}, 12645}, {}};
  ServiceInfo receiver1_ipv6{"id3", "name1", 1, {}, {}};

  ErrorOr<IPAddress> address = IPAddress::Parse("::12:34");
  ASSERT_TRUE(address);
  receiver1_ipv6.v6_endpoint.address = address.value();

  EXPECT_EQ(receiver1, receiver1);
  EXPECT_EQ(receiver2, receiver2);
  EXPECT_NE(receiver1, receiver2);
  EXPECT_NE(receiver2, receiver1);

  EXPECT_NE(receiver1, receiver1_alt_id);
  EXPECT_NE(receiver1, receiver1_alt_name);
  EXPECT_NE(receiver1, receiver1_alt_interface);
  EXPECT_NE(receiver1, receiver1_alt_ip);
  EXPECT_NE(receiver1, receiver1_alt_port);
  EXPECT_NE(receiver1, receiver1_ipv6);
}

TEST(ServiceInfoTest, Update) {
  ServiceInfo original{"foo", "baz", 1, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo updated{"foo", "buzz", 1, {{193, 169, 2, 11}, 1234}, {}};
  original.Update("buzz", 1, {{193, 169, 2, 11}, 1234}, {});
  EXPECT_EQ(original, updated);
}
}  // namespace openscreen
