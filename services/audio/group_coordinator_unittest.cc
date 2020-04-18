// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/group_coordinator.h"

#include "base/stl_util.h"
#include "base/unguessable_token.h"
#include "services/audio/group_member.h"
#include "services/audio/test/mock_group_member.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::UnguessableToken;

using testing::AtLeast;
using testing::NiceMock;
using testing::ReturnRef;
using testing::Sequence;
using testing::StrictMock;
using testing::_;

namespace audio {
namespace {

class MockGroupObserver : public GroupCoordinator::Observer {
 public:
  MockGroupObserver() = default;
  ~MockGroupObserver() override = default;

  MOCK_METHOD1(OnMemberJoinedGroup, void(GroupMember* member));
  MOCK_METHOD1(OnMemberLeftGroup, void(GroupMember* member));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockGroupObserver);
};

TEST(GroupCoordinatorTest, NeverUsed) {
  GroupCoordinator coordinator;
}

TEST(GroupCoordinatorTest, RegistersMembersInSameGroup) {
  const UnguessableToken group_id = UnguessableToken::Create();
  StrictMock<MockGroupMember> member1(group_id);
  StrictMock<MockGroupMember> member2(group_id);

  // An observer should see each member join and leave the group once.
  StrictMock<MockGroupObserver> observer;
  Sequence join_leave_sequence;
  EXPECT_CALL(observer, OnMemberJoinedGroup(&member1))
      .InSequence(join_leave_sequence);
  EXPECT_CALL(observer, OnMemberJoinedGroup(&member2))
      .InSequence(join_leave_sequence);
  EXPECT_CALL(observer, OnMemberLeftGroup(&member1))
      .InSequence(join_leave_sequence);
  EXPECT_CALL(observer, OnMemberLeftGroup(&member2))
      .InSequence(join_leave_sequence);

  GroupCoordinator coordinator;
  coordinator.AddObserver(group_id, &observer);
  coordinator.RegisterGroupMember(&member1);
  coordinator.RegisterGroupMember(&member2);

  const std::vector<GroupMember*>& members =
      coordinator.GetCurrentMembers(group_id);
  EXPECT_EQ(2u, members.size());
  EXPECT_TRUE(base::ContainsValue(members, &member1));
  EXPECT_TRUE(base::ContainsValue(members, &member2));
  EXPECT_TRUE(
      coordinator.GetCurrentMembers(UnguessableToken::Create()).empty());

  coordinator.UnregisterGroupMember(&member1);
  coordinator.UnregisterGroupMember(&member2);
  EXPECT_TRUE(coordinator.GetCurrentMembers(group_id).empty());

  coordinator.RemoveObserver(group_id, &observer);
  EXPECT_TRUE(coordinator.GetCurrentMembers(group_id).empty());
}

TEST(GroupCoordinatorTest, RegistersMembersInDifferentGroups) {
  const UnguessableToken group_id_a = UnguessableToken::Create();
  StrictMock<MockGroupMember> member_a_1(group_id_a);
  StrictMock<MockGroupMember> member_a_2(group_id_a);

  StrictMock<MockGroupObserver> observer_a;
  Sequence join_leave_sequence_a;
  EXPECT_CALL(observer_a, OnMemberJoinedGroup(&member_a_1))
      .InSequence(join_leave_sequence_a);
  EXPECT_CALL(observer_a, OnMemberJoinedGroup(&member_a_2))
      .InSequence(join_leave_sequence_a);
  EXPECT_CALL(observer_a, OnMemberLeftGroup(&member_a_1))
      .InSequence(join_leave_sequence_a);
  EXPECT_CALL(observer_a, OnMemberLeftGroup(&member_a_2))
      .InSequence(join_leave_sequence_a);

  const UnguessableToken group_id_b = UnguessableToken::Create();
  StrictMock<MockGroupMember> member_b_1(group_id_b);

  StrictMock<MockGroupObserver> observer_b;
  Sequence join_leave_sequence_b;
  EXPECT_CALL(observer_b, OnMemberJoinedGroup(&member_b_1))
      .InSequence(join_leave_sequence_b);
  EXPECT_CALL(observer_b, OnMemberLeftGroup(&member_b_1))
      .InSequence(join_leave_sequence_b);

  GroupCoordinator coordinator;
  coordinator.AddObserver(group_id_a, &observer_a);
  coordinator.AddObserver(group_id_b, &observer_b);
  coordinator.RegisterGroupMember(&member_a_1);
  coordinator.RegisterGroupMember(&member_b_1);
  coordinator.RegisterGroupMember(&member_a_2);
  const std::vector<GroupMember*>& members_a =
      coordinator.GetCurrentMembers(group_id_a);
  EXPECT_EQ(2u, members_a.size());
  EXPECT_TRUE(base::ContainsValue(members_a, &member_a_1));
  EXPECT_TRUE(base::ContainsValue(members_a, &member_a_2));
  EXPECT_EQ(std::vector<GroupMember*>({&member_b_1}),
            coordinator.GetCurrentMembers(group_id_b));
  EXPECT_TRUE(
      coordinator.GetCurrentMembers(UnguessableToken::Create()).empty());

  coordinator.UnregisterGroupMember(&member_a_1);
  EXPECT_EQ(std::vector<GroupMember*>({&member_a_2}),
            coordinator.GetCurrentMembers(group_id_a));

  coordinator.UnregisterGroupMember(&member_b_1);
  EXPECT_TRUE(coordinator.GetCurrentMembers(group_id_b).empty());

  coordinator.UnregisterGroupMember(&member_a_2);
  EXPECT_TRUE(coordinator.GetCurrentMembers(group_id_a).empty());

  coordinator.RemoveObserver(group_id_a, &observer_a);
  coordinator.RemoveObserver(group_id_b, &observer_b);
  EXPECT_TRUE(coordinator.GetCurrentMembers(group_id_a).empty());
  EXPECT_TRUE(coordinator.GetCurrentMembers(group_id_b).empty());
}

TEST(GroupCoordinatorTest, TracksMembersWithoutAnObserverPresent) {
  const UnguessableToken group_id = UnguessableToken::Create();
  StrictMock<MockGroupMember> member1(group_id);
  StrictMock<MockGroupMember> member2(group_id);

  GroupCoordinator coordinator;
  coordinator.RegisterGroupMember(&member1);
  coordinator.RegisterGroupMember(&member2);

  const std::vector<GroupMember*>& members =
      coordinator.GetCurrentMembers(group_id);
  EXPECT_EQ(2u, members.size());
  EXPECT_TRUE(base::ContainsValue(members, &member1));
  EXPECT_TRUE(base::ContainsValue(members, &member2));
  EXPECT_TRUE(
      coordinator.GetCurrentMembers(UnguessableToken::Create()).empty());

  coordinator.UnregisterGroupMember(&member1);
  coordinator.UnregisterGroupMember(&member2);
  EXPECT_TRUE(coordinator.GetCurrentMembers(group_id).empty());
}

TEST(GroupCoordinatorTest, NotifiesOnlyWhileObserving) {
  const UnguessableToken group_id = UnguessableToken::Create();
  StrictMock<MockGroupMember> member1(group_id);
  StrictMock<MockGroupMember> member2(group_id);

  // The observer will only be around at the time when member2 joins the group
  // and when member1 leaves the group.
  StrictMock<MockGroupObserver> observer;
  Sequence join_leave_sequence;
  EXPECT_CALL(observer, OnMemberJoinedGroup(&member1)).Times(0);
  EXPECT_CALL(observer, OnMemberJoinedGroup(&member2))
      .InSequence(join_leave_sequence);
  EXPECT_CALL(observer, OnMemberLeftGroup(&member1))
      .InSequence(join_leave_sequence);
  EXPECT_CALL(observer, OnMemberLeftGroup(&member2)).Times(0);

  GroupCoordinator coordinator;
  coordinator.RegisterGroupMember(&member1);
  EXPECT_EQ(std::vector<GroupMember*>({&member1}),
            coordinator.GetCurrentMembers(group_id));

  coordinator.AddObserver(group_id, &observer);
  coordinator.RegisterGroupMember(&member2);
  const std::vector<GroupMember*>& members =
      coordinator.GetCurrentMembers(group_id);
  EXPECT_EQ(2u, members.size());
  EXPECT_TRUE(base::ContainsValue(members, &member1));
  EXPECT_TRUE(base::ContainsValue(members, &member2));

  coordinator.UnregisterGroupMember(&member1);
  EXPECT_EQ(std::vector<GroupMember*>({&member2}),
            coordinator.GetCurrentMembers(group_id));

  coordinator.RemoveObserver(group_id, &observer);
  EXPECT_EQ(std::vector<GroupMember*>({&member2}),
            coordinator.GetCurrentMembers(group_id));

  coordinator.UnregisterGroupMember(&member2);
  EXPECT_TRUE(coordinator.GetCurrentMembers(group_id).empty());
}

}  // namespace
}  // namespace audio
