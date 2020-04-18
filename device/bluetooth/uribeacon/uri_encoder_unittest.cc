// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "device/bluetooth/uribeacon/uri_encoder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

TEST(UriEncoderTest, Url1) {
  const std::string kUri = "http://123.com";
  const char encoded_uri[] = {2, '1', '2', '3', 7};
  const std::vector<uint8_t> kEncodedUri(encoded_uri,
                                         encoded_uri + sizeof(encoded_uri));

  std::vector<uint8_t> encoded;
  std::string decoded;

  EncodeUriBeaconUri(kUri, encoded);
  EXPECT_EQ(kEncodedUri, encoded);

  DecodeUriBeaconUri(encoded, decoded);
  EXPECT_EQ(kUri, decoded);
}

TEST(UriEncoderTest, Url2) {
  const std::string kUri = "http://www.abcdefghijklmnop.org";
  const char encoded_uri[] = {
      0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
      'o', 'p', 8};
  const std::vector<uint8_t> kEncodedUri(encoded_uri,
                                         encoded_uri + sizeof(encoded_uri));

  std::vector<uint8_t> encoded;
  std::string decoded;

  EncodeUriBeaconUri(kUri, encoded);
  EXPECT_EQ(kEncodedUri, encoded);

  DecodeUriBeaconUri(encoded, decoded);
  EXPECT_EQ(kUri, decoded);
}

TEST(UriEncoderTest, Url3) {
  const std::string kUri = "https://123.com/123";
  const char encoded_uri[] = {3, '1', '2', '3', 0, '1', '2', '3'};
  const std::vector<uint8_t> kEncodedUri(encoded_uri,
                                         encoded_uri + sizeof(encoded_uri));

  std::vector<uint8_t> encoded;
  std::string decoded;

  EncodeUriBeaconUri(kUri, encoded);
  EXPECT_EQ(kEncodedUri, encoded);

  DecodeUriBeaconUri(encoded, decoded);
  EXPECT_EQ(kUri, decoded);
}

TEST(UriEncoderTest, Url4) {
  const std::string kUri = "http://www.uribeacon.org";
  const char encoded_uri[] = {
      0, 'u', 'r', 'i', 'b', 'e', 'a', 'c', 'o', 'n', 8};
  const std::vector<uint8_t> kEncodedUri(encoded_uri,
                                         encoded_uri + sizeof(encoded_uri));

  std::vector<uint8_t> encoded;
  std::string decoded;

  EncodeUriBeaconUri(kUri, encoded);
  EXPECT_EQ(kEncodedUri, encoded);

  DecodeUriBeaconUri(encoded, decoded);
  EXPECT_EQ(kUri, decoded);
}

TEST(UriEncoderTest, Url5) {
  const std::string kUri = "http://web.mit.edu/";
  const char encoded_uri[] = {2, 'w', 'e', 'b', '.', 'm', 'i', 't', 2};
  const std::vector<uint8_t> kEncodedUri(encoded_uri,
                                         encoded_uri + sizeof(encoded_uri));

  std::vector<uint8_t> encoded;
  std::string decoded;

  EncodeUriBeaconUri(kUri, encoded);
  EXPECT_EQ(kEncodedUri, encoded);

  DecodeUriBeaconUri(encoded, decoded);
  EXPECT_EQ(kUri, decoded);
}

}  // namespace device
