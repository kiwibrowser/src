// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_CURSOR_RENDERER_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_CURSOR_RENDERER_H_

#include <atomic>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/synchronization/lock.h"
#include "base/timer/timer.h"
#include "content/common/content_export.h"
#include "media/base/video_frame.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/cursor/cursor.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

class CursorRendererUndoer;

// CursorRenderer is an abstract base class that handles all the
// non-platform-specific common cursor rendering functionality. In order to
// track the cursor, the platform-specific implementation will listen to
// mouse events and this base class will process them.
//
// All parts of this class are meant to run on the UI BrowserThread, except for
// RenderOnVideoFrame() and IsUserInteractingWithView(), which may be called
// from any thread. It is up to the client code to ensure the CursorRenderer's
// lifetime while in use across multiple threads.
class CONTENT_EXPORT CursorRenderer {
 public:
  // Setting to control cursor display based on either mouse movement or always
  // forced to be enabled.
  enum CursorDisplaySetting {
    CURSOR_DISPLAYED_ALWAYS,
    CURSOR_DISPLAYED_ON_MOUSE_MOVEMENT,
  };

  static std::unique_ptr<CursorRenderer> Create(CursorDisplaySetting display);

  virtual ~CursorRenderer();

  // Sets a new target view to monitor for mouse cursor updates.
  virtual void SetTargetView(gfx::NativeView view) = 0;

  // Renders cursor on the given video frame within the content region,
  // returning true if |frame| was modified. |undoer| is optional: If provided,
  // it will be updated with state necessary for later undoing the cursor
  // rendering.
  bool RenderOnVideoFrame(media::VideoFrame* frame,
                          const gfx::Rect& region_in_frame,
                          CursorRendererUndoer* undoer);

  // Sets a callback that will be run whenever RenderOnVideoFrame() should be
  // called soon, to update the mouse cursor location or image in the video.
  void SetNeedsRedrawCallback(base::RepeatingClosure callback);

  // Returns true if the user has recently interacted with the view.
  bool IsUserInteractingWithView() const;

  // Returns a weak pointer.
  base::WeakPtr<CursorRenderer> GetWeakPtr();

 protected:
  enum {
    // Minium movement before cursor has been considered intentionally moved by
    // the user.
    MIN_MOVEMENT_PIXELS = 15,
    // Amount of time to elapse with no mouse activity before the cursor should
    // stop showing in the video. Does not apply to CURSOR_DISPLAYED_ALWAYS
    // mode, of course.
    IDLE_TIMEOUT_SECONDS = 2
  };

  explicit CursorRenderer(CursorDisplaySetting display);

  // Returns true if the captured view is a part of an active application
  // window.
  virtual bool IsCapturedViewActive() = 0;

  // Returns the size of the captured view (view coordinates).
  virtual gfx::Size GetCapturedViewSize() = 0;

  // Returns the cursor's position within the captured view (view coordinates).
  virtual gfx::Point GetCursorPositionInView() = 0;

  // Returns the last-known mouse cursor.
  virtual gfx::NativeCursor GetLastKnownCursor() = 0;

  // Returns the image of the last-known mouse cursor and its hotspot.
  virtual SkBitmap GetLastKnownCursorImage(gfx::Point* hot_point) = 0;

  // Called by subclasses to report mouse events within the captured view.
  void OnMouseMoved(const gfx::Point& location);
  void OnMouseClicked(const gfx::Point& location);

  // Called by the |mouse_activity_ended_timer_| once no mouse events have
  // occurred for IDLE_TIMEOUT_SECONDS. Also, called by subclasses when changing
  // the target view.
  void OnMouseHasGoneIdle();

 private:
  friend class CursorRendererAuraTest;
  friend class CursorRendererMacTest;

  enum MouseMoveBehavior {
    NOT_MOVING,                 // Mouse has not moved recently.
    STARTING_TO_MOVE,           // Mouse has moved, but not significantly.
    RECENTLY_MOVED_OR_CLICKED,  // Sufficient mouse activity present.
  };

  // Accessors for |mouse_move_behavior_atomic_|. See comments below.
  MouseMoveBehavior mouse_move_behavior() const {
    return mouse_move_behavior_atomic_.load(std::memory_order_relaxed);
  }
  void set_mouse_move_behavior(MouseMoveBehavior behavior) {
    mouse_move_behavior_atomic_.store(behavior, std::memory_order_relaxed);
  }

  // Takes a snapshot of the current mouse cursor state, for use by
  // RenderOnVideoFrame().
  void SnapshotCursorState();

  // Controls whether cursor is displayed based on active mouse movement.
  const CursorDisplaySetting cursor_display_setting_;

  // Protects members shared by RenderOnVideoFrame() and the rest of the class.
  base::Lock lock_;

  // These are updated by SnapshotCursorState(), then later read within
  // RenderOnVideoFrame(). Access is protected by |lock_|.
  gfx::Size view_size_;  // Empty means "do not show mouse cursor."
  gfx::Point cursor_position_;
  gfx::NativeCursor cursor_;
  gfx::Point cursor_hot_point_;
  SkBitmap cursor_image_;
  // Flag set to invalidate |scaled_cursor_bitmap_|.
  bool update_scaled_cursor_bitmap_;

  // A cache of the current scaled cursor bitmap. This is only accessed by the
  // thread calling RenderOnVideoFrame().
  SkBitmap scaled_cursor_bitmap_;

  // Updated in the mouse event handlers and used to decide whether the user is
  // interacting with the view and whether to run the |needs_redraw_callback_|.
  // These do not need to be protected by |lock_| since they are only accessed
  // on the UI BrowserThread.
  gfx::Point mouse_move_start_location_;
  base::OneShotTimer mouse_activity_ended_timer_;

  // Updated in the mouse event handlers (on the UI BrowserThread) and read from
  // by IsUserInteractingWithView() (on any thread). This is not protected by
  // |lock_| since strict memory ordering semantics are not necessary, just
  // atomicity between threads. All code should use the accessors to read or set
  // this value.
  std::atomic<MouseMoveBehavior> mouse_move_behavior_atomic_;

  // Run whenever the mouse cursor would be rendered differently than when it
  // was rendered in the last video frame.
  base::RepeatingClosure needs_redraw_callback_;

  // Everything except the constructor and RenderOnVideoFrame() must be called
  // on the UI BrowserThread.
  SEQUENCE_CHECKER(ui_sequence_checker_);

  // RenderOnVideoFrame() must be called on the same thread each time.
  SEQUENCE_CHECKER(render_sequence_checker_);

  base::WeakPtrFactory<CursorRenderer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CursorRenderer);
};

// Restores the original content of a VideoFrame, to the point before cursor
// rendering modified it. See CursorRenderer::RenderOnVideoFrame().
class CONTENT_EXPORT CursorRendererUndoer {
 public:
  CursorRendererUndoer();
  ~CursorRendererUndoer();

  CursorRendererUndoer(CursorRendererUndoer&& other);
  CursorRendererUndoer& operator=(CursorRendererUndoer&& other);

  void TakeSnapshot(const media::VideoFrame& frame, const gfx::Rect& rect);

  // Restores the frame content to the point where TakeSnapshot() was last
  // called.
  void Undo(media::VideoFrame* frame) const;

 private:
  gfx::Rect rect_;
  std::vector<uint8_t> snapshot_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_CURSOR_RENDERER_H_
