// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "content/child/child_process.h"
#include "content/renderer/media/video_capture_impl.h"
#include "content/renderer/media/video_capture_impl_manager.h"
#include "media/base/bind_to_current_loop.h"
#include "media/capture/mojom/video_capture.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::SaveArg;
using media::BindToCurrentLoop;

namespace content {

ACTION_P(RunClosure, closure) {
  closure.Run();
}

namespace {

// Callback interface to be implemented by VideoCaptureImplManagerTest.
// MockVideoCaptureImpl intercepts IPC messages and calls these methods to
// simulate what the VideoCaptureHost would do.
class PauseResumeCallback {
 public:
  PauseResumeCallback() {}
  virtual ~PauseResumeCallback() {}

  virtual void OnPaused(media::VideoCaptureSessionId session_id) = 0;
  virtual void OnResumed(media::VideoCaptureSessionId session_id) = 0;
};

class MockVideoCaptureImpl : public VideoCaptureImpl,
                             public media::mojom::VideoCaptureHost {
 public:
  MockVideoCaptureImpl(media::VideoCaptureSessionId session_id,
                       PauseResumeCallback* pause_callback,
                       base::Closure destruct_callback)
      : VideoCaptureImpl(session_id),
        pause_callback_(pause_callback),
        destruct_callback_(destruct_callback) {}

  ~MockVideoCaptureImpl() override { destruct_callback_.Run(); }

 private:
  void Start(int32_t device_id,
             int32_t session_id,
             const media::VideoCaptureParams& params,
             media::mojom::VideoCaptureObserverPtr observer) override {
    // For every Start(), expect a corresponding Stop() call.
    EXPECT_CALL(*this, Stop(_));
    // Simulate device started.
    OnStateChanged(media::mojom::VideoCaptureState::STARTED);
  }

  MOCK_METHOD1(Stop, void(int32_t));

  void Pause(int device_id) override {
    pause_callback_->OnPaused(session_id());
  }

  void Resume(int32_t device_id,
              int32_t session_id,
              const media::VideoCaptureParams& params) override {
    pause_callback_->OnResumed(session_id);
  }

  MOCK_METHOD1(RequestRefreshFrame, void(int32_t));
  MOCK_METHOD3(ReleaseBuffer,
               void(int32_t, int32_t, double));

  void GetDeviceSupportedFormats(int32_t,
                                 int32_t,
                                 GetDeviceSupportedFormatsCallback) override {
    NOTREACHED();
  }

  void GetDeviceFormatsInUse(int32_t,
                             int32_t,
                             GetDeviceFormatsInUseCallback) override {
    NOTREACHED();
  }

  PauseResumeCallback* const pause_callback_;
  const base::Closure destruct_callback_;

  DISALLOW_COPY_AND_ASSIGN(MockVideoCaptureImpl);
};

class MockVideoCaptureImplManager : public VideoCaptureImplManager {
 public:
  MockVideoCaptureImplManager(PauseResumeCallback* pause_callback,
                              base::Closure stop_capture_callback)
      : pause_callback_(pause_callback),
        stop_capture_callback_(stop_capture_callback) {}
  ~MockVideoCaptureImplManager() override {}

 private:
  std::unique_ptr<VideoCaptureImpl> CreateVideoCaptureImplForTesting(
      media::VideoCaptureSessionId session_id) const override {
    auto video_capture_impl = std::make_unique<MockVideoCaptureImpl>(
        session_id, pause_callback_, stop_capture_callback_);
    video_capture_impl->SetVideoCaptureHostForTesting(video_capture_impl.get());
    return std::move(video_capture_impl);
  }

  PauseResumeCallback* const pause_callback_;
  const base::Closure stop_capture_callback_;

