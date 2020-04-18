// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/sync_data.h"

#include <algorithm>
#include <ostream>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/sync/protocol/proto_value_conversions.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/syncable/base_node.h"

namespace syncer {

void SyncData::ImmutableSyncEntityTraits::InitializeWrapper(Wrapper* wrapper) {
  *wrapper = new sync_pb::SyncEntity();
}

void SyncData::ImmutableSyncEntityTraits::DestroyWrapper(Wrapper* wrapper) {
  delete *wrapper;
}

const sync_pb::SyncEntity& SyncData::ImmutableSyncEntityTraits::Unwrap(
    const Wrapper& wrapper) {
  return *wrapper;
}

sync_pb::SyncEntity* SyncData::ImmutableSyncEntityTraits::UnwrapMutable(
    Wrapper* wrapper) {
  return *wrapper;
}

void SyncData::ImmutableSyncEntityTraits::Swap(sync_pb::SyncEntity* t1,
                                               sync_pb::SyncEntity* t2) {
  t1->Swap(t2);
}

SyncData::SyncData() : id_(kInvalidId), is_valid_(false) {}

SyncData::SyncData(int64_t id,
                   sync_pb::SyncEntity* entity,
                   const base::Time& remote_modification_time)
    : id_(id),
      remote_modification_time_(remote_modification_time),
      immutable_entity_(entity),
      is_valid_(true) {}

SyncData::SyncData(const SyncData& other) = default;

SyncData::~SyncData() {}

// Static.
SyncData SyncData::CreateLocalDelete(const std::string& sync_tag,
                                     ModelType datatype) {
  sync_pb::EntitySpecifics specifics;
  AddDefaultFieldValue(datatype, &specifics);
  return CreateLocalData(sync_tag, std::string(), specifics);
}

// Static.
SyncData SyncData::CreateLocalData(const std::string& sync_tag,
                                   const std::string& non_unique_title,
                                   const sync_pb::EntitySpecifics& specifics) {
  sync_pb::SyncEntity entity;
  entity.set_client_defined_unique_tag(sync_tag);
  entity.set_non_unique_name(non_unique_title);
  entity.mutable_specifics()->CopyFrom(specifics);
  return SyncData(kInvalidId, &entity, base::Time());
}

// Static.
SyncData SyncData::CreateRemoteData(
    int64_t id,
    const sync_pb::EntitySpecifics& specifics,
    const base::Time& modification_time,
    const std::string& client_tag_hash) {
  DCHECK_NE(id, kInvalidId);
  sync_pb::SyncEntity entity;
  entity.mutable_specifics()->CopyFrom(specifics);
  entity.set_client_defined_unique_tag(client_tag_hash);
  return SyncData(id, &entity, modification_time);
}

bool SyncData::IsValid() const {
  return is_valid_;
}

const sync_pb::EntitySpecifics& SyncData::GetSpecifics() const {
  return immutable_entity_.Get().specifics();
}

ModelType SyncData::GetDataType() const {
  return GetModelTypeFromSpecifics(GetSpecifics());
}

const std::string& SyncData::GetTitle() const {
  // TODO(zea): set this for data coming from the syncer too.
  DCHECK(immutable_entity_.Get().has_non_unique_name());
  return immutable_entity_.Get().non_unique_name();
}

bool SyncData::IsLocal() const {
  return id_ == kInvalidId;
}

std::string SyncData::ToString() const {
  if (!IsValid())
    return "<Invalid SyncData>";

  std::string type = ModelTypeToString(GetDataType());
  std::string specifics;
  base::JSONWriter::WriteWithOptions(*EntitySpecificsToValue(GetSpecifics()),
                                     base::JSONWriter::OPTIONS_PRETTY_PRINT,
                                     &specifics);

  if (IsLocal()) {
    SyncDataLocal sync_data_local(*this);
    return "{ isLocal: true, type: " + type + ", tag: " +
           sync_data_local.GetTag() + ", title: " + GetTitle() +
           ", specifics: " + specifics + "}";
  }

  SyncDataRemote sync_data_remote(*this);
  std::string id = base::Int64ToString(sync_data_remote.GetId());
  return "{ isLocal: false, type: " + type + ", specifics: " + specifics +
         ", id: " + id + "}";
}

void PrintTo(const SyncData& sync_data, std::ostream* os) {
  *os << sync_data.ToString();
}

SyncDataLocal::SyncDataLocal(const SyncData& sync_data) : SyncData(sync_data) {
  DCHECK(sync_data.IsLocal());
}

SyncDataLocal::~SyncDataLocal() {}

const std::string& SyncDataLocal::GetTag() const {
  return immutable_entity_.Get().client_defined_unique_tag();
}

SyncDataRemote::SyncDataRemote(const SyncData& sync_data)
    : SyncData(sync_data) {
  DCHECK(!sync_data.IsLocal());
}

SyncDataRemote::~SyncDataRemote() {}

const base::Time& SyncDataRemote::GetModifiedTime() const {
  return remote_modification_time_;
}

int64_t SyncDataRemote::GetId() const {
  return id_;
}

const std::string& SyncDataRemote::GetClientTagHash() const {
  // It seems that client_defined_unique_tag has a bit of an overloaded use,
  // holding onto the un-hashed tag while local, and then the hashed value when
  // communicating with the server. This usage is copying the latter of these
  // cases, where this is the hashed tag value. The original tag is not sent to
  // the server so we wouldn't be able to set this value anyways. The only way
  // to recreate an un-hashed tag is for the service to do so with a specifics.
  // Should only be used by sessions, see crbug.com/604657.
  DCHECK_EQ(SESSIONS, GetDataType());
  return immutable_entity_.Get().client_defined_unique_tag();
}

}  // namespace syncer
