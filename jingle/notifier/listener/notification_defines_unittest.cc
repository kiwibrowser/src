// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/strings/string_util.h"
#include "jingle/notifier/listener/notification_defines.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace notifier {
namespace {

class NotificationTest : public testing::Test {};

// Create a notification with binary data in the data field.
// Converting it to string shouldn't cause a crash.
TEST_F(NotificationTest, BinaryData) {
  const char kNonUtf8Data[] = { '\xff', '\0' };
  EXPECT_FALSE(base::IsStringUTF8(kNonUtf8Data));
  Notification notification;
  notification.data = kNonUtf8Data;
  EXPECT_EQ("{ channel: \"\", data: \"\\u00FF\" }", notification.ToString());
}

}  // namespace
}  // namespace notifier
