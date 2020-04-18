// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/public/object_id_invalidation_map.h"

#include <memory>

#include "google/cacheinvalidation/types.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

class ObjectIdInvalidationMapTest : public testing::Test {
 public:
  ObjectIdInvalidationMapTest()
      : kIdOne(ipc::invalidation::ObjectSource::TEST, "one"),
        kIdTwo(ipc::invalidation::ObjectSource::TEST, "two"),
        kInv1(Invalidation::Init(kIdOne, 10, "ten")) {
    set1.insert(kIdOne);
    set2.insert(kIdTwo);
    all_set.insert(kIdOne);
    all_set.insert(kIdTwo);

    one_invalidation.Insert(kInv1);
    invalidate_all = ObjectIdInvalidationMap::InvalidateAll(all_set);
  }

 protected:
  const invalidation::ObjectId kIdOne;
  const invalidation::ObjectId kIdTwo;
  const Invalidation kInv1;

  ObjectIdSet set1;
  ObjectIdSet set2;
  ObjectIdSet all_set;
  ObjectIdInvalidationMap empty;
  ObjectIdInvalidationMap one_invalidation;
  ObjectIdInvalidationMap invalidate_all;
};

TEST_F(ObjectIdInvalidationMapTest, Empty) {
  EXPECT_TRUE(empty.Empty());
  EXPECT_FALSE(one_invalidation.Empty());
  EXPECT_FALSE(invalidate_all.Empty());
}

TEST_F(ObjectIdInvalidationMapTest, Equality) {
  ObjectIdInvalidationMap empty2;
  EXPECT_TRUE(empty == empty2);

  ObjectIdInvalidationMap one_invalidation2;
  one_invalidation2.Insert(kInv1);
  EXPECT_TRUE(one_invalidation == one_invalidation2);

  EXPECT_FALSE(empty == invalidate_all);
}

TEST_F(ObjectIdInvalidationMapTest, GetObjectIds) {
  EXPECT_EQ(ObjectIdSet(), empty.GetObjectIds());
  EXPECT_EQ(set1, one_invalidation.GetObjectIds());
  EXPECT_EQ(all_set, invalidate_all.GetObjectIds());
}

TEST_F(ObjectIdInvalidationMapTest, GetSubsetWithObjectIds) {
  EXPECT_TRUE(empty.GetSubsetWithObjectIds(set1).Empty());

  EXPECT_TRUE(one_invalidation.GetSubsetWithObjectIds(set1) ==
              one_invalidation);
  EXPECT_TRUE(one_invalidation.GetSubsetWithObjectIds(all_set) ==
              one_invalidation);
  EXPECT_TRUE(one_invalidation.GetSubsetWithObjectIds(set2).Empty());

  EXPECT_TRUE(invalidate_all.GetSubsetWithObjectIds(ObjectIdSet()).Empty());
}

TEST_F(ObjectIdInvalidationMapTest, SerializeEmpty) {
  std::unique_ptr<base::ListValue> value = empty.ToValue();
  ASSERT_TRUE(value.get());
  ObjectIdInvalidationMap deserialized;
  deserialized.ResetFromValue(*value);
  EXPECT_TRUE(empty == deserialized);
}

TEST_F(ObjectIdInvalidationMapTest, SerializeOneInvalidation) {
  std::unique_ptr<base::ListValue> value = one_invalidation.ToValue();
  ASSERT_TRUE(value.get());
  ObjectIdInvalidationMap deserialized;
  deserialized.ResetFromValue(*value);
  EXPECT_TRUE(one_invalidation == deserialized);
}

TEST_F(ObjectIdInvalidationMapTest, SerializeInvalidateAll) {
  std::unique_ptr<base::ListValue> value = invalidate_all.ToValue();
  ASSERT_TRUE(value.get());
  ObjectIdInvalidationMap deserialized;
  deserialized.ResetFromValue(*value);
  EXPECT_TRUE(invalidate_all == deserialized);
}

}  // namespace

}  // namespace syncer
