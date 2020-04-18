// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/non_blocking_type_commit_contribution.h"

#include <string>

#include "base/base64.h"
#include "base/sha1.h"
#include "components/sync/base/hash_util.h"
#include "components/sync/base/model_type.h"
#include "components/sync/base/unique_position.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

using sync_pb::EntitySpecifics;
using sync_pb::SyncEntity;

const char kTag[] = "tag";
const char kValue[] = "value";
const char kURL[] = "url";
const char kTitle[] = "title";

EntitySpecifics GeneratePreferenceSpecifics(const std::string& tag,
                                            const std::string& value) {
  EntitySpecifics specifics;
  specifics.mutable_preference()->set_name(tag);
  specifics.mutable_preference()->set_value(value);
  return specifics;
}

EntitySpecifics GenerateBookmarkSpecifics(const std::string& url,
                                          const std::string& title) {
  EntitySpecifics specifics;
  specifics.mutable_bookmark()->set_url(url);
  specifics.mutable_bookmark()->set_title(title);
  return specifics;
}

TEST(NonBlockingTypeCommitContribution, PopulateCommitProtoNonBookmark) {
  const int64_t kBaseVersion = 7;
  base::Time creation_time =
      base::Time::UnixEpoch() + base::TimeDelta::FromDays(1);
  base::Time modification_time =
      creation_time + base::TimeDelta::FromSeconds(1);

  EntityData data;

  data.client_tag_hash = kTag;
  data.specifics = GeneratePreferenceSpecifics(kTag, kValue);

  // These fields are not really used for much, but we set them anyway
  // to make this item look more realistic.
  data.creation_time = creation_time;
  data.modification_time = modification_time;
  data.non_unique_name = "Name:";

  CommitRequestData request_data;
  request_data.entity = data.PassToPtr();
  request_data.sequence_number = 2;
  request_data.base_version = kBaseVersion;
  base::Base64Encode(base::SHA1HashString(data.specifics.SerializeAsString()),
                     &request_data.specifics_hash);

  SyncEntity entity;
  NonBlockingTypeCommitContribution::PopulateCommitProto(request_data, &entity);

  // Exhaustively verify the populated SyncEntity.
  EXPECT_TRUE(entity.id_string().empty());
  EXPECT_EQ(7, entity.version());
  EXPECT_EQ(modification_time.ToJsTime(), entity.mtime());
  EXPECT_EQ(creation_time.ToJsTime(), entity.ctime());
  EXPECT_FALSE(entity.name().empty());
  EXPECT_FALSE(entity.client_defined_unique_tag().empty());
  EXPECT_EQ(kTag, entity.specifics().preference().name());
  EXPECT_FALSE(entity.deleted());
  EXPECT_EQ(kValue, entity.specifics().preference().value());
  EXPECT_TRUE(entity.parent_id_string().empty());
  EXPECT_FALSE(entity.unique_position().has_custom_compressed_v1());
  EXPECT_EQ(0, entity.position_in_parent());
}

TEST(NonBlockingTypeCommitContribution, PopulateCommitProtoBookmark) {
  const int64_t kBaseVersion = 7;
  base::Time creation_time =
      base::Time::UnixEpoch() + base::TimeDelta::FromDays(1);
  base::Time modification_time =
      creation_time + base::TimeDelta::FromSeconds(1);

  EntityData data;

  data.id = "bookmark";
  data.specifics = GenerateBookmarkSpecifics(kURL, kTitle);

  // These fields are not really used for much, but we set them anyway
  // to make this item look more realistic.
  data.creation_time = creation_time;
  data.modification_time = modification_time;
  data.non_unique_name = "Name:";
  data.parent_id = "ParentOf:";
  data.is_folder = true;
  syncer::UniquePosition uniquePosition = syncer::UniquePosition::FromInt64(
      10, syncer::UniquePosition::RandomSuffix());
  uniquePosition.ToProto(&data.unique_position);

  CommitRequestData request_data;
  request_data.entity = data.PassToPtr();
  request_data.sequence_number = 2;
  request_data.base_version = kBaseVersion;
  base::Base64Encode(base::SHA1HashString(data.specifics.SerializeAsString()),
                     &request_data.specifics_hash);

  SyncEntity entity;
  NonBlockingTypeCommitContribution::PopulateCommitProto(request_data, &entity);

  // Exhaustively verify the populated SyncEntity.
  EXPECT_FALSE(entity.id_string().empty());
  EXPECT_EQ(7, entity.version());
  EXPECT_EQ(modification_time.ToJsTime(), entity.mtime());
  EXPECT_EQ(creation_time.ToJsTime(), entity.ctime());
  EXPECT_FALSE(entity.name().empty());
  EXPECT_TRUE(entity.client_defined_unique_tag().empty());
  EXPECT_EQ(kURL, entity.specifics().bookmark().url());
  EXPECT_FALSE(entity.deleted());
  EXPECT_EQ(kTitle, entity.specifics().bookmark().title());
  EXPECT_TRUE(entity.folder());
  EXPECT_FALSE(entity.parent_id_string().empty());
  EXPECT_TRUE(entity.unique_position().has_custom_compressed_v1());
  EXPECT_NE(0, entity.position_in_parent());
}

}  // namespace

}  // namespace syncer
