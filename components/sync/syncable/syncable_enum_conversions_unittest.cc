// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/syncable_enum_conversions.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace syncable {
namespace {

// Keep this file in sync with entry_kernel.h.
class SyncableEnumConversionsTest : public testing::Test {};

template <class T>
void TestEnumStringFunction(const char* (*enum_string_fn)(T),
                            int enum_min,
                            int enum_max) {
  EXPECT_LE(enum_min, enum_max);
  for (int i = enum_min; i <= enum_max; ++i) {
    const std::string& str = enum_string_fn(static_cast<T>(i));
    EXPECT_FALSE(str.empty());
  }
}

TEST_F(SyncableEnumConversionsTest, GetMetahandleFieldString) {
  TestEnumStringFunction(GetMetahandleFieldString, INT64_FIELDS_BEGIN,
                         META_HANDLE);
}

TEST_F(SyncableEnumConversionsTest, GetBaseVersionString) {
  TestEnumStringFunction(GetBaseVersionString, META_HANDLE + 1, BASE_VERSION);
}

TEST_F(SyncableEnumConversionsTest, GetInt64FieldString) {
  TestEnumStringFunction(GetInt64FieldString, BASE_VERSION + 1,
                         INT64_FIELDS_END - 1);
}

TEST_F(SyncableEnumConversionsTest, GetTimeFieldString) {
  TestEnumStringFunction(GetTimeFieldString, TIME_FIELDS_BEGIN,
                         TIME_FIELDS_END - 1);
}

TEST_F(SyncableEnumConversionsTest, GetIdFieldString) {
  TestEnumStringFunction(GetIdFieldString, ID_FIELDS_BEGIN, ID_FIELDS_END - 1);
}

TEST_F(SyncableEnumConversionsTest, GetIndexedBitFieldString) {
  TestEnumStringFunction(GetIndexedBitFieldString, BIT_FIELDS_BEGIN,
                         INDEXED_BIT_FIELDS_END - 1);
}

TEST_F(SyncableEnumConversionsTest, GetIsDelFieldString) {
  TestEnumStringFunction(GetIsDelFieldString, INDEXED_BIT_FIELDS_END, IS_DEL);
}

TEST_F(SyncableEnumConversionsTest, GetBitFieldString) {
  TestEnumStringFunction(GetBitFieldString, IS_DEL + 1, BIT_FIELDS_END - 1);
}

TEST_F(SyncableEnumConversionsTest, GetStringFieldString) {
  TestEnumStringFunction(GetStringFieldString, STRING_FIELDS_BEGIN,
                         STRING_FIELDS_END - 1);
}

TEST_F(SyncableEnumConversionsTest, GetProtoFieldString) {
  TestEnumStringFunction(GetProtoFieldString, PROTO_FIELDS_BEGIN,
                         PROTO_FIELDS_END - 1);
}

TEST_F(SyncableEnumConversionsTest, GetUniquePositionFieldString) {
  TestEnumStringFunction(GetUniquePositionFieldString,
                         UNIQUE_POSITION_FIELDS_BEGIN,
                         UNIQUE_POSITION_FIELDS_END - 1);
}

TEST_F(SyncableEnumConversionsTest, GetBitTempString) {
  TestEnumStringFunction(GetBitTempString, BIT_TEMPS_BEGIN, BIT_TEMPS_END - 1);
}

}  // namespace
}  // namespace syncable
}  // namespace syncer
