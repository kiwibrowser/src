// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/unacked_invalidation_set.h"

#include <stddef.h>

#include <memory>

#include "base/json/json_string_value_serializer.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "components/invalidation/impl/unacked_invalidation_set_test_util.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "components/invalidation/public/single_object_invalidation_set.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

class UnackedInvalidationSetTest : public testing::Test {
 public:
  UnackedInvalidationSetTest()
      : kObjectId_(10, "ASDF"),
        unacked_invalidations_(kObjectId_) {}

  SingleObjectInvalidationSet GetStoredInvalidations() {
    ObjectIdInvalidationMap map;
    unacked_invalidations_.ExportInvalidations(
        base::WeakPtr<AckHandler>(),
        scoped_refptr<base::SingleThreadTaskRunner>(),
        &map);
    ObjectIdSet ids = map.GetObjectIds();
    if (ids.find(kObjectId_) != ids.end()) {
      return map.ForObject(kObjectId_);
    } else {
      return SingleObjectInvalidationSet();
    }
  }

  const invalidation::ObjectId kObjectId_;
  UnackedInvalidationSet unacked_invalidations_;
};

namespace {

// Test storage and retrieval of zero invalidations.
TEST_F(UnackedInvalidationSetTest, Empty) {
  EXPECT_EQ(0U, GetStoredInvalidations().GetSize());
}

// Test storage and retrieval of a single invalidation.
TEST_F(UnackedInvalidationSetTest, OneInvalidation) {
  Invalidation inv1 = Invalidation::Init(kObjectId_, 10, "payload");
  unacked_invalidations_.Add(inv1);

  SingleObjectInvalidationSet set = GetStoredInvalidations();
  ASSERT_EQ(1U, set.GetSize());
  EXPECT_FALSE(set.StartsWithUnknownVersion());
}

// Test that calling Clear() returns us to the empty state.
TEST_F(UnackedInvalidationSetTest, Clear) {
  Invalidation inv1 = Invalidation::Init(kObjectId_, 10, "payload");
  unacked_invalidations_.Add(inv1);
  unacked_invalidations_.Clear();

  EXPECT_EQ(0U, GetStoredInvalidations().GetSize());
}

// Test that repeated unknown version invalidations are squashed together.
TEST_F(UnackedInvalidationSetTest, UnknownVersions) {
  Invalidation inv1 = Invalidation::Init(kObjectId_, 10, "payload");
  Invalidation inv2 = Invalidation::InitUnknownVersion(kObjectId_);
  Invalidation inv3 = Invalidation::InitUnknownVersion(kObjectId_);
  unacked_invalidations_.Add(inv1);
  unacked_invalidations_.Add(inv2);
  unacked_invalidations_.Add(inv3);

  SingleObjectInvalidationSet set = GetStoredInvalidations();
  ASSERT_EQ(2U, set.GetSize());
  EXPECT_TRUE(set.StartsWithUnknownVersion());
}

// Tests that no truncation occurs while we're under the limit.
TEST_F(UnackedInvalidationSetTest, NoTruncation) {
  size_t kMax = UnackedInvalidationSet::kMaxBufferedInvalidations;

  for (size_t i = 0; i < kMax; ++i) {
    Invalidation inv = Invalidation::Init(kObjectId_, i, "payload");
    unacked_invalidations_.Add(inv);
  }

  SingleObjectInvalidationSet set = GetStoredInvalidations();
  ASSERT_EQ(kMax, set.GetSize());
  EXPECT_FALSE(set.StartsWithUnknownVersion());
  EXPECT_EQ(0, set.begin()->version());
  EXPECT_EQ(kMax-1, static_cast<size_t>(set.rbegin()->version()));
}

// Test that truncation happens as we reach the limit.
TEST_F(UnackedInvalidationSetTest, Truncation) {
  size_t kMax = UnackedInvalidationSet::kMaxBufferedInvalidations;

  for (size_t i = 0; i < kMax + 1; ++i) {
    Invalidation inv = Invalidation::Init(kObjectId_, i, "payload");
    unacked_invalidations_.Add(inv);
  }

  SingleObjectInvalidationSet set = GetStoredInvalidations();
  ASSERT_EQ(kMax, set.GetSize());
  EXPECT_TRUE(set.StartsWithUnknownVersion());
  EXPECT_TRUE(set.begin()->is_unknown_version());
  EXPECT_EQ(kMax, static_cast<size_t>(set.rbegin()->version()));
}

// Test that we don't truncate while a handler is registered.
TEST_F(UnackedInvalidationSetTest, RegistrationAndTruncation) {
  unacked_invalidations_.SetHandlerIsRegistered();

  size_t kMax = UnackedInvalidationSet::kMaxBufferedInvalidations;

  for (size_t i = 0; i < kMax + 1; ++i) {
    Invalidation inv = Invalidation::Init(kObjectId_, i, "payload");
    unacked_invalidations_.Add(inv);
  }

  SingleObjectInvalidationSet set = GetStoredInvalidations();
  ASSERT_EQ(kMax+1, set.GetSize());
  EXPECT_FALSE(set.StartsWithUnknownVersion());
  EXPECT_EQ(0, set.begin()->version());
  EXPECT_EQ(kMax, static_cast<size_t>(set.rbegin()->version()));

  // Unregistering should re-enable truncation.
  unacked_invalidations_.SetHandlerIsUnregistered();
  SingleObjectInvalidationSet set2 = GetStoredInvalidations();
  ASSERT_EQ(kMax, set2.GetSize());
  EXPECT_TRUE(set2.StartsWithUnknownVersion());
  EXPECT_TRUE(set2.begin()->is_unknown_version());
  EXPECT_EQ(kMax, static_cast<size_t>(set2.rbegin()->version()));
}

// Test acknowledgement.
TEST_F(UnackedInvalidationSetTest, Acknowledge) {
  // inv2 is included in this test just to make sure invalidations that
  // are supposed to be unaffected by this operation will be unaffected.

  // We don't expect to be receiving acks or drops unless this flag is set.
  // Not that it makes much of a difference in behavior.
  unacked_invalidations_.SetHandlerIsRegistered();

  Invalidation inv1 = Invalidation::Init(kObjectId_, 10, "payload");
  Invalidation inv2 = Invalidation::InitUnknownVersion(kObjectId_);
  AckHandle inv1_handle = inv1.ack_handle();

  unacked_invalidations_.Add(inv1);
  unacked_invalidations_.Add(inv2);

  unacked_invalidations_.Acknowledge(inv1_handle);

  SingleObjectInvalidationSet set = GetStoredInvalidations();
  EXPECT_EQ(1U, set.GetSize());
  EXPECT_TRUE(set.StartsWithUnknownVersion());
}

// Test drops.
TEST_F(UnackedInvalidationSetTest, Drop) {
  // inv2 is included in this test just to make sure invalidations that
  // are supposed to be unaffected by this operation will be unaffected.

  // We don't expect to be receiving acks or drops unless this flag is set.
  // Not that it makes much of a difference in behavior.
  unacked_invalidations_.SetHandlerIsRegistered();

  Invalidation inv1 = Invalidation::Init(kObjectId_, 10, "payload");
  Invalidation inv2 = Invalidation::Init(kObjectId_, 15, "payload");
  AckHandle inv1_handle = inv1.ack_handle();

  unacked_invalidations_.Add(inv1);
  unacked_invalidations_.Add(inv2);

  unacked_invalidations_.Drop(inv1_handle);

  SingleObjectInvalidationSet set = GetStoredInvalidations();
  ASSERT_EQ(2U, set.GetSize());
  EXPECT_TRUE(set.StartsWithUnknownVersion());
  EXPECT_EQ(15, set.rbegin()->version());
}

class UnackedInvalidationSetSerializationTest
    : public UnackedInvalidationSetTest {
 public:
  UnackedInvalidationSet SerializeDeserialize() {
    std::unique_ptr<base::DictionaryValue> value =
        unacked_invalidations_.ToValue();
    UnackedInvalidationSet deserialized(kObjectId_);
    deserialized.ResetFromValue(*value);
    return deserialized;
  }
};

TEST_F(UnackedInvalidationSetSerializationTest, Empty) {
  UnackedInvalidationSet deserialized = SerializeDeserialize();
  EXPECT_THAT(unacked_invalidations_, test_util::Eq(deserialized));
}

TEST_F(UnackedInvalidationSetSerializationTest, OneInvalidation) {
  Invalidation inv = Invalidation::Init(kObjectId_, 10, "payload");
  unacked_invalidations_.Add(inv);

  UnackedInvalidationSet deserialized = SerializeDeserialize();
  EXPECT_THAT(unacked_invalidations_, test_util::Eq(deserialized));
}

TEST_F(UnackedInvalidationSetSerializationTest, WithUnknownVersion) {
  Invalidation inv1 = Invalidation::Init(kObjectId_, 10, "payload");
  Invalidation inv2 = Invalidation::InitUnknownVersion(kObjectId_);
  Invalidation inv3 = Invalidation::InitUnknownVersion(kObjectId_);
  unacked_invalidations_.Add(inv1);
  unacked_invalidations_.Add(inv2);
  unacked_invalidations_.Add(inv3);

  UnackedInvalidationSet deserialized = SerializeDeserialize();
  EXPECT_THAT(unacked_invalidations_, test_util::Eq(deserialized));
}

TEST_F(UnackedInvalidationSetSerializationTest, ValidConversionFromMap) {
  UnackedInvalidationsMap map;
  Invalidation inv = Invalidation::Init(kObjectId_, 10, "payload");
  unacked_invalidations_.Add(inv);
  std::unique_ptr<base::DictionaryValue> dict =
      unacked_invalidations_.ToValue();
  bool result = UnackedInvalidationSet::DeserializeSetIntoMap(*dict, &map);
  EXPECT_EQ(true, result);
  auto item = map.find(kObjectId_);
  ASSERT_NE(map.end(), item);
  EXPECT_EQ(kObjectId_, item->second.object_id());
}

TEST_F(UnackedInvalidationSetSerializationTest, InvalidConversionFromMap) {
  UnackedInvalidationsMap map;
  base::DictionaryValue dict;
  // Empty dictionary should fail.
  EXPECT_FALSE(UnackedInvalidationSet::DeserializeSetIntoMap(dict, &map));

  // Non-int source should fail.
  dict.SetString("source", "foo");
  EXPECT_FALSE(UnackedInvalidationSet::DeserializeSetIntoMap(dict, &map));

  // Missing "name" should fail.
  dict.SetString("source", base::IntToString(kObjectId_.source()));
  EXPECT_FALSE(UnackedInvalidationSet::DeserializeSetIntoMap(dict, &map));

  // The "invalidation-list" is not required, so add "name" to make valid.
  dict.SetString("name", kObjectId_.name());
  bool result = UnackedInvalidationSet::DeserializeSetIntoMap(dict, &map);
  EXPECT_TRUE(result);
  auto item = map.find(kObjectId_);
  ASSERT_NE(map.end(), item);
  EXPECT_EQ(kObjectId_, item->second.object_id());
}

}  // namespace

}  // namespace syncer
