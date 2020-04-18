// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_message_util.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cast_channel {

TEST(CastMessageUtilTest, IsCastInternalNamespace) {
  EXPECT_TRUE(IsCastInternalNamespace("urn:x-cast:com.google.cast.receiver"));
  EXPECT_FALSE(IsCastInternalNamespace("urn:x-cast:com.google.youtube"));
  EXPECT_FALSE(IsCastInternalNamespace("urn:x-cast:com.foo"));
  EXPECT_FALSE(IsCastInternalNamespace("foo"));
  EXPECT_FALSE(IsCastInternalNamespace(""));
}

TEST(CastMessageUtilTest, CastMessageType) {
  for (int i = 0; i < static_cast<int>(CastMessageType::kOther); ++i) {
    CastMessageType type = static_cast<CastMessageType>(i);
    EXPECT_EQ(type, CastMessageTypeFromString(CastMessageTypeToString(type)));
  }
}

TEST(CastMessageUtilTest, GetLaunchSessionResponseOk) {
  std::string payload = R"(
    {
      "type": "RECEIVER_STATUS",
      "requestId": 123,
      "status": {}
    }
  )";

  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(base::JSONReader::Read(payload));
  ASSERT_TRUE(value);

  LaunchSessionResponse response = GetLaunchSessionResponse(*value);
  EXPECT_EQ(LaunchSessionResponse::Result::kOk, response.result);
  EXPECT_TRUE(response.receiver_status);
}

TEST(CastMessageUtilTest, GetLaunchSessionResponseError) {
  std::string payload = R"(
    {
      "type": "LAUNCH_ERROR",
      "requestId": 123
    }
  )";

  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(base::JSONReader::Read(payload));
  ASSERT_TRUE(value);

  LaunchSessionResponse response = GetLaunchSessionResponse(*value);
  EXPECT_EQ(LaunchSessionResponse::Result::kError, response.result);
  EXPECT_FALSE(response.receiver_status);
}

TEST(CastMessageUtilTest, GetLaunchSessionResponseUnknown) {
  // Unrelated type.
  std::string payload = R"(
    {
      "type": "APPLICATION_BROADCAST",
      "requestId": 123,
      "status": {}
    }
  )";

  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(base::JSONReader::Read(payload));
  ASSERT_TRUE(value);

  LaunchSessionResponse response = GetLaunchSessionResponse(*value);
  EXPECT_EQ(LaunchSessionResponse::Result::kUnknown, response.result);
  EXPECT_FALSE(response.receiver_status);
}

}  // namespace cast_channel
