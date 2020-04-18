// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/offscreen_canvas_frame_dispatcher.h"

#include <memory>

#include "base/test/simple_test_tick_clock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkSurface.h"

using testing::_;
using testing::Mock;

namespace blink {

class MockOffscreenCanvasFrameDispatcher
    : public OffscreenCanvasFrameDispatcher {
 public:
  MockOffscreenCanvasFrameDispatcher()
      : OffscreenCanvasFrameDispatcher(nullptr, 0, 0, 0, {10, 10}) {}

  MOCK_METHOD2(PostImageToPlaceholder,
               void(scoped_refptr<StaticBitmapImage>, unsigned resource_id));
};

class OffscreenCanvasFrameDispatcherTest : public testing::Test {
 public:
  void DispatchOneFrame();
  OffscreenCanvasResourceProvider* GetResourceProvider() {
    return dispatcher_->offscreen_canvas_resource_provider_.get();
  }

  unsigned GetNumUnreclaimedFramesPosted() {
    return dispatcher_->num_unreclaimed_frames_posted_;
  }

  StaticBitmapImage* GetLatestUnpostedImage() {
    return dispatcher_->latest_unposted_image_.get();
  }

  unsigned GetLatestUnpostedResourceId() {
    return dispatcher_->latest_unposted_resource_id_;
  }

 protected:
  OffscreenCanvasFrameDispatcherTest() {
    dispatcher_ = std::make_unique<MockOffscreenCanvasFrameDispatcher>();
  }

  MockOffscreenCanvasFrameDispatcher* Dispatcher() { return dispatcher_.get(); }

 private:
  scoped_refptr<StaticBitmapImage> PrepareStaticBitmapImage();
  std::unique_ptr<MockOffscreenCanvasFrameDispatcher> dispatcher_;
};

void OffscreenCanvasFrameDispatcherTest::DispatchOneFrame() {
  sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(10, 10);
  dispatcher_->DispatchFrame(
      StaticBitmapImage::Create(surface->makeImageSnapshot()), 0.0,
      SkIRect::MakeEmpty());
}

TEST_F(OffscreenCanvasFrameDispatcherTest, PlaceholderRunsNormally) {
  /* We allow OffscreenCanvas to post up to 3 frames without hearing a response
   * from placeholder. */
  // Post first frame
  unsigned post_resource_id = 1u;
  EXPECT_CALL(*(Dispatcher()), PostImageToPlaceholder(_, post_resource_id));
  DispatchOneFrame();
  EXPECT_EQ(1u, GetNumUnreclaimedFramesPosted());
  EXPECT_EQ(1u, GetResourceProvider()->GetNextResourceId());
  Mock::VerifyAndClearExpectations(Dispatcher());

  // Post second frame
  post_resource_id++;
  EXPECT_CALL(*(Dispatcher()), PostImageToPlaceholder(_, post_resource_id));
  DispatchOneFrame();
  EXPECT_EQ(2u, GetNumUnreclaimedFramesPosted());
  EXPECT_EQ(2u, GetResourceProvider()->GetNextResourceId());
  Mock::VerifyAndClearExpectations(Dispatcher());

  // Post third frame
  post_resource_id++;
  EXPECT_CALL(*(Dispatcher()), PostImageToPlaceholder(_, post_resource_id));
  DispatchOneFrame();
  EXPECT_EQ(3u, GetNumUnreclaimedFramesPosted());
  EXPECT_EQ(3u, GetResourceProvider()->GetNextResourceId());
  EXPECT_EQ(nullptr, GetLatestUnpostedImage());
  Mock::VerifyAndClearExpectations(Dispatcher());

  /* We mock the behavior of placeholder on main thread here, by reclaiming
   * the resources in order. */
  // Reclaim first frame
  unsigned reclaim_resource_id = 1u;
  Dispatcher()->ReclaimResource(reclaim_resource_id);
  EXPECT_EQ(2u, GetNumUnreclaimedFramesPosted());

  // Reclaim second frame
  reclaim_resource_id++;
  Dispatcher()->ReclaimResource(reclaim_resource_id);
  EXPECT_EQ(1u, GetNumUnreclaimedFramesPosted());

  // Reclaim third frame
  reclaim_resource_id++;
  Dispatcher()->ReclaimResource(reclaim_resource_id);
  EXPECT_EQ(0u, GetNumUnreclaimedFramesPosted());
}

TEST_F(OffscreenCanvasFrameDispatcherTest, PlaceholderBeingBlocked) {
  /* When main thread is blocked, attempting to post more than 3 frames will
   * result in only 3 PostImageToPlaceholder. The latest unposted image will
   * be saved. */
  EXPECT_CALL(*(Dispatcher()), PostImageToPlaceholder(_, _)).Times(3);

  // Attempt to post 4 times
  DispatchOneFrame();
  DispatchOneFrame();
  DispatchOneFrame();
  DispatchOneFrame();
  unsigned post_resource_id = 4u;
  EXPECT_EQ(3u, GetNumUnreclaimedFramesPosted());
  EXPECT_EQ(post_resource_id, GetResourceProvider()->GetNextResourceId());
  EXPECT_TRUE(GetLatestUnpostedImage());
  EXPECT_EQ(post_resource_id, GetLatestUnpostedResourceId());

  // Attempt to post the 5th time. The latest unposted image will be replaced.
  post_resource_id++;
  DispatchOneFrame();
  EXPECT_EQ(3u, GetNumUnreclaimedFramesPosted());
  EXPECT_EQ(post_resource_id, GetResourceProvider()->GetNextResourceId());
  EXPECT_TRUE(GetLatestUnpostedImage());
  EXPECT_EQ(post_resource_id, GetLatestUnpostedResourceId());

  Mock::VerifyAndClearExpectations(Dispatcher());

  /* When main thread becomes unblocked, the first reclaim called by placeholder
   * will trigger OffscreenCanvasFrameDispatcher to post the last saved image.
   * Resource reclaim happens in the same order as frame posting. */
  unsigned reclaim_resource_id = 1u;
  EXPECT_CALL(*(Dispatcher()), PostImageToPlaceholder(_, post_resource_id));
  Dispatcher()->ReclaimResource(reclaim_resource_id);
  // Reclaim 1 frame and post 1 frame, so numPostImagesUnresponded remains as 3
  EXPECT_EQ(3u, GetNumUnreclaimedFramesPosted());
  // Not generating new resource Id
  EXPECT_EQ(post_resource_id, GetResourceProvider()->GetNextResourceId());
  EXPECT_FALSE(GetLatestUnpostedImage());
  EXPECT_EQ(0u, GetLatestUnpostedResourceId());
  Mock::VerifyAndClearExpectations(Dispatcher());

  reclaim_resource_id++;
  Dispatcher()->ReclaimResource(reclaim_resource_id);
  EXPECT_EQ(2u, GetNumUnreclaimedFramesPosted());
}

}  // namespace blink
