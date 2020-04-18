// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_vsync_provider_win.h"

#include <memory>

#include "base/bind.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/message_filter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/win/hidden_window.h"

namespace gpu {

namespace {

class FakeChannel : public base::Thread, public IPC::Channel {
 public:
  explicit FakeChannel(const gfx::VSyncProvider::UpdateVSyncCallback& callback)
      : base::Thread("io"),
        callback_(callback),
        io_done_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                       base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  ~FakeChannel() override {
    task_runner()->PostTask(FROM_HERE,
                            base::Bind(&FakeChannel::RemoveFilterOnIOThread,
                                       base::Unretained(this)));
    io_done_event_.Wait();
    Stop();
  }

  void SetNeedsVSync(bool needs_vsync) {
    task_runner()->PostTask(FROM_HERE,
                            base::Bind(&FakeChannel::SetNeedsVSyncOnIOThread,
                                       base::Unretained(this), needs_vsync));
    io_done_event_.Wait();
  }

  void AddFilter(IPC::MessageFilter* message_filter) {
    message_filter_ = message_filter;

    // Start the thread.
    Start();

    task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&FakeChannel::AddFilterOnIOThread, base::Unretained(this)));
    io_done_event_.Wait();
  }

  // IPC::Channel implementation
  bool Send(IPC::Message* msg) override {
    IPC_BEGIN_MESSAGE_MAP(FakeChannel, *msg)
      IPC_MESSAGE_HANDLER(GpuCommandBufferMsg_UpdateVSyncParameters,
                          OnUpdateVSyncParameters);
      IPC_MESSAGE_UNHANDLED(return false)
    IPC_END_MESSAGE_MAP()
    return true;
  }

  bool Connect() override {
    NOTREACHED();
    return false;
  }

  void Close() override { NOTREACHED(); }

 private:
  void SetNeedsVSyncOnIOThread(bool needs_vsync) {
    GpuCommandBufferMsg_SetNeedsVSync msg(0, needs_vsync);
    message_filter_->OnMessageReceived(msg);
    io_done_event_.Signal();
  }

  void AddFilterOnIOThread() {
    message_filter_->OnFilterAdded(this);
    io_done_event_.Signal();
  }

  void RemoveFilterOnIOThread() {
    message_filter_->OnFilterRemoved();
    io_done_event_.Signal();
  }

  void OnUpdateVSyncParameters(base::TimeTicks timestamp,
                               base::TimeDelta interval) {
    callback_.Run(timestamp, interval);
  }

  gfx::VSyncProvider::UpdateVSyncCallback callback_;
  base::WaitableEvent io_done_event_;

  scoped_refptr<IPC::MessageFilter> message_filter_;

  DISALLOW_COPY_AND_ASSIGN(FakeChannel);
};

class FakeDelegate : public ImageTransportSurfaceDelegate,
                     public base::SupportsWeakPtr<FakeDelegate> {
 public:
  explicit FakeDelegate(FakeChannel* channel) : channel_(channel) {}

  void DidCreateAcceleratedSurfaceChildWindow(
      SurfaceHandle parent_window,
      SurfaceHandle child_window) override {}
  void DidSwapBuffersComplete(SwapBuffersCompleteParams params) override {}
  const gles2::FeatureInfo* GetFeatureInfo() const override { return nullptr; }
  const GpuPreferences& GetGpuPreferences() const override {
    return gpu_preferences_;
  }
  void SetSnapshotRequestedCallback(const base::Closure& callback) override {}
  void BufferPresented(const gfx::PresentationFeedback& feedback) override {}

  void AddFilter(IPC::MessageFilter* message_filter) override {
    channel_->AddFilter(message_filter);
  }

  int32_t GetRouteID() const override { return 0; }

 private:
  FakeChannel* channel_;
  GpuPreferences gpu_preferences_;
  DISALLOW_COPY_AND_ASSIGN(FakeDelegate);
};

}  // namespace

class GpuVSyncProviderTest : public testing::Test {
 public:
  GpuVSyncProviderTest()
      : vsync_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                     base::WaitableEvent::InitialState::NOT_SIGNALED) {}
  ~GpuVSyncProviderTest() override {}

  void SetUp() override {
    channel_.reset(new FakeChannel(
        base::Bind(&GpuVSyncProviderTest::OnVSync, base::Unretained(this))));
    delegate_.reset(new FakeDelegate(channel_.get()));
    provider_.reset(
        new GpuVSyncProviderWin(delegate_->AsWeakPtr(), ui::GetHiddenWindow()));
  }

  void TearDown() override {}

  int vsync_count() {
    base::AutoLock lock(lock_);
    return vsync_count_;
  }

  void reset_vsync_count() {
    base::AutoLock lock(lock_);
    vsync_count_ = 0;
  }

  void set_vsync_stop_count(int value) { vsync_stop_count_ = value; }

  void SetNeedsVSync(bool needs_vsync) { channel_->SetNeedsVSync(needs_vsync); }

 protected:
  base::WaitableEvent vsync_event_;
  std::unique_ptr<GpuVSyncProviderWin> provider_;

 private:
  void OnVSync(base::TimeTicks timestamp, base::TimeDelta interval) {
    // This is called on VSync worker thread.
    EXPECT_GT(timestamp, previous_vsync_timestamp_);
    previous_vsync_timestamp_ = timestamp;

    base::AutoLock lock(lock_);
    if (++vsync_count_ == vsync_stop_count_)
      vsync_event_.Signal();
  }

  base::Lock lock_;
  int vsync_count_ = 0;
  int vsync_stop_count_ = 0;
  base::TimeTicks previous_vsync_timestamp_;
  std::unique_ptr<FakeChannel> channel_;
  std::unique_ptr<FakeDelegate> delegate_;
};

// Tests that VSync signal production is controlled by SetNeedsVSync.
TEST_F(GpuVSyncProviderTest, VSyncSignalTest) {
  set_vsync_stop_count(3);

  constexpr base::TimeDelta wait_timeout =
      base::TimeDelta::FromMilliseconds(300);

  // Verify that there are no VSync signals before provider is enabled
  bool wait_result = vsync_event_.TimedWait(wait_timeout);
  EXPECT_FALSE(wait_result);
  EXPECT_EQ(0, vsync_count());

  SetNeedsVSync(true);

  vsync_event_.Wait();

  SetNeedsVSync(false);

  // Verify that VSync callbacks stop coming after disabling.
  // Please note that it might still be possible for one
  // callback to be in flight on VSync worker thread, so |vsync_count_|
  // could still be incremented once, but not enough times to trigger
  // |vsync_event_|.
  reset_vsync_count();
  wait_result = vsync_event_.TimedWait(wait_timeout);
  EXPECT_FALSE(wait_result);
}

// Verifies that VSync timestamp is monotonic.
TEST_F(GpuVSyncProviderTest, VSyncMonotonicTimestampTest) {
  set_vsync_stop_count(60);
  SetNeedsVSync(true);
  // Make sure this doesn't run for longer than 1 second in case VSync
  // callbacks are slowed by running multiple tests in parallel.
  vsync_event_.TimedWait(base::TimeDelta::FromMilliseconds(1000));
}

// Verifies that receiving SetNeedsVSync signal after stopping the v-sync
// doesn't trigger a crash.
TEST_F(GpuVSyncProviderTest, SetNeedsVSyncAfterShutdown) {
  provider_.reset();
  SetNeedsVSync(true);
}

}  // namespace gpu
