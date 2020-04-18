// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/cursor_renderer_aura.h"

#include <stdint.h>

#include <memory>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "media/base/video_frame.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/env.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/wm/core/default_activation_client.h"
#include "ui/wm/core/default_screen_position_client.h"
#include "ui/wm/core/window_util.h"

namespace content {

using aura::test::AuraTestBase;

class CursorRendererAuraTest : public AuraTestBase {
 public:
  CursorRendererAuraTest() {}
  ~CursorRendererAuraTest() override {}

  void SetUp() override {
    AuraTestBase::SetUp();
    // This is needed to avoid duplicate initialization across tests that leads
    // to a failure.
    if (!ui::ResourceBundle::HasSharedInstance()) {
      // Initialize the shared global resource bundle that has bitmap
      // resources needed by CursorRenderer
      base::FilePath pak_file;
      bool r = base::PathService::Get(base::DIR_MODULE, &pak_file);
      DCHECK(r);
      pak_file = pak_file.Append(FILE_PATH_LITERAL("content_shell.pak"));
      ui::ResourceBundle::InitSharedInstanceWithPakPath(pak_file);
    }

    window_.reset(aura::test::CreateTestWindowWithBounds(
        gfx::Rect(0, 0, 800, 600), root_window()));
    cursor_renderer_.reset(new CursorRendererAura(
        CursorRenderer::CURSOR_DISPLAYED_ON_MOUSE_MOVEMENT));
    cursor_renderer_->SetTargetView(window_.get());
    new wm::DefaultActivationClient(root_window());
    aura::client::SetScreenPositionClient(root_window(),
                                          &screen_position_client_);
    wm::ActivateWindow(window_.get());
  }

  void TearDown() override {
    aura::client::SetScreenPositionClient(root_window(), nullptr);
    cursor_renderer_.reset();
    window_.reset();
    AuraTestBase::TearDown();
  }

  base::TimeTicks Now() { return base::TimeTicks::Now(); }

  bool CursorDisplayed() {
    // Request rendering into a dummy video frame. If RenderCursorOnVideoFrame()
    // returns true, then the cursor is being displayed.
    if (!dummy_frame_) {
      constexpr gfx::Size dummy_frame_size = gfx::Size(320, 200);
      dummy_frame_ = media::VideoFrame::CreateZeroInitializedFrame(
          media::PIXEL_FORMAT_I420, dummy_frame_size,
          gfx::Rect(dummy_frame_size), dummy_frame_size, base::TimeDelta());
    }
    return RenderCursorOnVideoFrame(dummy_frame_.get(), nullptr);
  }

  bool RenderCursorOnVideoFrame(media::VideoFrame* frame,
                                CursorRendererUndoer* undoer) {
    return cursor_renderer_->RenderOnVideoFrame(frame, frame->visible_rect(),
                                                undoer);
  }

  bool IsUserInteractingWithView() {
    return cursor_renderer_->IsUserInteractingWithView();
  }

  void MoveMouseCursorWithinWindow() {
    gfx::Point point1(20, 20);
    ui::MouseEvent event1(ui::ET_MOUSE_MOVED, point1, point1, Now(), 0, 0);
    aura::Env::GetInstance()->SetLastMouseLocation(point1);
    cursor_renderer_->OnMouseEvent(&event1);
    gfx::Point point2(60, 60);
    ui::MouseEvent event2(ui::ET_MOUSE_MOVED, point2, point2, Now(), 0, 0);
    aura::Env::GetInstance()->SetLastMouseLocation(point2);
    cursor_renderer_->OnMouseEvent(&event2);
  }

  void MoveMouseCursorOutsideWindow() {
    gfx::Point point(1000, 1000);
    ui::MouseEvent event1(ui::ET_MOUSE_MOVED, point, point, Now(), 0, 0);
    aura::Env::GetInstance()->SetLastMouseLocation(point);
    cursor_renderer_->OnMouseEvent(&event1);
  }

  void SimulateMouseWentIdle() {
    EXPECT_TRUE(cursor_renderer_->mouse_activity_ended_timer_.IsRunning());
    cursor_renderer_->mouse_activity_ended_timer_.Stop();
    cursor_renderer_->OnMouseHasGoneIdle();
  }

