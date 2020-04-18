// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/syncable_enum_conversions.h"

#include "base/logging.h"

namespace syncer {
namespace syncable {

// We can't tokenize expected_min/expected_max since it can be a
// general expression.
#define ASSERT_ENUM_BOUNDS(enum_min, enum_max, expected_min, expected_max)    \
  static_assert(static_cast<int>(enum_min) == static_cast<int>(expected_min), \
                #enum_min " not " #expected_min);                             \
  static_assert(static_cast<int>(enum_max) == static_cast<int>(expected_max), \
                #enum_max " not " #expected_max);

#define ENUM_CASE(enum_value) \
  case enum_value:            \
    return #enum_value

const char* GetMetahandleFieldString(MetahandleField metahandle_field) {
  ASSERT_ENUM_BOUNDS(META_HANDLE, META_HANDLE, INT64_FIELDS_BEGIN,
                     BASE_VERSION - 1);
  switch (metahandle_field) { ENUM_CASE(META_HANDLE); }
  NOTREACHED();
  return "";
}

const char* GetBaseVersionString(BaseVersion base_version) {
  ASSERT_ENUM_BOUNDS(BASE_VERSION, BASE_VERSION, META_HANDLE + 1,
                     SERVER_VERSION - 1);
  switch (base_version) { ENUM_CASE(BASE_VERSION); }
  NOTREACHED();
  return "";
}

const char* GetInt64FieldString(Int64Field int64_field) {
  ASSERT_ENUM_BOUNDS(SERVER_VERSION, TRANSACTION_VERSION, BASE_VERSION + 1,
                     INT64_FIELDS_END - 1);
  switch (int64_field) {
    ENUM_CASE(SERVER_VERSION);
    ENUM_CASE(LOCAL_EXTERNAL_ID);
    ENUM_CASE(TRANSACTION_VERSION);
    case INT64_FIELDS_END:
      break;
  }
  NOTREACHED();
  return "";
}

const char* GetTimeFieldString(TimeField time_field) {
  ASSERT_ENUM_BOUNDS(MTIME, SERVER_CTIME, TIME_FIELDS_BEGIN,
                     TIME_FIELDS_END - 1);
  switch (time_field) {
    ENUM_CASE(MTIME);
    ENUM_CASE(SERVER_MTIME);
    ENUM_CASE(CTIME);
    ENUM_CASE(SERVER_CTIME);
    case TIME_FIELDS_END:
      break;
  }
  NOTREACHED();
  return "";
}

const char* GetIdFieldString(IdField id_field) {
  ASSERT_ENUM_BOUNDS(ID, SERVER_PARENT_ID, ID_FIELDS_BEGIN, ID_FIELDS_END - 1);
  switch (id_field) {
    ENUM_CASE(ID);
    ENUM_CASE(PARENT_ID);
    ENUM_CASE(SERVER_PARENT_ID);
    case ID_FIELDS_END:
      break;
  }
  NOTREACHED();
  return "";
}

const char* GetIndexedBitFieldString(IndexedBitField indexed_bit_field) {
  ASSERT_ENUM_BOUNDS(IS_UNSYNCED, IS_UNAPPLIED_UPDATE, BIT_FIELDS_BEGIN,
                     INDEXED_BIT_FIELDS_END - 1);
  switch (indexed_bit_field) {
    ENUM_CASE(IS_UNSYNCED);
    ENUM_CASE(IS_UNAPPLIED_UPDATE);
    case INDEXED_BIT_FIELDS_END:
      break;
  }
  NOTREACHED();
  return "";
}

const char* GetIsDelFieldString(IsDelField is_del_field) {
  ASSERT_ENUM_BOUNDS(IS_DEL, IS_DEL, INDEXED_BIT_FIELDS_END, IS_DIR - 1);
  switch (is_del_field) { ENUM_CASE(IS_DEL); }
  NOTREACHED();
  return "";
}

const char* GetBitFieldString(BitField bit_field) {
  ASSERT_ENUM_BOUNDS(IS_DIR, SERVER_IS_DEL, IS_DEL + 1, BIT_FIELDS_END - 1);
  switch (bit_field) {
    ENUM_CASE(IS_DIR);
    ENUM_CASE(SERVER_IS_DIR);
    ENUM_CASE(SERVER_IS_DEL);
    case BIT_FIELDS_END:
      break;
  }
  NOTREACHED();
  return "";
}

const char* GetStringFieldString(StringField string_field) {
  ASSERT_ENUM_BOUNDS(NON_UNIQUE_NAME, UNIQUE_BOOKMARK_TAG, STRING_FIELDS_BEGIN,
                     STRING_FIELDS_END - 1);
  switch (string_field) {
    ENUM_CASE(NON_UNIQUE_NAME);
    ENUM_CASE(SERVER_NON_UNIQUE_NAME);
    ENUM_CASE(UNIQUE_SERVER_TAG);
    ENUM_CASE(UNIQUE_CLIENT_TAG);
    ENUM_CASE(UNIQUE_BOOKMARK_TAG);
    case STRING_FIELDS_END:
      break;
  }
  NOTREACHED();
  return "";
}

const char* GetProtoFieldString(ProtoField proto_field) {
  ASSERT_ENUM_BOUNDS(SPECIFICS, BASE_SERVER_SPECIFICS, PROTO_FIELDS_BEGIN,
                     PROTO_FIELDS_END - 1);
  switch (proto_field) {
    ENUM_CASE(SPECIFICS);
    ENUM_CASE(SERVER_SPECIFICS);
    ENUM_CASE(BASE_SERVER_SPECIFICS);
    case PROTO_FIELDS_END:
      break;
  }
  NOTREACHED();
  return "";
}

const char* GetUniquePositionFieldString(UniquePositionField position_field) {
  ASSERT_ENUM_BOUNDS(SERVER_UNIQUE_POSITION, UNIQUE_POSITION,
                     UNIQUE_POSITION_FIELDS_BEGIN,
                     UNIQUE_POSITION_FIELDS_END - 1);
  switch (position_field) {
    ENUM_CASE(SERVER_UNIQUE_POSITION);
    ENUM_CASE(UNIQUE_POSITION);
    case UNIQUE_POSITION_FIELDS_END:
      break;
  }
  NOTREACHED();
  return "";
}

const char* GetBitTempString(BitTemp bit_temp) {
  ASSERT_ENUM_BOUNDS(SYNCING, DIRTY_SYNC, BIT_TEMPS_BEGIN, BIT_TEMPS_END - 1);
  switch (bit_temp) {
    ENUM_CASE(SYNCING);
    ENUM_CASE(DIRTY_SYNC);
    case BIT_TEMPS_END:
      break;
  }
  NOTREACHED();
  return "";
}

#undef ENUM_CASE
#undef ASSERT_ENUM_BOUNDS

}  // namespace syncable
}  // namespace syncer
