// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/local_muter.h"

#include <memory>

#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/unguessable_token.h"
#include "services/audio/group_coordinator.h"
#include "services/audio/group_member.h"
#include "services/audio/test/mock_group_member.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::UnguessableToken;

using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::StrictMock;

namespace audio {
namespace {

TEST(LocalMuterTest, MutesExistingMembers) {
  GroupCoordinator coordinator;

  // Create a group with two members.
  const UnguessableToken group_id = UnguessableToken::Create();
  StrictMock<MockGroupMember> member1(group_id);
  StrictMock<MockGroupMember> member2(group_id);

  // Create another group with one member, which should never have its mute
  // state changed.
  const UnguessableToken other_group_id = UnguessableToken::Create();
  ASSERT_NE(group_id, other_group_id);
  StrictMock<MockGroupMember> non_member(other_group_id);
  EXPECT_CALL(non_member, StartMuting()).Times(0);
  EXPECT_CALL(non_member, StopMuting()).Times(0);

  // When the members join the group, no mute change should occur.
  coordinator.RegisterGroupMember(&member1);
  coordinator.RegisterGroupMember(&member2);

  // When the muter is created, both members should be muted.
  EXPECT_CALL(member1, StartMuting());
  EXPECT_CALL(member2, StartMuting());
  auto muter = std::make_unique<LocalMuter>(&coordinator, group_id);
  Mock::VerifyAndClearExpectations(&member1);
  Mock::VerifyAndClearExpectations(&member2);

  // When the muter is destroyed, both members should be un-muted.
  EXPECT_CALL(member1, StopMuting());
  EXPECT_CALL(member2, StopMuting());
  muter = nullptr;
  Mock::VerifyAndClearExpectations(&member1);
  Mock::VerifyAndClearExpectations(&member2);

  coordinator.UnregisterGroupMember(&member1);
  coordinator.UnregisterGroupMember(&member2);
}

TEST(LocalMuterTest, MutesJoiningMembers) {
  GroupCoordinator coordinator;
  const UnguessableToken group_id = UnguessableToken::Create();

  LocalMuter muter(&coordinator, group_id);

  StrictMock<MockGroupMember> member(group_id);

  // Since muting is in-effect, the group member is immediately muted when
  // joining the group.
  EXPECT_CALL(member, StartMuting());
  coordinator.RegisterGroupMember(&member);
  Mock::VerifyAndClearExpectations(&member);

  // Leaving the group should have no effect on the mute state of the member.
  EXPECT_CALL(member, StartMuting()).Times(0);
  EXPECT_CALL(member, StopMuting()).Times(0);
  coordinator.UnregisterGroupMember(&member);
  Mock::VerifyAndClearExpectations(&member);
}

TEST(LocalMuter, UnmutesWhenLastBindingIsLost) {
  base::test::ScopedTaskEnvironment task_environment;
  GroupCoordinator coordinator;
  const UnguessableToken group_id = UnguessableToken::Create();

  // Later in this test, once both bindings have been closed, the following
  // callback should be run. The callback will delete the LocalMuter in the same
  // stack as the mojo connection error handler, just as would take place in the
  // live build.
  auto muter = std::make_unique<LocalMuter>(&coordinator, group_id);
  base::MockCallback<base::OnceClosure> callback;
  EXPECT_CALL(callback, Run()).WillOnce(InvokeWithoutArgs([&muter]() {
    muter.reset();
  }));
  muter->SetAllBindingsLostCallback(callback.Get());

  // Create two bindings to the muter.
  mojom::LocalMuterAssociatedPtr muter_ptr1;
  muter->AddBinding(mojo::MakeRequest(&muter_ptr1));
  ASSERT_TRUE(!!muter_ptr1);
  mojom::LocalMuterAssociatedPtr muter_ptr2;
  muter->AddBinding(mojo::MakeRequest(&muter_ptr2));
  ASSERT_TRUE(!!muter_ptr2);

  // A member joins the group and should be muted.
  StrictMock<MockGroupMember> member(group_id);
  EXPECT_CALL(member, StartMuting());
  coordinator.RegisterGroupMember(&member);
  Mock::VerifyAndClearExpectations(&member);

  // Nothing happens to the member when one of the bindings is closed.
  EXPECT_CALL(member, StopMuting()).Times(0);
  muter_ptr1 = nullptr;
  task_environment.RunUntilIdle();  // Propagate mojo tasks.
  Mock::VerifyAndClearExpectations(&member);

  // The member is unmuted once the second binding is closed.
  EXPECT_CALL(member, StopMuting());
  muter_ptr2 = nullptr;
  task_environment.RunUntilIdle();  // Propagate mojo tasks.
  Mock::VerifyAndClearExpectations(&member);

  // At this point, the LocalMuter should have been destroyed.
  EXPECT_FALSE(muter);

  coordinator.UnregisterGroupMember(&member);
}

}  // namespace
}  // namespace audio
