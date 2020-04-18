// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/loopback_server/persistent_unique_client_entity.h"

#include "components/sync/protocol/sync.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

TEST(PersistentUniqueClientEntityTest, CreateFromEntity) {
  sync_pb::SyncEntity entity;
  entity.mutable_specifics()->mutable_preference();
  // Normal types need a client_defined_unique_tag.
  ASSERT_FALSE(PersistentUniqueClientEntity::CreateFromEntity(entity));

  *entity.mutable_client_defined_unique_tag() = "tag";
  ASSERT_TRUE(PersistentUniqueClientEntity::CreateFromEntity(entity));

  entity.clear_specifics();
  entity.mutable_specifics()->mutable_user_event();
  // CommitOnly type should never have a client_defined_unique_tag.
  ASSERT_FALSE(PersistentUniqueClientEntity::CreateFromEntity(entity));

  entity.clear_client_defined_unique_tag();
  ASSERT_TRUE(PersistentUniqueClientEntity::CreateFromEntity(entity));
}

TEST(PersistentUniqueClientEntityTest, CreateFromEntitySpecifics) {
  sync_pb::EntitySpecifics specifics;
  specifics.mutable_preference();
  ASSERT_TRUE(PersistentUniqueClientEntity::CreateFromEntitySpecifics(
      "name", specifics, 0, 0));
}

}  // namespace

}  // namespace syncer
