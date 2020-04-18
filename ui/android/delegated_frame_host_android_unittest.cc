// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/delegated_frame_host_android.h"
#include "base/android/build_info.h"
#include "base/test/test_mock_time_task_runner.h"
#include "cc/layers/layer.h"
#include "cc/layers/solid_color_layer.h"
#include "components/viz/common/hit_test/hit_test_region_list.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/android/resources/resource_manager.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android_compositor.h"

namespace ui {
namespace {

using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Invoke;

class MockDelegatedFrameHostAndroidClient
    : public DelegatedFrameHostAndroid::Client {
 public:
  MOCK_METHOD1(SetBeginFrameSource, void(viz::BeginFrameSource*));
  MOCK_METHOD0(DidReceiveCompositorFrameAck, void());
  MOCK_METHOD4(DidPresentCompositorFrame,
               void(uint32_t, base::TimeTicks, base::TimeDelta, uint32_t));
  MOCK_METHOD1(DidDiscardCompositorFrame, void(uint32_t));
  MOCK_METHOD1(ReclaimResources,
               void(const std::vector<viz::ReturnedResource>&));
  MOCK_METHOD1(OnFrameTokenChanged, void(uint32_t));
  MOCK_METHOD0(DidReceiveFirstFrameAfterNavigation, void());
};

class MockWindowAndroidCompositor : public WindowAndroidCompositor {
 public:
  MOCK_METHOD1(AttachLayerForReadback, void(scoped_refptr<cc::Layer>));
  MOCK_METHOD1(DoRequestCopyOfOutputOnRootLayer, void(viz::CopyOutputRequest*));
  MOCK_METHOD0(SetNeedsAnimate, void());
  MOCK_METHOD0(GetResourceManager, ResourceManager&());
  MOCK_METHOD0(GetFrameSinkId, viz::FrameSinkId());
  MOCK_METHOD1(AddChildFrameSink, void(const viz::FrameSinkId&));
  MOCK_METHOD1(RemoveChildFrameSink, void(const viz::FrameSinkId&));
  MOCK_METHOD2(DoGetCompositorLock,
               CompositorLock*(CompositorLockClient*, base::TimeDelta));
  MOCK_CONST_METHOD0(IsDrawingFirstVisibleFrame, bool());

  // Helpers for move-only types:
  void RequestCopyOfOutputOnRootLayer(
      std::unique_ptr<viz::CopyOutputRequest> request) override {
    return DoRequestCopyOfOutputOnRootLayer(request.get());
  }

  std::unique_ptr<CompositorLock> GetCompositorLock(
      CompositorLockClient* client,
      base::TimeDelta time_delta) override {
    return std::unique_ptr<CompositorLock>(
        DoGetCompositorLock(client, time_delta));
  }
};

class MockCompositorLockManagerClient : public ui::CompositorLockManagerClient {
 public:
  MOCK_METHOD1(OnCompositorLockStateChanged, void(bool));
};

class DelegatedFrameHostAndroidTest : public testing::Test {
 public:
  DelegatedFrameHostAndroidTest()
      : frame_sink_id_(1, 1),
        task_runner_(new base::TestMockTimeTaskRunner()),
        lock_manager_(task_runner_, &lock_manager_client_) {
    host_frame_sink_manager_.SetLocalManager(&frame_sink_manager_impl_);
    view_.SetLayer(cc::SolidColorLayer::Create());
    frame_host_ = std::make_unique<DelegatedFrameHostAndroid>(
        &view_, &host_frame_sink_manager_, &client_, frame_sink_id_);
  }

  static viz::LocalSurfaceId GetFakeId() {
    return viz::LocalSurfaceId(1, 1, base::UnguessableToken::Create());
  }

  ui::CompositorLock* GetLock(CompositorLockClient* client,
                              base::TimeDelta time_delta) {
    return lock_manager_.GetCompositorLock(client, time_delta).release();
  }

