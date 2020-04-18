// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/task_traits.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace base {

TEST(TaskSchedulerTaskTraitsTest, Default) {
  constexpr TaskTraits traits = {};
  EXPECT_FALSE(traits.priority_set_explicitly());
  EXPECT_EQ(TaskPriority::USER_VISIBLE, traits.priority());
  EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN, traits.shutdown_behavior());
  EXPECT_FALSE(traits.may_block());
  EXPECT_FALSE(traits.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, TaskPriority) {
  constexpr TaskTraits traits = {TaskPriority::BACKGROUND};
  EXPECT_TRUE(traits.priority_set_explicitly());
  EXPECT_EQ(TaskPriority::BACKGROUND, traits.priority());
  EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN, traits.shutdown_behavior());
  EXPECT_FALSE(traits.may_block());
  EXPECT_FALSE(traits.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, TaskShutdownBehavior) {
  constexpr TaskTraits traits = {TaskShutdownBehavior::BLOCK_SHUTDOWN};
  EXPECT_FALSE(traits.priority_set_explicitly());
  EXPECT_EQ(TaskPriority::USER_VISIBLE, traits.priority());
  EXPECT_EQ(TaskShutdownBehavior::BLOCK_SHUTDOWN, traits.shutdown_behavior());
  EXPECT_FALSE(traits.may_block());
  EXPECT_FALSE(traits.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, MayBlock) {
  constexpr TaskTraits traits = {MayBlock()};
  EXPECT_FALSE(traits.priority_set_explicitly());
  EXPECT_EQ(TaskPriority::USER_VISIBLE, traits.priority());
  EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN, traits.shutdown_behavior());
  EXPECT_TRUE(traits.may_block());
  EXPECT_FALSE(traits.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, WithBaseSyncPrimitives) {
  constexpr TaskTraits traits = {WithBaseSyncPrimitives()};
  EXPECT_FALSE(traits.priority_set_explicitly());
  EXPECT_EQ(TaskPriority::USER_VISIBLE, traits.priority());
  EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN, traits.shutdown_behavior());
  EXPECT_FALSE(traits.may_block());
  EXPECT_TRUE(traits.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, MultipleTraits) {
  constexpr TaskTraits traits = {TaskPriority::BACKGROUND,
                                 TaskShutdownBehavior::BLOCK_SHUTDOWN,
                                 MayBlock(), WithBaseSyncPrimitives()};
  EXPECT_TRUE(traits.priority_set_explicitly());
  EXPECT_EQ(TaskPriority::BACKGROUND, traits.priority());
  EXPECT_EQ(TaskShutdownBehavior::BLOCK_SHUTDOWN, traits.shutdown_behavior());
  EXPECT_TRUE(traits.may_block());
  EXPECT_TRUE(traits.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, Copy) {
  constexpr TaskTraits traits = {TaskPriority::BACKGROUND,
                                 TaskShutdownBehavior::BLOCK_SHUTDOWN,
                                 MayBlock(), WithBaseSyncPrimitives()};
  constexpr TaskTraits traits_copy(traits);
  EXPECT_EQ(traits.priority_set_explicitly(),
            traits_copy.priority_set_explicitly());
  EXPECT_EQ(traits.priority(), traits_copy.priority());
  EXPECT_EQ(traits.shutdown_behavior(), traits_copy.shutdown_behavior());
  EXPECT_EQ(traits.may_block(), traits_copy.may_block());
  EXPECT_EQ(traits.with_base_sync_primitives(),
            traits_copy.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, OverridePriority) {
  constexpr TaskTraits left = {TaskPriority::BACKGROUND};
  constexpr TaskTraits right = {TaskPriority::USER_BLOCKING};
  constexpr TaskTraits overridden = TaskTraits::Override(left, right);
  EXPECT_TRUE(overridden.priority_set_explicitly());
  EXPECT_EQ(TaskPriority::USER_BLOCKING, overridden.priority());
  EXPECT_FALSE(overridden.shutdown_behavior_set_explicitly());
  EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
            overridden.shutdown_behavior());
  EXPECT_FALSE(overridden.may_block());
  EXPECT_FALSE(overridden.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, OverrideShutdownBehavior) {
  constexpr TaskTraits left = {TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN};
  constexpr TaskTraits right = {TaskShutdownBehavior::BLOCK_SHUTDOWN};
  constexpr TaskTraits overridden = TaskTraits::Override(left, right);
  EXPECT_FALSE(overridden.priority_set_explicitly());
  EXPECT_EQ(TaskPriority::USER_VISIBLE, overridden.priority());
  EXPECT_TRUE(overridden.shutdown_behavior_set_explicitly());
  EXPECT_EQ(TaskShutdownBehavior::BLOCK_SHUTDOWN,
            overridden.shutdown_behavior());
  EXPECT_FALSE(overridden.may_block());
  EXPECT_FALSE(overridden.with_base_sync_primitives());
}

TEST(TaskSchedulerTaskTraitsTest, OverrideMayBlock) {
  {
    constexpr TaskTraits left = {MayBlock()};
    constexpr TaskTraits right = {};
    constexpr TaskTraits overridden = TaskTraits::Override(left, right);
    EXPECT_FALSE(overridden.priority_set_explicitly());
    EXPECT_EQ(TaskPriority::USER_VISIBLE, overridden.priority());
    EXPECT_FALSE(overridden.shutdown_behavior_set_explicitly());
    EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
              overridden.shutdown_behavior());
    EXPECT_TRUE(overridden.may_block());
    EXPECT_FALSE(overridden.with_base_sync_primitives());
  }
  {
    constexpr TaskTraits left = {};
    constexpr TaskTraits right = {MayBlock()};
    constexpr TaskTraits overridden = TaskTraits::Override(left, right);
    EXPECT_FALSE(overridden.priority_set_explicitly());
    EXPECT_EQ(TaskPriority::USER_VISIBLE, overridden.priority());
    EXPECT_FALSE(overridden.shutdown_behavior_set_explicitly());
    EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
              overridden.shutdown_behavior());
    EXPECT_TRUE(overridden.may_block());
    EXPECT_FALSE(overridden.with_base_sync_primitives());
  }
}

TEST(TaskSchedulerTaskTraitsTest, OverrideWithBaseSyncPrimitives) {
  {
    constexpr TaskTraits left = {WithBaseSyncPrimitives()};
    constexpr TaskTraits right = {};
    constexpr TaskTraits overridden = TaskTraits::Override(left, right);
    EXPECT_FALSE(overridden.priority_set_explicitly());
    EXPECT_EQ(TaskPriority::USER_VISIBLE, overridden.priority());
    EXPECT_FALSE(overridden.shutdown_behavior_set_explicitly());
    EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
              overridden.shutdown_behavior());
    EXPECT_FALSE(overridden.may_block());
    EXPECT_TRUE(overridden.with_base_sync_primitives());
  }
  {
    constexpr TaskTraits left = {};
    constexpr TaskTraits right = {WithBaseSyncPrimitives()};
    constexpr TaskTraits overridden = TaskTraits::Override(left, right);
    EXPECT_FALSE(overridden.priority_set_explicitly());
    EXPECT_EQ(TaskPriority::USER_VISIBLE, overridden.priority());
    EXPECT_FALSE(overridden.shutdown_behavior_set_explicitly());
    EXPECT_EQ(TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
              overridden.shutdown_behavior());
    EXPECT_FALSE(overridden.may_block());
    EXPECT_TRUE(overridden.with_base_sync_primitives());
  }
}

TEST(TaskSchedulerTaskTraitsTest, OverrideMultipleTraits) {
  constexpr TaskTraits left = {MayBlock(), TaskPriority::BACKGROUND,
                               TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN};
  constexpr TaskTraits right = {WithBaseSyncPrimitives(),
                                TaskPriority::USER_BLOCKING};
  constexpr TaskTraits overridden = TaskTraits::Override(left, right);
  EXPECT_TRUE(overridden.priority_set_explicitly());
  EXPECT_EQ(right.priority(), overridden.priority());
  EXPECT_TRUE(overridden.shutdown_behavior_set_explicitly());
  EXPECT_EQ(left.shutdown_behavior(), overridden.shutdown_behavior());
  EXPECT_TRUE(overridden.may_block());
  EXPECT_TRUE(overridden.with_base_sync_primitives());
}

}  // namespace base
