// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/entity_data.h"

#include "components/sync/base/model_type.h"
#include "components/sync/base/unique_position.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

class EntityDataTest : public testing::Test {
 protected:
  EntityDataTest() {}
  ~EntityDataTest() override {}
};

TEST_F(EntityDataTest, IsDeleted) {
  EntityData data;
  EXPECT_TRUE(data.is_deleted());

  AddDefaultFieldValue(BOOKMARKS, &data.specifics);
  EXPECT_FALSE(data.is_deleted());
}

TEST_F(EntityDataTest, Swap) {
  EntityData data;
  AddDefaultFieldValue(BOOKMARKS, &data.specifics);
  data.id = "id";
  data.server_defined_unique_tag = "server_defined_unique_tag";
  data.client_tag_hash = "client_tag_hash";
  data.non_unique_name = "non_unique_name";
  data.creation_time = base::Time::FromTimeT(10);
  data.modification_time = base::Time::FromTimeT(20);
  data.parent_id = "parent_id";
  data.is_folder = true;

  UniquePosition unique_position =
      UniquePosition::InitialPosition(UniquePosition::RandomSuffix());

  unique_position.ToProto(&data.unique_position);

  // Remember addresses of some data within EntitySpecific and UniquePosition
  // to make sure that the underlying data isn't copied.
  const sync_pb::BookmarkSpecifics* bookmark_specifics =
      &data.specifics.bookmark();
  const std::string* unique_position_value = &data.unique_position.value();

  EntityDataPtr ptr(data.PassToPtr());

  // Compare addresses of the data wrapped by EntityDataPtr to make sure that
  // the underlying objects are exactly the same.
  EXPECT_EQ(bookmark_specifics, &ptr->specifics.bookmark());
  EXPECT_EQ(unique_position_value, &ptr->unique_position.value());

  // Compare other fields.
  EXPECT_EQ("id", ptr->id);
  EXPECT_EQ("server_defined_unique_tag", ptr->server_defined_unique_tag);
  EXPECT_EQ("client_tag_hash", ptr->client_tag_hash);
  EXPECT_EQ("non_unique_name", ptr->non_unique_name);
  EXPECT_EQ("parent_id", ptr->parent_id);
  EXPECT_EQ(base::Time::FromTimeT(10), ptr->creation_time);
  EXPECT_EQ(base::Time::FromTimeT(20), ptr->modification_time);
  EXPECT_EQ(true, ptr->is_folder);
  EXPECT_EQ(false, data.is_folder);
}

}  // namespace syncer