  void SubmitCompositorFrame(const gfx::Size& frame_size = gfx::Size(10, 10)) {
    viz::CompositorFrame frame;
    auto render_pass = viz::RenderPass::Create();
    render_pass->output_rect = gfx::Rect(frame_size);
    frame.render_pass_list.push_back(std::move(render_pass));
    frame.metadata.begin_frame_ack.sequence_number = 1;
    frame.metadata.device_scale_factor = 1;
    frame_host_->SubmitCompositorFrame(GetFakeId(), std::move(frame),
                                       base::nullopt);
  }

  void SetUpValidFrame(const gfx::Size& frame_size) {
    EXPECT_CALL(compositor_, IsDrawingFirstVisibleFrame())
        .WillOnce(Return(true));
    EXPECT_CALL(compositor_, DoGetCompositorLock(frame_host_.get(), _))
        .WillOnce(Invoke(this, &DelegatedFrameHostAndroidTest::GetLock));
    EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(true))
        .Times(1);
    frame_host_->AttachToCompositor(&compositor_);

    EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(false))
        .Times(1);
    SubmitCompositorFrame(frame_size);
  }

 protected:
  MockWindowAndroidCompositor compositor_;
  ui::ViewAndroid view_;
  viz::FrameSinkManagerImpl frame_sink_manager_impl_;
  viz::HostFrameSinkManager host_frame_sink_manager_;
  MockDelegatedFrameHostAndroidClient client_;
  viz::FrameSinkId frame_sink_id_;
  std::unique_ptr<DelegatedFrameHostAndroid> frame_host_;
  MockCompositorLockManagerClient lock_manager_client_;
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  CompositorLockManager lock_manager_;
};

TEST_F(DelegatedFrameHostAndroidTest, CompositorLockDuringFirstFrame) {
  // Attach during the first frame, lock will be taken.
  EXPECT_CALL(compositor_, IsDrawingFirstVisibleFrame()).WillOnce(Return(true));
  EXPECT_CALL(compositor_, DoGetCompositorLock(frame_host_.get(), _))
      .WillOnce(Invoke(this, &DelegatedFrameHostAndroidTest::GetLock));
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(true))
      .Times(1);
  frame_host_->AttachToCompositor(&compositor_);

  // Lock should be released when we submit a compositor frame.
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(false))
      .Times(1);
  SubmitCompositorFrame();
}

TEST_F(DelegatedFrameHostAndroidTest, CompositorLockDuringLaterFrame) {
  // Attach after the first frame, lock will not be taken.
  EXPECT_CALL(compositor_, IsDrawingFirstVisibleFrame())
      .WillOnce(Return(false));
  EXPECT_CALL(compositor_, DoGetCompositorLock(_, _)).Times(0);
  frame_host_->AttachToCompositor(&compositor_);
}

TEST_F(DelegatedFrameHostAndroidTest, CompositorLockWithDelegatedContent) {
  // Submit a compositor frame to ensure we have delegated content.
  SubmitCompositorFrame();

  // Even though it's the first frame, we won't take the lock as we already have
  // delegated content.
  EXPECT_CALL(compositor_, IsDrawingFirstVisibleFrame()).WillOnce(Return(true));
  EXPECT_CALL(compositor_, DoGetCompositorLock(_, _)).Times(0);
  frame_host_->AttachToCompositor(&compositor_);
}

TEST_F(DelegatedFrameHostAndroidTest, CompositorLockReleasedWithDetach) {
  // Attach during the first frame, lock will be taken.
  EXPECT_CALL(compositor_, IsDrawingFirstVisibleFrame()).WillOnce(Return(true));
  EXPECT_CALL(compositor_, DoGetCompositorLock(frame_host_.get(), _))
      .WillOnce(Invoke(this, &DelegatedFrameHostAndroidTest::GetLock));
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(true))
      .Times(1);
  frame_host_->AttachToCompositor(&compositor_);

  // Lock should be released when we detach.
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(false))
      .Times(1);
  frame_host_->DetachFromCompositor();
}

