// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/cursor_renderer_mac.h"

#include <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "media/base/video_frame.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/test/cocoa_helper.h"
#include "ui/gfx/mac/coordinate_conversion.h"

namespace content {

const int kTestViewWidth = 320;
const int kTestViewHeight = 240;

CGEventRef myCGEventCallback(CGEventTapProxy proxy,
                             CGEventType type,
                             CGEventRef event,
                             void* refcon) {
  // Paranoid sanity check.
  if (type != kCGEventMouseMoved)
    return event;
  // Discard mouse-moved event.
  return NULL;
}

class CursorRendererMacTest : public ui::CocoaTest {
 public:
  CursorRendererMacTest() {}
  ~CursorRendererMacTest() override {}

  void SetUp() override {
    ui::CocoaTest::SetUp();
    base::scoped_nsobject<NSView> view([[NSView alloc]
        initWithFrame:NSMakeRect(0, 0, kTestViewWidth, kTestViewHeight)]);
    view_ = view.get();
    [[test_window() contentView] addSubview:view_];
    cursor_renderer_.reset(new CursorRendererMac(
        CursorRenderer::CURSOR_DISPLAYED_ON_MOUSE_MOVEMENT));
    cursor_renderer_->SetTargetView(view_);
    // Dis-associate mouse and cursor.
    StartEventTap();
    [test_window() setPretendIsKeyWindow:YES];
  }

  void TearDown() override {
    StopEventTap();
    cursor_renderer_.reset();
    ui::CocoaTest::TearDown();
  }

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

  // Here the |point| is in Aura coordinates (the origin (0, 0) is at top-left
  // of the view). To move the cursor to that point by Quartz Display service,
  // it needs to be converted into Cocoa coordinates (the origin is at
  // bottom-left of the main screen) first, and then info Quartz coordinates
  // (the origin is at top-left of the main display).
  void MoveMouseCursorWithinWindow() {
    CGWarpMouseCursorPosition(
        gfx::ScreenPointToNSPoint(gfx::Point(50, kTestViewHeight - 50)));
    cursor_renderer_->OnMouseEvent();

    CGWarpMouseCursorPosition(
        gfx::ScreenPointToNSPoint(gfx::Point(100, kTestViewHeight - 100)));
    cursor_renderer_->OnMouseEvent();
  }

  void MoveMouseCursorOutsideWindow() {
    CGWarpMouseCursorPosition(CGPointMake(1000, 200));
    cursor_renderer_->OnMouseEvent();
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

  // The test cases here need to move the actual cursor. If the mouse moves the
  // cursor at same time, the cursor position might be unexpected and test cases
  // will fail. So dis-associate mouse and cursor by enabling event tap for
  // mouse-moved event during tests runnning.
  void StartEventTap() {
    // Create an event tap. We are interested in mouse moved.
    CGEventMask eventMask = 1 << kCGEventMouseMoved;
    event_tap_ = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                                  kCGEventTapOptionDefault, eventMask,
                                  myCGEventCallback, NULL);
    if (event_tap_) {
      // Enable the event tap.
      CGEventTapEnable(event_tap_, true);
    }
  }

  void StopEventTap() { CGEventTapEnable(event_tap_, false); }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  NSView* view_;
  std::unique_ptr<CursorRendererMac> cursor_renderer_;

  CFMachPortRef event_tap_;

  scoped_refptr<media::VideoFrame> dummy_frame_;
};

TEST_F(CursorRendererMacTest, CursorDuringMouseMovement) {
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

TEST_F(CursorRendererMacTest, CursorOnActiveWindow) {
  // Cursor not displayed at start.
  EXPECT_FALSE(CursorDisplayed());
  EXPECT_FALSE(IsUserInteractingWithView());

  // Cursor displayed after mouse movement.
  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());

  // Cursor not displayed if window is not activated.
  [test_window() setPretendIsKeyWindow:NO];
  MoveMouseCursorWithinWindow();
  EXPECT_FALSE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());

  // Cursor is displayed again if window is activated again.
  [test_window() setPretendIsKeyWindow:YES];
  MoveMouseCursorWithinWindow();
  EXPECT_TRUE(CursorDisplayed());
  EXPECT_TRUE(IsUserInteractingWithView());
}

TEST_F(CursorRendererMacTest, CursorRenderedOnFrame) {
  // Cursor not displayed at start.
  EXPECT_FALSE(CursorDisplayed());

  gfx::Size size(kTestViewWidth, kTestViewHeight);
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

}  // namespace content
