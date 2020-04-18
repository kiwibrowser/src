// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_LOOPBACK_SERVER_PERSISTENT_UNIQUE_CLIENT_ENTITY_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_LOOPBACK_SERVER_PERSISTENT_UNIQUE_CLIENT_ENTITY_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "components/sync/base/model_type.h"
#include "components/sync/engine_impl/loopback_server/loopback_server_entity.h"

namespace sync_pb {
class EntitySpecifics;
class SyncEntity;
}  // namespace sync_pb

namespace syncer {

// An entity that is unique per client account.
//
// TODO(pvalenzuela): Reconsider the naming of this class. In some cases, this
// type is used to represent entities where the unique client tag is irrelevant
// (e.g., Autofill Wallet).
class PersistentUniqueClientEntity : public LoopbackServerEntity {
 public:
  PersistentUniqueClientEntity(const std::string& id,
                               syncer::ModelType model_type,
                               int64_t version,
                               const std::string& name,
                               const std::string& client_defined_unique_tag,
                               const sync_pb::EntitySpecifics& specifics,
                               int64_t creation_time,
                               int64_t last_modified_time);

  ~PersistentUniqueClientEntity() override;

  // Factory function for creating a PersistentUniqueClientEntity.
  static std::unique_ptr<LoopbackServerEntity> CreateFromEntity(
      const sync_pb::SyncEntity& client_entity);

  // Factory function for creating a PersistentUniqueClientEntity for use in the
  // FakeServer injection API.
  static std::unique_ptr<LoopbackServerEntity> CreateFromEntitySpecifics(
      const std::string& name,
      const sync_pb::EntitySpecifics& entity_specifics,
      int64_t creation_time,
      int64_t last_modified_time);

  // LoopbackServerEntity implementation.
  bool RequiresParentId() const override;
  std::string GetParentId() const override;
  void SerializeAsProto(sync_pb::SyncEntity* proto) const override;
  sync_pb::LoopbackServerEntity_Type GetLoopbackServerEntityType()
      const override;

 private:
  // These member values have equivalent fields in SyncEntity.
  std::string client_defined_unique_tag_;
  int64_t creation_time_;
  int64_t last_modified_time_;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_LOOPBACK_SERVER_PERSISTENT_UNIQUE_CLIENT_ENTITY_H_