TEST_F(DelegatedFrameHostAndroidTest, ResizeLockBasic) {
  // Resize lock is only enabled on O+.
  if (base::android::BuildInfo::GetInstance()->sdk_int() <
      base::android::SDK_VERSION_OREO) {
    return;
  }

  SetUpValidFrame(gfx::Size(10, 10));

  // Tell the frame host to resize, it should take a lock.
  EXPECT_CALL(compositor_, DoGetCompositorLock(frame_host_.get(), _))
      .WillOnce(Invoke(this, &DelegatedFrameHostAndroidTest::GetLock));
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(true))
      .Times(1);
  frame_host_->PixelSizeWillChange(gfx::Size(50, 50));

  // Submit a frame of the wrong size, nothing should change.
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(false))
      .Times(0);
  SubmitCompositorFrame(gfx::Size(20, 20));

  // Submit a frame with the right size, the lock should release.
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(false))
      .Times(1);
  SubmitCompositorFrame(gfx::Size(50, 50));
}

TEST_F(DelegatedFrameHostAndroidTest, ResizeLockNotTakenIfNoSizeChange) {
  // Resize lock is only enabled on O+.
  if (base::android::BuildInfo::GetInstance()->sdk_int() <
      base::android::SDK_VERSION_OREO) {
    return;
  }

  SetUpValidFrame(gfx::Size(10, 10));

  // Tell the frame host to resize to the existing size, nothing should happen.
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(true))
      .Times(0);
  frame_host_->PixelSizeWillChange(gfx::Size(10, 10));
}

TEST_F(DelegatedFrameHostAndroidTest, ResizeLockReleasedWithDetach) {
  // Resize lock is only enabled on O+.
  if (base::android::BuildInfo::GetInstance()->sdk_int() <
      base::android::SDK_VERSION_OREO) {
    return;
  }

  SetUpValidFrame(gfx::Size(10, 10));

  // Tell the frame host to resize, it should take a lock.
  EXPECT_CALL(compositor_, DoGetCompositorLock(frame_host_.get(), _))
      .WillOnce(Invoke(this, &DelegatedFrameHostAndroidTest::GetLock));
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(true))
      .Times(1);
  frame_host_->PixelSizeWillChange(gfx::Size(50, 50));

  // Lock should be released when we detach.
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(false))
      .Times(1);
  frame_host_->DetachFromCompositor();
}

TEST_F(DelegatedFrameHostAndroidTest, TestBothCompositorLocks) {
  // Resize lock is only enabled on O+.
  if (base::android::BuildInfo::GetInstance()->sdk_int() <
      base::android::SDK_VERSION_OREO) {
    return;
  }

  // Attach during the first frame, first lock will be taken.
  EXPECT_CALL(compositor_, IsDrawingFirstVisibleFrame()).WillOnce(Return(true));
  EXPECT_CALL(compositor_, DoGetCompositorLock(frame_host_.get(), _))
      .WillOnce(Invoke(this, &DelegatedFrameHostAndroidTest::GetLock));
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(true))
      .Times(1);
  frame_host_->AttachToCompositor(&compositor_);

  // Tell the frame host to resize, it should take a second lock.
  EXPECT_CALL(compositor_, DoGetCompositorLock(frame_host_.get(), _))
      .WillOnce(Invoke(this, &DelegatedFrameHostAndroidTest::GetLock));
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(true))
      .Times(0);
  frame_host_->PixelSizeWillChange(gfx::Size(50, 50));

  // Submit a compositor frame of the right size, both locks should release.
  EXPECT_CALL(lock_manager_client_, OnCompositorLockStateChanged(false))
      .Times(1);
  SubmitCompositorFrame(gfx::Size(50, 50));
}

}  // namespace
}  // namespace ui
