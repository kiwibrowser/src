// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/battor_agent/battor_protocol_types.h"

#include <iostream>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/battor_agent/serial_utils.h"

using namespace testing;

namespace battor {

namespace {

const BattOrEEPROM kUnserializedEEPROM{
    {0, 0, 0, 1}, 2,  "serialno", 3,  4,  5,  6,  7,  8,  9,  10, 11,
    12,           13, 14,         15, 16, 17, 18, 19, 20, 21, 22, 24,
};

// The serialized version of the above EEPROM.
const unsigned char kSerializedEEPROM[] = {
    0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x73, 0x65, 0x72, 0x69, 0x61, 0x6c,
    0x6e, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00,
    0xa0, 0x40, 0x00, 0x00, 0xc0, 0x40, 0x00, 0x00, 0xe0, 0x40, 0x00, 0x00,
    0x00, 0x41, 0x00, 0x00, 0x10, 0x41, 0x0a, 0x00, 0x00, 0x00, 0x30, 0x41,
    0x00, 0x00, 0x40, 0x41, 0x00, 0x00, 0x50, 0x41, 0x0e, 0x00, 0x0f, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x11, 0x00, 0x12, 0x00, 0x13, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x15, 0x00, 0x16, 0x00, 0x18, 0x00, 0x00, 0x00,
};

}  // namespace

TEST(BattOrProtocolTypeTest, EEPROMSerializesCorrectly) {
  // The easier way to write this test would be using memcmp. However, because
  // the EEPROM will change in the future and we'll need to update the
  // serialized version when it does, it makes sense to print the bytes as a
  // string that can just be copied and pasted into kSerializedEEPROM.
  const char* eeprom_bytes =
      reinterpret_cast<const char*>(&kUnserializedEEPROM);

  ASSERT_EQ(CharArrayToString(reinterpret_cast<const char*>(kSerializedEEPROM),
                              sizeof(kSerializedEEPROM)),
            CharArrayToString(eeprom_bytes, sizeof(kUnserializedEEPROM)));
}

}  // namespace battor
