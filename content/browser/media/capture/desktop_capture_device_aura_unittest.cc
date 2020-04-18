// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/desktop_capture_device_aura.h"

#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "base/location.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "content/browser/compositor/test/test_image_transport_factory.h"
#include "content/public/browser/desktop_media_id.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/capture/video_capture_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/window_parenting_client.h"
#include "ui/aura/test/aura_test_helper.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/wm/core/default_activation_client.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::AtMost;
using ::testing::DoAll;
using ::testing::Expectation;
using ::testing::InvokeWithoutArgs;
using ::testing::SaveArg;

namespace media {
class VideoFrame;
}  // namespace media

namespace content {
namespace {

const int kFrameRate = 30;

class MockDeviceClient : public media::VideoCaptureDevice::Client {
 public:
  MOCK_METHOD7(OnIncomingCapturedData,
               void(const uint8_t* data,
                    int length,
                    const media::VideoCaptureFormat& frame_format,
                    int rotation,
                    base::TimeTicks reference_time,
                    base::TimeDelta tiemstamp,
                    int frame_feedback_id));
  MOCK_METHOD6(OnIncomingCapturedGfxBuffer,
               void(gfx::GpuMemoryBuffer* buffer,
                    const media::VideoCaptureFormat& frame_format,
                    int clockwise_rotation,
                    base::TimeTicks reference_time,
                    base::TimeDelta timestamp,
                    int frame_feedback_id));
  MOCK_METHOD0(DoReserveOutputBuffer, void(void));
  MOCK_METHOD0(DoOnIncomingCapturedBuffer, void(void));
  MOCK_METHOD0(DoOnIncomingCapturedVideoFrame, void(void));
  MOCK_METHOD0(DoResurrectLastOutputBuffer, void(void));
  MOCK_METHOD2(OnError,
               void(const base::Location& from_here,
                    const std::string& reason));
  MOCK_METHOD0(OnStarted, void(void));

  // Trampoline methods to workaround GMOCK problems with std::unique_ptr<>.
  Buffer ReserveOutputBuffer(const gfx::Size& dimensions,
                             media::VideoPixelFormat format,
                             int frame_feedback_id) override {
    EXPECT_EQ(media::PIXEL_FORMAT_I420, format);
    DoReserveOutputBuffer();
    return Buffer();
  }
  void OnIncomingCapturedBuffer(Buffer buffer,
                                const media::VideoCaptureFormat& frame_format,
                                base::TimeTicks reference_time,
                                base::TimeDelta timestamp) override {
    DoOnIncomingCapturedBuffer();
  }
  void OnIncomingCapturedBufferExt(
      Buffer buffer,
      const media::VideoCaptureFormat& format,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      gfx::Rect visible_rect,
      const media::VideoFrameMetadata& additional_metadata) override {
    DoOnIncomingCapturedVideoFrame();
  }
  Buffer ResurrectLastOutputBuffer(const gfx::Size& dimensions,
                                   media::VideoPixelFormat format,
                                   int frame_feedback_id) override {
    EXPECT_EQ(media::PIXEL_FORMAT_I420, format);
    DoResurrectLastOutputBuffer();
    return Buffer();
  }
  double GetBufferPoolUtilization() const override { return 0.0; }
};

// Test harness that sets up a minimal environment with necessary stubs.
class DesktopCaptureDeviceAuraTest : public testing::Test {
 public:
  DesktopCaptureDeviceAuraTest() = default;
  ~DesktopCaptureDeviceAuraTest() override = default;

 protected:
  void SetUp() override {
    // The ContextFactory must exist before any Compositors are created.
    ImageTransportFactory::SetFactory(
        std::make_unique<TestImageTransportFactory>());
    helper_.reset(new aura::test::AuraTestHelper());
    helper_->SetUp(
        ImageTransportFactory::GetInstance()->GetContextFactory(),
        ImageTransportFactory::GetInstance()->GetContextFactoryPrivate());
    new wm::DefaultActivationClient(helper_->root_window());
    // We need a window to cover desktop area so that DesktopCaptureDeviceAura
    // can use gfx::NativeWindow::GetWindowAtScreenPoint() to locate the
    // root window associated with the primary display.
    gfx::Rect desktop_bounds = root_window()->bounds();
    window_delegate_.reset(new aura::test::TestWindowDelegate());
    desktop_window_.reset(new aura::Window(window_delegate_.get()));
    desktop_window_->Init(ui::LAYER_TEXTURED);
    desktop_window_->SetBounds(desktop_bounds);
    aura::client::ParentWindowWithContext(
        desktop_window_.get(), root_window(), desktop_bounds);
    desktop_window_->Show();
  }

  void TearDown() override {
    helper_->RunAllPendingInMessageLoop();
    root_window()->RemoveChild(desktop_window_.get());
    desktop_window_.reset();
    window_delegate_.reset();
    helper_->TearDown();
    base::RunLoop().RunUntilIdle();
    ImageTransportFactory::Terminate();
  }

  aura::Window* root_window() { return helper_->root_window(); }

 private:
  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<aura::test::AuraTestHelper> helper_;
  std::unique_ptr<aura::Window> desktop_window_;
  std::unique_ptr<aura::test::TestWindowDelegate> window_delegate_;

  DISALLOW_COPY_AND_ASSIGN(DesktopCaptureDeviceAuraTest);
};

TEST_F(DesktopCaptureDeviceAuraTest, StartAndStop) {
  std::unique_ptr<media::VideoCaptureDevice> capture_device =
      DesktopCaptureDeviceAura::Create(
          content::DesktopMediaID::RegisterAuraWindow(
              content::DesktopMediaID::TYPE_SCREEN, root_window()));
  ASSERT_TRUE(capture_device);

  std::unique_ptr<MockDeviceClient> client(new MockDeviceClient());
  EXPECT_CALL(*client, OnError(_, _)).Times(0);
  // |STARTED| is reported asynchronously, which may not be received if capture
  // is stopped immediately.
  EXPECT_CALL(*client, OnStarted()).Times(AtMost(1));

  media::VideoCaptureParams capture_params;
  capture_params.requested_format.frame_size.SetSize(640, 480);
  capture_params.requested_format.frame_rate = kFrameRate;
  capture_params.requested_format.pixel_format = media::PIXEL_FORMAT_I420;
  capture_device->AllocateAndStart(capture_params, std::move(client));
  capture_device->StopAndDeAllocate();
}

}  // namespace
}  // namespace content