  DISALLOW_COPY_AND_ASSIGN(MockVideoCaptureImplManager);
};

}  // namespace

class VideoCaptureImplManagerTest : public ::testing::Test,
                                    public PauseResumeCallback {
 public:
  VideoCaptureImplManagerTest()
      : manager_(new MockVideoCaptureImplManager(
            this,
            BindToCurrentLoop(cleanup_run_loop_.QuitClosure()))) {}

 protected:
  static constexpr size_t kNumClients = 3;

  std::array<base::Closure, kNumClients> StartCaptureForAllClients(
      bool same_session_id) {
    base::RunLoop run_loop;
    base::Closure quit_closure = BindToCurrentLoop(run_loop.QuitClosure());

    InSequence s;
    if (!same_session_id) {
      // |OnStarted| will only be received once from each device if there are
      // multiple request to the same device.
      EXPECT_CALL(*this, OnStarted(_))
          .Times(kNumClients - 1)
          .RetiresOnSaturation();
    }
    EXPECT_CALL(*this, OnStarted(_))
        .WillOnce(RunClosure(std::move(quit_closure)))
        .RetiresOnSaturation();
    std::array<base::Closure, kNumClients> stop_callbacks;
    media::VideoCaptureParams params;
    params.requested_format = media::VideoCaptureFormat(
        gfx::Size(176, 144), 30, media::PIXEL_FORMAT_I420);
    for (size_t i = 0; i < kNumClients; ++i) {
      stop_callbacks[i] = StartCapture(
          same_session_id ? 0 : static_cast<media::VideoCaptureSessionId>(i),
          params);
    }
    run_loop.Run();
    return stop_callbacks;
  }

  void StopCaptureForAllClients(
      std::array<base::Closure, kNumClients>* stop_callbacks) {
    base::RunLoop run_loop;
    base::Closure quit_closure = BindToCurrentLoop(run_loop.QuitClosure());
    EXPECT_CALL(*this, OnStopped(_)).Times(kNumClients - 1)
        .RetiresOnSaturation();
    EXPECT_CALL(*this, OnStopped(_))
        .WillOnce(RunClosure(std::move(quit_closure)))
        .RetiresOnSaturation();
    for (const auto& stop_callback : *stop_callbacks)
      stop_callback.Run();
    run_loop.Run();
  }

  MOCK_METHOD2(OnFrameReady,
               void(const scoped_refptr<media::VideoFrame>&,
                    base::TimeTicks estimated_capture_time));
  MOCK_METHOD1(OnStarted, void(media::VideoCaptureSessionId id));
  MOCK_METHOD1(OnStopped, void(media::VideoCaptureSessionId id));
  MOCK_METHOD1(OnPaused, void(media::VideoCaptureSessionId id));
  MOCK_METHOD1(OnResumed, void(media::VideoCaptureSessionId id));

  void OnStateUpdate(media::VideoCaptureSessionId id, VideoCaptureState state) {
    if (state == VIDEO_CAPTURE_STATE_STARTED)
      OnStarted(id);
    else if (state == VIDEO_CAPTURE_STATE_STOPPED)
      OnStopped(id);
    else
      NOTREACHED();
  }

  base::Closure StartCapture(media::VideoCaptureSessionId id,
                             const media::VideoCaptureParams& params) {
    return manager_->StartCapture(
        id, params, base::Bind(&VideoCaptureImplManagerTest::OnStateUpdate,
                               base::Unretained(this), id),
        base::Bind(&VideoCaptureImplManagerTest::OnFrameReady,
                   base::Unretained(this)));
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ChildProcess child_process_;
  base::RunLoop cleanup_run_loop_;
  std::unique_ptr<MockVideoCaptureImplManager> manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoCaptureImplManagerTest);
};

// Multiple clients with the same session id. There is only one
// media::VideoCapture object.
TEST_F(VideoCaptureImplManagerTest, MultipleClients) {
  std::array<base::Closure, kNumClients> release_callbacks;
  for (size_t i = 0; i < kNumClients; ++i)
    release_callbacks[i] = manager_->UseDevice(0);
  std::array<base::Closure, kNumClients> stop_callbacks =
      StartCaptureForAllClients(true);
  StopCaptureForAllClients(&stop_callbacks);
  for (const auto& release_callback : release_callbacks)
    release_callback.Run();
  cleanup_run_loop_.Run();
}

TEST_F(VideoCaptureImplManagerTest, NoLeak) {
  manager_->UseDevice(0).Reset();
  manager_.reset();
  cleanup_run_loop_.Run();
}

TEST_F(VideoCaptureImplManagerTest, SuspendAndResumeSessions) {
  std::array<base::Closure, kNumClients> release_callbacks;
  MediaStreamDevices video_devices;
  for (size_t i = 0; i < kNumClients; ++i) {
    release_callbacks[i] =
        manager_->UseDevice(static_cast<media::VideoCaptureSessionId>(i));
    MediaStreamDevice video_device;
    video_device.session_id = static_cast<media::VideoCaptureSessionId>(i);
    video_devices.push_back(video_device);
  }
  std::array<base::Closure, kNumClients> stop_callbacks =
      StartCaptureForAllClients(false);

  // Call SuspendDevices(true) to suspend all clients, and expect all to be
  // paused.
  {
    base::RunLoop run_loop;
    base::Closure quit_closure = BindToCurrentLoop(run_loop.QuitClosure());
    EXPECT_CALL(*this, OnPaused(0)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(*this, OnPaused(1)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(*this, OnPaused(2))
        .WillOnce(RunClosure(std::move(quit_closure)))
        .RetiresOnSaturation();
    manager_->SuspendDevices(video_devices, true);
    run_loop.Run();
  }

  // Call SuspendDevices(false) and expect all to be resumed.
  {
    base::RunLoop run_loop;
    base::Closure quit_closure = BindToCurrentLoop(run_loop.QuitClosure());
    EXPECT_CALL(*this, OnResumed(0)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(*this, OnResumed(1)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(*this, OnResumed(2))
        .WillOnce(RunClosure(std::move(quit_closure)))
        .RetiresOnSaturation();
    manager_->SuspendDevices(video_devices, false);
    run_loop.Run();
  }

  // Suspend just the first client and expect just the first client to be
  // paused.
  {
    base::RunLoop run_loop;
    base::Closure quit_closure = BindToCurrentLoop(run_loop.QuitClosure());
    EXPECT_CALL(*this, OnPaused(0))
        .WillOnce(RunClosure(std::move(quit_closure)))
        .RetiresOnSaturation();
    manager_->Suspend(0);
    run_loop.Run();
  }

  // Now call SuspendDevices(true) again, and expect just the second and third
  // clients to be paused.
  {
    base::RunLoop run_loop;
    base::Closure quit_closure = BindToCurrentLoop(run_loop.QuitClosure());
    EXPECT_CALL(*this, OnPaused(1)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(*this, OnPaused(2))
        .WillOnce(RunClosure(std::move(quit_closure)))
        .RetiresOnSaturation();
    manager_->SuspendDevices(video_devices, true);
    run_loop.Run();
  }

  // Resume just the first client, but it should not resume because all devices
  // are supposed to be suspended.
  {
    manager_->Resume(0);
    base::RunLoop().RunUntilIdle();
  }

  // Now, call SuspendDevices(false) and expect all to be resumed.
  {
    base::RunLoop run_loop;
    base::Closure quit_closure = BindToCurrentLoop(run_loop.QuitClosure());
    EXPECT_CALL(*this, OnResumed(0)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(*this, OnResumed(1)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(*this, OnResumed(2))
        .WillOnce(RunClosure(std::move(quit_closure)))
        .RetiresOnSaturation();
    manager_->SuspendDevices(video_devices, false);
    run_loop.Run();
  }

  StopCaptureForAllClients(&stop_callbacks);
  for (const auto& release_callback : release_callbacks)
    release_callback.Run();
  cleanup_run_loop_.Run();
}

}  // namespace content