  // A very simple test of whether there are any non-zero pixels
  // in the region |rect| within |frame|.
  bool NonZeroPixelsInRegion(scoped_refptr<media::VideoFrame> frame,
                             gfx::Rect rect) {
    bool y_found = false, u_found = false, v_found = false;
    for (int y = rect.y(); y < rect.bottom(); ++y) {
      uint8_t* yplane = frame->visible_data(media::VideoFrame::kYPlane) +
                        y * frame->stride(media::VideoFrame::kYPlane);
      uint8_t* uplane = frame->visible_data(media::VideoFrame::kUPlane) +
                        (y / 2) * frame->stride(media::VideoFrame::kUPlane);
      uint8_t* vplane = frame->visible_data(media::VideoFrame::kVPlane) +
                        (y / 2) * frame->stride(media::VideoFrame::kVPlane);
      for (int x = rect.x(); x < rect.right(); ++x) {
        if (yplane[x] != 0)
          y_found = true;
        if (uplane[x / 2])
          u_found = true;
        if (vplane[x / 2])
          v_found = true;
      }
    }
    return (y_found && u_found && v_found);
  }

 protected:
  wm::DefaultScreenPositionClient screen_position_client_;
  std::unique_ptr<aura::Window> window_;
  std::unique_ptr<CursorRendererAura> cursor_renderer_;

  scoped_refptr<media::VideoFrame> dummy_frame_;
};

TEST_F(CursorRendererAuraTest, CursorAlwaysDisplayed) {
  // Set up cursor renderer to always display cursor.
  cursor_renderer_.reset(
      new CursorRendererAura(CursorRenderer::CURSOR_DISPLAYED_ALWAYS));
  cursor_renderer_->SetTargetView(window_.get());

  // Cursor displayed at start.
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_FALSE(IsUserInteractingWithView());

  // Cursor displayed after mouse movement.
  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());

  // Cursor displayed after idle period.
  SimulateMouseWentIdle();
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_FALSE(IsUserInteractingWithView());

  // Cursor not displayed with mouse outside the window.
  MoveMouseCursorOutsideWindow();
  EXPECT_FALSE(CursorDisplayed());
}

TEST_F(CursorRendererAuraTest, CursorDuringMouseMovement) {
  // Cursor not displayed at start.
  EXPECT_FALSE(CursorDisplayed());
  EXPECT_FALSE(IsUserInteractingWithView());

  // Cursor displayed after mouse movement.
  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());

  // Cursor not displayed after idle period.
  SimulateMouseWentIdle();
  EXPECT_FALSE(CursorDisplayed());
  EXPECT_FALSE(IsUserInteractingWithView());

  // Cursor displayed with mouse movement following idle period.
  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());

  // Cursor not displayed if mouse outside the window
  MoveMouseCursorOutsideWindow();
  EXPECT_FALSE(CursorDisplayed());
}

TEST_F(CursorRendererAuraTest, CursorOnActiveWindow) {
  // Cursor not displayed at start.
  EXPECT_FALSE(CursorDisplayed());
  EXPECT_FALSE(IsUserInteractingWithView());

  // Cursor displayed after mouse movement.
  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());

  // Cursor not be displayed if a second window is activated.
  std::unique_ptr<aura::Window> window2(aura::test::CreateTestWindowWithBounds(
      gfx::Rect(0, 0, 800, 600), root_window()));
  wm::ActivateWindow(window2.get());
  MoveMouseCursorWithinWindow();
  EXPECT_FALSE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());

  // Cursor displayed if window activated again.
  wm::ActivateWindow(window_.get());
  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());
}

TEST_F(CursorRendererAuraTest, CursorRenderedOnFrame) {
  // Cursor not displayed at start.
  EXPECT_FALSE(CursorDisplayed());

  gfx::Size size(800, 600);
  scoped_refptr<media::VideoFrame> frame =
      media::VideoFrame::CreateZeroInitializedFrame(media::PIXEL_FORMAT_I420,
                                                    size, gfx::Rect(size), size,
                                                    base::TimeDelta());

  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());

  EXPECT_FALSE(NonZeroPixelsInRegion(frame, frame->visible_rect()));
  CursorRendererUndoer undoer;
  EXPECT_TRUE(RenderCursorOnVideoFrame(frame.get(), &undoer));
  EXPECT_TRUE(NonZeroPixelsInRegion(frame, gfx::Rect(50, 50, 70, 70)));
  undoer.Undo(frame.get());
  EXPECT_FALSE(NonZeroPixelsInRegion(frame, frame->visible_rect()));
}

TEST_F(CursorRendererAuraTest, CursorRenderedOnRootWindow) {
  cursor_renderer_->SetTargetView(root_window());

  // Cursor not displayed at start.
  EXPECT_FALSE(CursorDisplayed());

  // Cursor displayed after mouse movement.
  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());

  // Cursor being displayed even if another window is activated.
  std::unique_ptr<aura::Window> window2(aura::test::CreateTestWindowWithBounds(
      gfx::Rect(0, 0, 800, 600), root_window()));
  wm::ActivateWindow(window2.get());
  EXPECT_TRUE(CursorDisplayed());
}

}  // namespace content
