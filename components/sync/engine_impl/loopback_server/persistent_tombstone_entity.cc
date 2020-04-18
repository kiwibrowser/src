// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/loopback_server/persistent_tombstone_entity.h"

#include "base/memory/ptr_util.h"

using std::string;

using syncer::ModelType;

namespace syncer {

PersistentTombstoneEntity::~PersistentTombstoneEntity() {}

// static
std::unique_ptr<LoopbackServerEntity>
PersistentTombstoneEntity::CreateFromEntity(const sync_pb::SyncEntity& entity) {
  return CreateNewInternal(entity.id_string(), entity.version(),
                           entity.client_defined_unique_tag());
}

// static
std::unique_ptr<LoopbackServerEntity> PersistentTombstoneEntity::CreateNew(
    const std::string& id,
    const std::string& client_defined_unique_tag) {
  return CreateNewInternal(id, 0, client_defined_unique_tag);
}

// static
std::unique_ptr<LoopbackServerEntity>
PersistentTombstoneEntity::CreateNewInternal(
    const std::string& id,
    int64_t version,
    const std::string& client_defined_unique_tag) {
  const ModelType model_type = LoopbackServerEntity::GetModelTypeFromId(id);
  if (model_type == syncer::UNSPECIFIED) {
    DLOG(WARNING) << "Invalid ID was given: " << id;
    return nullptr;
  }

  return base::WrapUnique(new PersistentTombstoneEntity(
      id, version, model_type, client_defined_unique_tag));
}

PersistentTombstoneEntity::PersistentTombstoneEntity(
    const string& id,
    int64_t version,
    const ModelType& model_type,
    const std::string& client_defined_unique_tag)
    : LoopbackServerEntity(id, model_type, version, string()),
      client_defined_unique_tag_(client_defined_unique_tag) {
  sync_pb::EntitySpecifics specifics;
  AddDefaultFieldValue(model_type, &specifics);
  SetSpecifics(specifics);
}

bool PersistentTombstoneEntity::RequiresParentId() const {
  return false;
}

string PersistentTombstoneEntity::GetParentId() const {
  return string();
}

void PersistentTombstoneEntity::SerializeAsProto(
    sync_pb::SyncEntity* proto) const {
  LoopbackServerEntity::SerializeBaseProtoFields(proto);
  if (!client_defined_unique_tag_.empty())
    proto->set_client_defined_unique_tag(client_defined_unique_tag_);
}

bool PersistentTombstoneEntity::IsDeleted() const {
  return true;
}

sync_pb::LoopbackServerEntity_Type
PersistentTombstoneEntity::GetLoopbackServerEntityType() const {
  return sync_pb::LoopbackServerEntity_Type_TOMBSTONE;
}

}  // namespace syncer
