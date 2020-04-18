// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_device_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

namespace {

// Asserts equality between the two objects.
void ExpectEqual(const DialDeviceData& first, const DialDeviceData& second) {
  EXPECT_EQ(first.device_id(), second.device_id());
  EXPECT_EQ(first.label(), second.label());
  EXPECT_EQ(first.device_description_url(), second.device_description_url());
  EXPECT_EQ(first.response_time(), second.response_time());
  EXPECT_EQ(first.max_age(), second.max_age());
  EXPECT_EQ(first.config_id(), second.config_id());
}

}  // namespace

TEST(DialDeviceDataTest, TestUpdateFrom) {
  DialDeviceData original;
  original.set_device_id("device_a");
  original.set_label("label_a");
  original.set_device_description_url(GURL("http://127.0.0.1/dd-a.xml"));
  original.set_response_time(base::Time::FromInternalValue(1000));
  original.set_max_age(100);
  original.set_config_id(1);

  DialDeviceData new_data;
  new_data.set_device_id("device_a");
  new_data.set_device_description_url(GURL("http://127.0.0.1/dd-a.xml"));
  new_data.set_response_time(base::Time::FromInternalValue(1000));
  new_data.set_max_age(100);
  new_data.set_config_id(1);

  DialDeviceData original_1(original);
  EXPECT_FALSE(original_1.UpdateFrom(new_data));
  ExpectEqual(original_1, original);

  DialDeviceData original_2(original);
  new_data.set_max_age(200);
  new_data.set_response_time(base::Time::FromInternalValue(2000));
  EXPECT_FALSE(original_2.UpdateFrom(new_data));

  EXPECT_EQ("device_a", original_2.device_id());
  EXPECT_EQ(GURL("http://127.0.0.1/dd-a.xml"),
            original_2.device_description_url());
  EXPECT_EQ("label_a", original_2.label());
  EXPECT_EQ(200, original_2.max_age());
  EXPECT_EQ(base::Time::FromInternalValue(2000), original_2.response_time());
  EXPECT_EQ(1, original_2.config_id());

  DialDeviceData original_3(original);
  new_data.set_device_description_url(GURL("http://127.0.0.2/dd-b.xml"));
  new_data.set_config_id(2);
  EXPECT_TRUE(original_3.UpdateFrom(new_data));

  EXPECT_EQ("device_a", original_3.device_id());
  EXPECT_EQ(GURL("http://127.0.0.2/dd-b.xml"),
            original_3.device_description_url());
  EXPECT_EQ("label_a", original_3.label());
  EXPECT_EQ(200, original_3.max_age());
  EXPECT_EQ(base::Time::FromInternalValue(2000), original_3.response_time());
  EXPECT_EQ(2, original_3.config_id());
}

TEST(DialDeviceDataTest, TestIsDeviceDescriptionUrl) {
  EXPECT_FALSE(DialDeviceData::IsDeviceDescriptionUrl(
      GURL("http://some.device.com/dd.xml")));
  EXPECT_FALSE(DialDeviceData::IsDeviceDescriptionUrl(
      GURL("https://some.device.com/dd.xml")));
  EXPECT_TRUE(DialDeviceData::IsDeviceDescriptionUrl(
      GURL("http://192.168.1.1:1234/dd.xml")));
  EXPECT_TRUE(DialDeviceData::IsDeviceDescriptionUrl(
      GURL("https://192.168.1.1:1234/dd.xml")));

  EXPECT_FALSE(DialDeviceData::IsDeviceDescriptionUrl(GURL()));
  EXPECT_FALSE(DialDeviceData::IsDeviceDescriptionUrl(GURL(std::string())));
  EXPECT_FALSE(
      DialDeviceData::IsDeviceDescriptionUrl(GURL("file://path/to/file")));
}

}  // namespace media_router
