// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_ENUM_CONVERSIONS_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_ENUM_CONVERSIONS_H_

#include "components/sync/syncable/entry_kernel.h"

// Keep this file in sync with entry_kernel.h.
//
// Utility functions to get the string equivalent for some syncable
// enums.

namespace syncer {
namespace syncable {

// The returned strings (which don't have to be freed) are in ASCII.
// The result of passing in an invalid enum value is undefined.

const char* GetMetahandleFieldString(MetahandleField metahandle_field);

const char* GetBaseVersionString(BaseVersion base_version);

const char* GetInt64FieldString(Int64Field int64_field);

const char* GetTimeFieldString(TimeField time_field);

const char* GetIdFieldString(IdField id_field);

const char* GetIndexedBitFieldString(IndexedBitField indexed_bit_field);

const char* GetIsDelFieldString(IsDelField is_del_field);

const char* GetBitFieldString(BitField bit_field);

const char* GetStringFieldString(StringField string_field);

const char* GetProtoFieldString(ProtoField proto_field);

const char* GetUniquePositionFieldString(UniquePositionField position_field);

const char* GetBitTempString(BitTemp bit_temp);

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_ENUM_CONVERSIONS_H_
