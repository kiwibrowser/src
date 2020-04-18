// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/push_messaging/push_manager.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/push_messaging/web_push_subscription_options.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/modules/push_messaging/push_subscription_options.h"
#include "third_party/blink/renderer/modules/push_messaging/push_subscription_options_init.h"

namespace blink {
namespace {

const unsigned kMaxKeyLength = 255;

// NIST P-256 public key made available to tests. Must be an uncompressed
// point in accordance with SEC1 2.3.3.
const unsigned kApplicationServerKeyLength = 65;
const uint8_t kApplicationServerKey[kApplicationServerKeyLength] = {
    0x04, 0x55, 0x52, 0x6A, 0xA5, 0x6E, 0x8E, 0xAA, 0x47, 0x97, 0x36,
    0x10, 0xC1, 0x66, 0x3C, 0x1E, 0x65, 0xBF, 0xA1, 0x7B, 0xEE, 0x48,
    0xC9, 0xC6, 0xBB, 0xBF, 0x02, 0x18, 0x53, 0x72, 0x1D, 0x0C, 0x7B,
    0xA9, 0xE3, 0x11, 0xB7, 0x03, 0x52, 0x21, 0xD3, 0x71, 0x90, 0x13,
    0xA8, 0xC1, 0xCF, 0xED, 0x20, 0xF7, 0x1F, 0xD1, 0x7F, 0xF2, 0x76,
    0xB6, 0x01, 0x20, 0xD8, 0x35, 0xA5, 0xD9, 0x3C, 0x43, 0xDF};

TEST(PushManagerTest, ValidSenderKey) {
  PushSubscriptionOptionsInit options;
  options.setApplicationServerKey(
      ArrayBufferOrArrayBufferView::FromArrayBuffer(DOMArrayBuffer::Create(
          kApplicationServerKey, kApplicationServerKeyLength)));

  DummyExceptionStateForTesting exception_state;
  WebPushSubscriptionOptions output =
      PushSubscriptionOptions::ToWeb(options, exception_state);
  EXPECT_FALSE(exception_state.HadException());
  EXPECT_EQ(output.application_server_key.length(),
            kApplicationServerKeyLength);

  // Copy the key into a size+1 buffer so that it can be treated as a null
  // terminated string for the purposes of EXPECT_EQ.
  uint8_t sender_key[kApplicationServerKeyLength + 1];
  for (unsigned i = 0; i < kApplicationServerKeyLength; i++)
    sender_key[i] = kApplicationServerKey[i];
  sender_key[kApplicationServerKeyLength] = 0x0;
  EXPECT_EQ(reinterpret_cast<const char*>(sender_key),
            output.application_server_key.Latin1());
  EXPECT_FALSE(output.application_server_key.IsEmpty());
}

TEST(PushManagerTest, InvalidSenderKeyLength) {
  uint8_t sender_key[kMaxKeyLength + 1];
  memset(sender_key, 0, sizeof(sender_key));
  PushSubscriptionOptionsInit options;
  options.setApplicationServerKey(ArrayBufferOrArrayBufferView::FromArrayBuffer(
      DOMArrayBuffer::Create(sender_key, kMaxKeyLength + 1)));

  DummyExceptionStateForTesting exception_state;
  WebPushSubscriptionOptions output =
      PushSubscriptionOptions::ToWeb(options, exception_state);
  EXPECT_TRUE(exception_state.HadException());
}

}  // namespace
}  // namespace blink
