// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/content_video_view_overlay_allocator.h"

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/tick_clock.h"
#include "media/base/surface_manager.h"
#include "media/gpu/android/fake_codec_allocator.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::AnyNumber;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;
using testing::_;

namespace media {
class ContentVideoViewOverlayAllocatorTest : public testing::Test {
 public:
  class MockClient
      : public StrictMock<ContentVideoViewOverlayAllocator::Client> {
   public:
    MOCK_METHOD1(ScheduleLayout, void(const gfx::Rect&));
    MOCK_CONST_METHOD0(GetJavaSurface,
                       const base::android::JavaRef<jobject>&());

    MOCK_METHOD1(OnSurfaceAvailable, void(bool success));
    MOCK_METHOD0(OnSurfaceDestroyed, void());
    MOCK_METHOD0(GetSurfaceId, int32_t());
  };

  ContentVideoViewOverlayAllocatorTest() {}

  ~ContentVideoViewOverlayAllocatorTest() override {}

 protected:
  void SetUp() override {
    codec_allocator_ =
        new FakeCodecAllocator(base::SequencedTaskRunnerHandle::Get());
    allocator_ = new ContentVideoViewOverlayAllocator(codec_allocator_);

    avda1_ = new MockClient();
    avda2_ = new MockClient();
    avda3_ = new MockClient();
    // Default all |avda*| instances to surface ID 1.
    SetSurfaceId(avda1_, 1);
    SetSurfaceId(avda2_, 1);
    SetSurfaceId(avda3_, 1);
  }

  void TearDown() override {
    delete avda3_;
    delete avda2_;
    delete avda1_;
    delete allocator_;
    delete codec_allocator_;
  }

  void SetSurfaceId(MockClient* client, int32_t surface_id) {
    ON_CALL(*client, GetSurfaceId()).WillByDefault(Return(surface_id));
    EXPECT_CALL(*client, GetSurfaceId()).Times(AnyNumber());
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ContentVideoViewOverlayAllocator* allocator_;
  FakeCodecAllocator* codec_allocator_;

  MockClient* avda1_;
  MockClient* avda2_;
  MockClient* avda3_;
};

TEST_F(ContentVideoViewOverlayAllocatorTest, AllocatingAnOwnedSurfaceFails) {
  ASSERT_TRUE(allocator_->AllocateSurface(avda1_));
  ASSERT_FALSE(allocator_->AllocateSurface(avda2_));
}

TEST_F(ContentVideoViewOverlayAllocatorTest,
       LaterWaitersReplaceEarlierWaiters) {
  allocator_->AllocateSurface(avda1_);
  allocator_->AllocateSurface(avda2_);
  EXPECT_CALL(*avda2_, OnSurfaceAvailable(false));
  allocator_->AllocateSurface(avda3_);
}

TEST_F(ContentVideoViewOverlayAllocatorTest,
       WaitersBecomeOwnersWhenSurfacesAreReleased) {
  allocator_->AllocateSurface(avda1_);
  allocator_->AllocateSurface(avda2_);
  EXPECT_CALL(*avda2_, OnSurfaceAvailable(true));
  allocator_->DeallocateSurface(avda1_);
  // The surface should still be owned.
  ASSERT_FALSE(allocator_->AllocateSurface(avda1_));
}

TEST_F(ContentVideoViewOverlayAllocatorTest,
       DeallocatingUnownedSurfacesIsSafe) {
  allocator_->DeallocateSurface(avda1_);
}

TEST_F(ContentVideoViewOverlayAllocatorTest,
       WaitersAreRemovedIfTheyDeallocate) {
  allocator_->AllocateSurface(avda1_);
  allocator_->AllocateSurface(avda2_);
  allocator_->DeallocateSurface(avda2_);
  // |avda2_| should should not receive a notification.
  EXPECT_CALL(*avda2_, OnSurfaceAvailable(_)).Times(0);
  allocator_->DeallocateSurface(avda1_);
}

TEST_F(ContentVideoViewOverlayAllocatorTest, OwnersAreNotifiedOnDestruction) {
  allocator_->AllocateSurface(avda1_);
  // Owner is notified for a surface it owns.
  EXPECT_CALL(*avda1_, OnSurfaceDestroyed());
  allocator_->OnSurfaceDestroyed(1);
}

TEST_F(ContentVideoViewOverlayAllocatorTest,
       NonOwnersAreNotNotifiedOnDestruction) {
  allocator_->AllocateSurface(avda1_);
  // Not notified for a surface it doesn't own.
  EXPECT_CALL(*avda1_, OnSurfaceDestroyed()).Times(0);
  allocator_->OnSurfaceDestroyed(123);
}

TEST_F(ContentVideoViewOverlayAllocatorTest, WaitersAreNotifiedOnDestruction) {
  allocator_->AllocateSurface(avda1_);
  allocator_->AllocateSurface(avda2_);
  EXPECT_CALL(*avda1_, OnSurfaceDestroyed());
  EXPECT_CALL(*avda2_, OnSurfaceAvailable(false));
  allocator_->OnSurfaceDestroyed(1);
}

TEST_F(ContentVideoViewOverlayAllocatorTest,
       DeallocatingIsSafeDuringSurfaceDestroyed) {
  allocator_->AllocateSurface(avda1_);
  EXPECT_CALL(*avda1_, OnSurfaceDestroyed()).WillOnce(Invoke([=]() {
    allocator_->DeallocateSurface(avda1_);
  }));
  allocator_->OnSurfaceDestroyed(1);
}

}  // namespace media
