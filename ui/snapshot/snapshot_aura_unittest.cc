// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/snapshot/snapshot.h"

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/test_simple_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPixelRef.h"
#include "ui/aura/test/aura_test_helper.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/compositor/test/draw_waiter_for_test.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/gfx_paths.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/transform.h"
#include "ui/gl/gl_implementation.h"
#include "ui/wm/core/default_activation_client.h"

namespace ui {
namespace {

SkColor GetExpectedColorForPoint(int x, int y) {
  return SkColorSetRGB(std::min(x, 255), std::min(y, 255), 0);
}

// Paint simple rectangle on the specified aura window.
class TestPaintingWindowDelegate : public aura::test::TestWindowDelegate {
 public:
  explicit TestPaintingWindowDelegate(const gfx::Size& window_size)
      : window_size_(window_size) {
  }

  ~TestPaintingWindowDelegate() override {}

  void OnPaint(const ui::PaintContext& context) override {
    ui::PaintRecorder recorder(context, window_size_);
    for (int y = 0; y < window_size_.height(); ++y) {
      for (int x = 0; x < window_size_.width(); ++x) {
        recorder.canvas()->FillRect(gfx::Rect(x, y, 1, 1),
                                    GetExpectedColorForPoint(x, y));
      }
    }
  }

 private:
  gfx::Size window_size_;

  DISALLOW_COPY_AND_ASSIGN(TestPaintingWindowDelegate);
};

size_t GetFailedPixelsCountWithScaleFactor(const gfx::Image& image,
                                           int scale_factor) {
  const SkBitmap* bitmap = image.ToSkBitmap();
  uint32_t* bitmap_data =
      reinterpret_cast<uint32_t*>(bitmap->pixelRef()->pixels());
  size_t result = 0;
  for (int y = 0; y < bitmap->height(); y += scale_factor) {
    for (int x = 0; x < bitmap->width(); x += scale_factor) {
      if (static_cast<SkColor>(bitmap_data[x + y * bitmap->width()]) !=
          GetExpectedColorForPoint(x / scale_factor, y / scale_factor)) {
        ++result;
      }
    }
  }
  return result;
}

size_t GetFailedPixelsCount(const gfx::Image& image) {
  return GetFailedPixelsCountWithScaleFactor(image, 1);
}

}  // namespace

class SnapshotAuraTest : public testing::Test {
 public:
  SnapshotAuraTest() {}
  ~SnapshotAuraTest() override {}

  void SetUp() override {
    testing::Test::SetUp();

    // The ContextFactory must exist before any Compositors are created.
    // Snapshot test tests real drawing and readback, so needs pixel output.
    bool enable_pixel_output = true;
    ui::ContextFactory* context_factory = nullptr;
    ui::ContextFactoryPrivate* context_factory_private = nullptr;

    ui::InitializeContextFactoryForTests(enable_pixel_output, &context_factory,
                                         &context_factory_private);

    helper_.reset(new aura::test::AuraTestHelper());
    helper_->SetUp(context_factory, context_factory_private);
    new ::wm::DefaultActivationClient(helper_->root_window());
  }

  void TearDown() override {
    test_window_.reset();
    delegate_.reset();
    helper_->RunAllPendingInMessageLoop();
    helper_->TearDown();
    ui::TerminateContextFactoryForTests();
    testing::Test::TearDown();
  }

 protected:
  aura::Window* test_window() { return test_window_.get(); }
  aura::Window* root_window() { return helper_->root_window(); }
  aura::TestScreen* test_screen() { return helper_->test_screen(); }

  void WaitForDraw() {
    helper_->host()->compositor()->ScheduleDraw();
    ui::DrawWaiterForTest::WaitForCompositingEnded(
        helper_->host()->compositor());
  }

  void SetupTestWindow(const gfx::Rect& window_bounds) {
    delegate_.reset(new TestPaintingWindowDelegate(window_bounds.size()));
    test_window_.reset(aura::test::CreateTestWindowWithDelegate(
        delegate_.get(), 0, window_bounds, root_window()));
  }

  gfx::Image GrabSnapshotForTestWindow() {
    gfx::Rect source_rect(test_window_->bounds().size());
    aura::Window::ConvertRectToTarget(
        test_window(), root_window(), &source_rect);

    scoped_refptr<SnapshotHolder> holder(new SnapshotHolder);
    ui::GrabWindowSnapshotAsync(
        root_window(), source_rect,
        base::Bind(&SnapshotHolder::SnapshotCallback, holder));

    holder->WaitForSnapshot();
    DCHECK(holder->completed());
    return holder->image();
  }

 private:
  class SnapshotHolder : public base::RefCountedThreadSafe<SnapshotHolder> {
   public:
    SnapshotHolder() : completed_(false) {}

    void SnapshotCallback(gfx::Image image) {
      DCHECK(!completed_);
      image_ = image;
      completed_ = true;
      run_loop_.Quit();
    }
    void WaitForSnapshot() { run_loop_.Run(); }
    bool completed() const { return completed_; }
    const gfx::Image& image() const { return image_; }

   private:
    friend class base::RefCountedThreadSafe<SnapshotHolder>;

    virtual ~SnapshotHolder() {}

