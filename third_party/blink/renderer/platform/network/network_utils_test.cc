// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/network/network_utils.h"

#include "net/base/ip_address.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

TEST(NetworkUtilsTest, IsReservedIPAddress) {
  // Unreserved IPv4 addresses (in various forms).
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("8.8.8.8"));
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("99.64.0.0"));
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("212.15.0.0"));
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("212.15"));
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("212.15.0"));
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("3557752832"));

  // Reserved IPv4 addresses (in various forms).
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("192.168.0.0"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("192.168.0.6"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("10.0.0.5"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("10.0.0"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("10.0"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("3232235526"));

  // Unreserved IPv6 addresses.
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress(
      "[FFC0:ba98:7654:3210:FEDC:BA98:7654:3210]"));
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress(
      "[2000:ba98:7654:2301:EFCD:BA98:7654:3210]"));
  // IPv4-mapped to IPv6
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("[::ffff:8.8.8.8]"));

  // Reserved IPv6 addresses.
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("[::1]"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("[::192.9.5.5]"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("[::ffff:192.168.1.1]"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress("[FEED::BEEF]"));
  EXPECT_TRUE(NetworkUtils::IsReservedIPAddress(
      "[FEC0:ba98:7654:3210:FEDC:BA98:7654:3210]"));

  // Not IP addresses at all.
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("example.com"));
  EXPECT_FALSE(NetworkUtils::IsReservedIPAddress("127.0.0.1.example.com"));

  // Moar IPv4
  for (int i = 0; i < 256; i++) {
    net::IPAddress address(i, 0, 0, 1);
    std::string address_string = address.ToString();
    if (i == 0 || i == 10 || i == 127 || i > 223) {
      EXPECT_TRUE(NetworkUtils::IsReservedIPAddress(
          String::FromUTF8(address_string.data(), address_string.length())));
    } else {
      EXPECT_FALSE(NetworkUtils::IsReservedIPAddress(
          String::FromUTF8(address_string.data(), address_string.length())));
    }
  }
}

TEST(NetworkUtilsTest, GetDomainAndRegistry) {
  EXPECT_EQ("", NetworkUtils::GetDomainAndRegistry(
                    "", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("", NetworkUtils::GetDomainAndRegistry(
                    ".", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("", NetworkUtils::GetDomainAndRegistry(
                    "..", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("", NetworkUtils::GetDomainAndRegistry(
                    "com", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("", NetworkUtils::GetDomainAndRegistry(
                    ".com", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ(
      "", NetworkUtils::GetDomainAndRegistry(
              "www.example.com:8000", NetworkUtils::kIncludePrivateRegistries));

  EXPECT_EQ("", NetworkUtils::GetDomainAndRegistry(
                    "localhost", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("", NetworkUtils::GetDomainAndRegistry(
                    "127.0.0.1", NetworkUtils::kIncludePrivateRegistries));

  EXPECT_EQ("example.com",
            NetworkUtils::GetDomainAndRegistry(
                "example.com", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("example.com",
            NetworkUtils::GetDomainAndRegistry(
                "www.example.com", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("example.com",
            NetworkUtils::GetDomainAndRegistry(
                "static.example.com", NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("example.com", NetworkUtils::GetDomainAndRegistry(
                               "multilevel.www.example.com",
                               NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("example.co.uk",
            NetworkUtils::GetDomainAndRegistry(
                "www.example.co.uk", NetworkUtils::kIncludePrivateRegistries));

  // Verify proper handling of 'private registries'.
  EXPECT_EQ("foo.appspot.com", NetworkUtils::GetDomainAndRegistry(
                                   "www.foo.appspot.com",
                                   NetworkUtils::kIncludePrivateRegistries));
  EXPECT_EQ("appspot.com", NetworkUtils::GetDomainAndRegistry(
                               "www.foo.appspot.com",
                               NetworkUtils::kExcludePrivateRegistries));

  // Verify that unknown registries are included.
  EXPECT_EQ("example.notarealregistry",
            NetworkUtils::GetDomainAndRegistry(
                "www.example.notarealregistry",
                NetworkUtils::kIncludePrivateRegistries));
}

}  // namespace blink
