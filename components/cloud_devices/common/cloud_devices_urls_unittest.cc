// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cloud_devices/common/cloud_devices_urls.h"

#include <string>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::HasSubstr;

namespace cloud_devices {

TEST(CloudPrintURLTest, GetCloudPrintURL) {
  std::string service_url = GetCloudPrintURL().spec();
  EXPECT_THAT(service_url, HasSubstr("www.google.com"));
  EXPECT_THAT(service_url, HasSubstr("cloudprint"));
}

TEST(CloudPrintURLTest, GetCloudPrintRelativeURL) {
  EXPECT_THAT(GetCloudPrintRelativeURL("///a/b/c///").spec(),
              HasSubstr("/cloudprint/a/b/c"));

  EXPECT_THAT(GetCloudPrintRelativeURL("a/b/c").spec(),
              HasSubstr("/cloudprint/a/b/c"));
}

TEST(CloudPrintURLTest, GetCloudPrintDialogUrl) {
  std::string dialog_url =
      GetCloudPrintRelativeURL("client/dialog.html").spec();
  EXPECT_THAT(dialog_url, HasSubstr("www.google.com"));
  EXPECT_THAT(dialog_url, HasSubstr("/cloudprint/"));
  EXPECT_THAT(dialog_url, HasSubstr("/client/"));
  EXPECT_THAT(dialog_url, Not(HasSubstr("cloudprint/cloudprint")));
  EXPECT_THAT(dialog_url, HasSubstr("/dialog.html"));
}

TEST(CloudPrintURLTest, GetCloudPrintManageUrl) {
  std::string manage_url = GetCloudPrintRelativeURL("manage.html").spec();
  EXPECT_THAT(manage_url, HasSubstr("www.google.com"));
  EXPECT_THAT(manage_url, HasSubstr("/cloudprint/"));
  EXPECT_THAT(manage_url, Not(HasSubstr("/client/")));
  EXPECT_THAT(manage_url, Not(HasSubstr("cloudprint/cloudprint")));
  EXPECT_THAT(manage_url, HasSubstr("/manage.html"));
}

TEST(CloudPrintURLTest, GetCloudPrintEnableURL) {
  std::string enable_url = GetCloudPrintEnableURL("123123").spec();
  EXPECT_THAT(enable_url, HasSubstr("proxy=123123"));
  EXPECT_THAT(enable_url, HasSubstr("/enable_chrome_connector/enable.html"));
  EXPECT_THAT(enable_url, HasSubstr("/cloudprint/"));
}

TEST(CloudPrintURLTest, GetCloudPrintEnableWithSigninURL) {
  std::string enable_url = GetCloudPrintEnableWithSigninURL("123123").spec();
  EXPECT_THAT(enable_url, HasSubstr("accounts.google.com"));
  EXPECT_THAT(enable_url, HasSubstr("/ServiceLogin"));
  EXPECT_THAT(enable_url, HasSubstr("service=cloudprint"));
  EXPECT_THAT(enable_url, HasSubstr("continue="));
  EXPECT_THAT(enable_url, HasSubstr("proxy%3D123123"));
  EXPECT_THAT(enable_url, HasSubstr("%2Fenable_chrome_connector%2Fenable"));
  EXPECT_THAT(enable_url, HasSubstr("%2Fcloudprint%2F"));
}

TEST(CloudPrintURLTest, GetCloudPrintManageDeviceURL) {
  std::string manage_url = GetCloudPrintManageDeviceURL("123").spec();
  EXPECT_THAT(manage_url, HasSubstr("www.google.com"));
  EXPECT_THAT(manage_url, HasSubstr("/cloudprint"));
  EXPECT_THAT(manage_url, HasSubstr("#printers/123"));
}

TEST(CloudPrintURLTest, GetCloudPrintSigninURL) {
  std::string signin_url = GetCloudPrintSigninURL().spec();
  EXPECT_THAT(signin_url, HasSubstr("accounts.google.com"));
  EXPECT_THAT(signin_url, HasSubstr("/ServiceLogin"));
  EXPECT_THAT(signin_url, HasSubstr("service=cloudprint"));
  EXPECT_THAT(signin_url, HasSubstr("continue="));
  EXPECT_THAT(signin_url, HasSubstr("%2Fcloudprint"));
  EXPECT_THAT(signin_url, Not(HasSubstr("/cloudprint")));
}

TEST(CloudPrintURLTest, GetCloudPrintAddAccountURL) {
  std::string add_url = GetCloudPrintAddAccountURL().spec();
  EXPECT_THAT(add_url, HasSubstr("accounts.google.com"));
  EXPECT_THAT(add_url, HasSubstr("/AddSession"));
  EXPECT_THAT(add_url, HasSubstr("service=cloudprint"));
  EXPECT_THAT(add_url, HasSubstr("continue="));
  EXPECT_THAT(add_url, HasSubstr("%2Fcloudprint"));
  EXPECT_THAT(add_url, Not(HasSubstr("/cloudprint")));
}

}  // namespace cloud_devices