    base::RunLoop run_loop_;
    gfx::Image image_;
    bool completed_;
  };

  std::unique_ptr<aura::test::AuraTestHelper> helper_;
  std::unique_ptr<aura::Window> test_window_;
  std::unique_ptr<TestPaintingWindowDelegate> delegate_;
  std::vector<unsigned char> png_representation_;

  DISALLOW_COPY_AND_ASSIGN(SnapshotAuraTest);
};

TEST_F(SnapshotAuraTest, FullScreenWindow) {
  SetupTestWindow(root_window()->bounds());
  WaitForDraw();

  gfx::Image snapshot = GrabSnapshotForTestWindow();
  EXPECT_EQ(test_window()->bounds().size().ToString(),
            snapshot.Size().ToString());
  EXPECT_EQ(0u, GetFailedPixelsCount(snapshot));
}

TEST_F(SnapshotAuraTest, PartialBounds) {
  gfx::Rect test_bounds(100, 100, 300, 200);
  SetupTestWindow(test_bounds);
  WaitForDraw();

  gfx::Image snapshot = GrabSnapshotForTestWindow();
  EXPECT_EQ(test_bounds.size().ToString(), snapshot.Size().ToString());
  EXPECT_EQ(0u, GetFailedPixelsCount(snapshot));
}

TEST_F(SnapshotAuraTest, Rotated) {
  test_screen()->SetDisplayRotation(display::Display::ROTATE_90);

  gfx::Rect test_bounds(100, 100, 300, 200);
  SetupTestWindow(test_bounds);
  WaitForDraw();

  gfx::Image snapshot = GrabSnapshotForTestWindow();
  EXPECT_EQ(test_bounds.size().ToString(), snapshot.Size().ToString());
  EXPECT_EQ(0u, GetFailedPixelsCount(snapshot));
}

TEST_F(SnapshotAuraTest, UIScale) {
  const float kUIScale = 0.5f;
  test_screen()->SetUIScale(kUIScale);

  gfx::Rect test_bounds(100, 100, 300, 200);
  SetupTestWindow(test_bounds);
  WaitForDraw();

  // Snapshot always captures the physical pixels.
  gfx::SizeF snapshot_size(test_bounds.size());
  snapshot_size.Scale(1 / kUIScale);

  gfx::Image snapshot = GrabSnapshotForTestWindow();
  EXPECT_EQ(gfx::ToRoundedSize(snapshot_size).ToString(),
            snapshot.Size().ToString());
  EXPECT_EQ(0u, GetFailedPixelsCountWithScaleFactor(snapshot, 1 / kUIScale));
}

TEST_F(SnapshotAuraTest, DeviceScaleFactor) {
  test_screen()->SetDeviceScaleFactor(2.0f);

  gfx::Rect test_bounds(100, 100, 150, 100);
  SetupTestWindow(test_bounds);
  WaitForDraw();

  // Snapshot always captures the physical pixels.
  gfx::SizeF snapshot_size(test_bounds.size());
  snapshot_size.Scale(2.0f);

  gfx::Image snapshot = GrabSnapshotForTestWindow();
  EXPECT_EQ(gfx::ToRoundedSize(snapshot_size).ToString(),
            snapshot.Size().ToString());
  EXPECT_EQ(0u, GetFailedPixelsCountWithScaleFactor(snapshot, 2));
}

TEST_F(SnapshotAuraTest, RotateAndUIScale) {
  const float kUIScale = 0.5f;
  test_screen()->SetUIScale(kUIScale);
  test_screen()->SetDisplayRotation(display::Display::ROTATE_90);

  gfx::Rect test_bounds(100, 100, 300, 200);
  SetupTestWindow(test_bounds);
  WaitForDraw();

  // Snapshot always captures the physical pixels.
  gfx::SizeF snapshot_size(test_bounds.size());
  snapshot_size.Scale(1 / kUIScale);

  gfx::Image snapshot = GrabSnapshotForTestWindow();
  EXPECT_EQ(gfx::ToRoundedSize(snapshot_size).ToString(),
            snapshot.Size().ToString());
  EXPECT_EQ(0u, GetFailedPixelsCountWithScaleFactor(snapshot, 1 / kUIScale));
}

TEST_F(SnapshotAuraTest, RotateAndUIScaleAndScaleFactor) {
  test_screen()->SetDeviceScaleFactor(2.0f);
  const float kUIScale = 0.5f;
  test_screen()->SetUIScale(kUIScale);
  test_screen()->SetDisplayRotation(display::Display::ROTATE_90);

  gfx::Rect test_bounds(20, 30, 150, 100);
  SetupTestWindow(test_bounds);
  WaitForDraw();

  // Snapshot always captures the physical pixels.
  gfx::SizeF snapshot_size(test_bounds.size());
  snapshot_size.Scale(2.0f / kUIScale);

  gfx::Image snapshot = GrabSnapshotForTestWindow();
  EXPECT_EQ(gfx::ToRoundedSize(snapshot_size).ToString(),
            snapshot.Size().ToString());
  EXPECT_EQ(0u, GetFailedPixelsCountWithScaleFactor(snapshot, 2 / kUIScale));
}

}  // namespace ui
