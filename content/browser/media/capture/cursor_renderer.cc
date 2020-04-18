// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/cursor_renderer.h"

#include <algorithm>
#include <cmath>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "skia/ext/image_operations.h"
#include "ui/gfx/geometry/rect.h"

namespace content {

namespace {

inline int clip_byte(int x) {
  return std::max(0, std::min(x, 255));
}

inline int alpha_blend(int alpha, int src, int dst) {
  return (src * alpha + dst * (255 - alpha)) / 255;
}

}  // namespace

CursorRenderer::CursorRenderer(CursorDisplaySetting cursor_display_setting)
    : cursor_display_setting_(cursor_display_setting),
      cursor_(gfx::NativeCursor()),
      update_scaled_cursor_bitmap_(false),
      mouse_move_behavior_atomic_(NOT_MOVING),
      weak_factory_(this) {
  // CursorRenderer can be constructed on any thread, but thereafter must be
  // used according to class-level comments.
  DETACH_FROM_SEQUENCE(ui_sequence_checker_);
  DETACH_FROM_SEQUENCE(render_sequence_checker_);
}

CursorRenderer::~CursorRenderer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);
}

base::WeakPtr<CursorRenderer> CursorRenderer::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void CursorRenderer::SnapshotCursorState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  base::AutoLock auto_lock(lock_);

  // In CURSOR_DISPLAYED_ON_MOUSE_MOVEMENT mode, if the user hasn't recently
  // moved nor clicked the mouse, do not render the mouse cursor.
  if (cursor_display_setting_ == CURSOR_DISPLAYED_ON_MOUSE_MOVEMENT &&
      mouse_move_behavior() != RECENTLY_MOVED_OR_CLICKED) {
    view_size_ = gfx::Size();
    return;
  }

  // Do not render the mouse cursor if the view is not in the foreground window
  // on the user's desktop.
  if (!IsCapturedViewActive()) {
    view_size_ = gfx::Size();
    return;
  }

  // Collect current view size and mouse cursor state.
  view_size_ = GetCapturedViewSize();
  if (view_size_.IsEmpty()) {
    return;
  }
  cursor_position_ = GetCursorPositionInView();
  if (!gfx::Rect(view_size_).Contains(cursor_position_)) {
    view_size_ = gfx::Size();
    return;
  }
  const gfx::NativeCursor cursor = GetLastKnownCursor();
  if (cursor != cursor_ || !cursor_image_.readyToDraw()) {
    cursor_ = cursor;
    cursor_image_ = GetLastKnownCursorImage(&cursor_hot_point_);
    // Force RenderOnVideoFrame() to re-generate its scaled cursor bitmap.
    update_scaled_cursor_bitmap_ = true;
  }
}

bool CursorRenderer::RenderOnVideoFrame(media::VideoFrame* frame,
                                        const gfx::Rect& region_in_frame,
                                        CursorRendererUndoer* undoer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(render_sequence_checker_);
  DCHECK(frame);

  // Grab the |lock_| to read from shared mouse cursor state, and maybe
  // re-render the scaled cursor bitmap.
  gfx::Size view_size;
  gfx::Point cursor_position;
  {
    base::AutoLock auto_lock(lock_);

    if (view_size_.IsEmpty() || !cursor_image_.readyToDraw()) {
      return false;
    }

    view_size = view_size_;

    cursor_position = cursor_position_;
    cursor_position.Offset(-cursor_hot_point_.x(), -cursor_hot_point_.y());
    // |cursor_position| is further modified, below; but the |lock_| need not be
    // held for that.

    const int scaled_width =
        base::saturated_cast<int>((static_cast<int64_t>(cursor_image_.width()) *
                                   region_in_frame.width()) /
                                  view_size.width());
    const int scaled_height = base::saturated_cast<int>(
        (static_cast<int64_t>(cursor_image_.height()) *
         region_in_frame.height()) /
        view_size.height());
    if (scaled_width <= 0 || scaled_height <= 0) {
      return false;
    }
    if (update_scaled_cursor_bitmap_) {
      scaled_cursor_bitmap_ = SkBitmap();
      update_scaled_cursor_bitmap_ = false;
    }
    if (!scaled_cursor_bitmap_.readyToDraw() ||
        scaled_width != scaled_cursor_bitmap_.width() ||
        scaled_height != scaled_cursor_bitmap_.height()) {
      scaled_cursor_bitmap_ = skia::ImageOperations::Resize(
          cursor_image_, skia::ImageOperations::RESIZE_BEST, scaled_width,
          scaled_height);
    }
  }

  // Translate cursor position from view coordinates to video frame content
  // coordinates.
  cursor_position.set_x(base::saturated_cast<int>(
      region_in_frame.x() +
      (static_cast<int64_t>(cursor_position.x()) * region_in_frame.width()) /
          view_size.width()));
  cursor_position.set_y(base::saturated_cast<int>(
      region_in_frame.y() +
      (static_cast<int64_t>(cursor_position.y()) * region_in_frame.height()) /
          view_size.height()));

  // Determine the region of the video frame to be modified.
  gfx::Rect rect = gfx::IntersectRects(
      gfx::Rect(cursor_position, gfx::Size(scaled_cursor_bitmap_.width(),
                                           scaled_cursor_bitmap_.height())),
      frame->visible_rect());
  if (rect.IsEmpty())
    return false;

  if (undoer)
    undoer->TakeSnapshot(*frame, rect);

  // Render the cursor in the video frame. This loop also performs a simple
  // RGB→YUV color space conversion, with alpha-blended compositing.
  for (int y = rect.y(); y < rect.bottom(); ++y) {
    int cursor_y = y - cursor_position.y();
    uint8_t* yplane = frame->visible_data(media::VideoFrame::kYPlane) +
                      y * frame->stride(media::VideoFrame::kYPlane);
    uint8_t* uplane = frame->visible_data(media::VideoFrame::kUPlane) +
                      (y / 2) * frame->stride(media::VideoFrame::kUPlane);
    uint8_t* vplane = frame->visible_data(media::VideoFrame::kVPlane) +
                      (y / 2) * frame->stride(media::VideoFrame::kVPlane);
    for (int x = rect.x(); x < rect.right(); ++x) {
      int cursor_x = x - cursor_position.x();
      SkColor color = scaled_cursor_bitmap_.getColor(cursor_x, cursor_y);
      int alpha = SkColorGetA(color);
      int color_r = SkColorGetR(color);
      int color_g = SkColorGetG(color);
      int color_b = SkColorGetB(color);
      int color_y = clip_byte(
          ((color_r * 66 + color_g * 129 + color_b * 25 + 128) >> 8) + 16);
      yplane[x] = alpha_blend(alpha, color_y, yplane[x]);

      // Only sample U and V at even coordinates.
      // TODO(miu): This isn't right. We should be blending four cursor pixels
      // into each U or V output pixel.
      if ((x % 2 == 0) && (y % 2 == 0)) {
        int color_u = clip_byte(
            ((color_r * -38 + color_g * -74 + color_b * 112 + 128) >> 8) + 128);
        int color_v = clip_byte(
            ((color_r * 112 + color_g * -94 + color_b * -18 + 128) >> 8) + 128);
        uplane[x / 2] = alpha_blend(alpha, color_u, uplane[x / 2]);
        vplane[x / 2] = alpha_blend(alpha, color_v, vplane[x / 2]);
      }
    }
  }

  return true;
}

void CursorRenderer::SetNeedsRedrawCallback(base::RepeatingClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  needs_redraw_callback_ = std::move(callback);
}

bool CursorRenderer::IsUserInteractingWithView() const {
  return mouse_move_behavior() == RECENTLY_MOVED_OR_CLICKED;
}

void CursorRenderer::OnMouseMoved(const gfx::Point& location) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  switch (mouse_move_behavior()) {
    case NOT_MOVING:
      set_mouse_move_behavior(STARTING_TO_MOVE);
      mouse_move_start_location_ = location;
      mouse_activity_ended_timer_.Start(
          FROM_HERE, base::TimeDelta::FromSeconds(IDLE_TIMEOUT_SECONDS),
          base::BindRepeating(&CursorRenderer::OnMouseHasGoneIdle,
                              base::Unretained(this)));
      break;
    case STARTING_TO_MOVE:
      if (std::abs(location.x() - mouse_move_start_location_.x()) >
              MIN_MOVEMENT_PIXELS ||
          std::abs(location.y() - mouse_move_start_location_.y()) >
              MIN_MOVEMENT_PIXELS) {
        set_mouse_move_behavior(RECENTLY_MOVED_OR_CLICKED);
        mouse_activity_ended_timer_.Reset();
      }
      break;
    case RECENTLY_MOVED_OR_CLICKED:
      mouse_activity_ended_timer_.Reset();
      break;
  }

  // If there is sufficient mouse activity, or the cursor should always be
  // displayed, snapshot the cursor state and run the redraw callback to show it
  // at its new location in the video.
  if (mouse_move_behavior() == RECENTLY_MOVED_OR_CLICKED ||
      cursor_display_setting_ == CURSOR_DISPLAYED_ALWAYS) {
    SnapshotCursorState();
    if (!needs_redraw_callback_.is_null()) {
      needs_redraw_callback_.Run();
    }
  }
}

void CursorRenderer::OnMouseClicked(const gfx::Point& location) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  if (mouse_activity_ended_timer_.IsRunning()) {
    mouse_activity_ended_timer_.Reset();
  } else {
    mouse_activity_ended_timer_.Start(
        FROM_HERE, base::TimeDelta::FromSeconds(IDLE_TIMEOUT_SECONDS),
        base::BindRepeating(&CursorRenderer::OnMouseHasGoneIdle,
                            base::Unretained(this)));
  }
  set_mouse_move_behavior(RECENTLY_MOVED_OR_CLICKED);

  // Regardless of the |cursor_display_setting_|, snapshot the cursor and run
  // the redraw callback to show it at its current location in the video.
  SnapshotCursorState();
  if (!needs_redraw_callback_.is_null()) {
    needs_redraw_callback_.Run();
  }
}

void CursorRenderer::OnMouseHasGoneIdle() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(ui_sequence_checker_);

  set_mouse_move_behavior(NOT_MOVING);

  // The timer has fired to indicate no further mouse activity. It's a good idea
  // to snapshot the cursor and run the redraw callback to ensure it is being
  // presented in the video correctly, whether showing its final location or to
  // remove it from the video.
  SnapshotCursorState();
  if (!needs_redraw_callback_.is_null()) {
    needs_redraw_callback_.Run();
  }
}

CursorRendererUndoer::CursorRendererUndoer() = default;

CursorRendererUndoer::~CursorRendererUndoer() = default;

CursorRendererUndoer::CursorRendererUndoer(CursorRendererUndoer&& other) =
    default;

CursorRendererUndoer& CursorRendererUndoer::operator=(
    CursorRendererUndoer&& other) = default;

namespace {

// Returns the rect of pixels in a Chroma plane affected by the given |rect| in
// the Luma plane.
gfx::Rect ToEncompassingChromaRect(const gfx::Rect& rect) {
  const int left = rect.x() / 2;
  const int top = rect.y() / 2;
  const int right = (rect.right() + 1) / 2;
  const int bottom = (rect.bottom() + 1) / 2;
  return gfx::Rect(left, top, right - left, bottom - top);
}

constexpr size_t kYuvPlanes[] = {media::VideoFrame::kYPlane,
                                 media::VideoFrame::kUPlane,
                                 media::VideoFrame::kVPlane};

}  // namespace

void CursorRendererUndoer::TakeSnapshot(const media::VideoFrame& frame,
                                        const gfx::Rect& rect) {
  DCHECK(frame.visible_rect().Contains(rect));

  rect_ = rect;
  const gfx::Rect chroma_rect = ToEncompassingChromaRect(rect_);
  snapshot_.resize(rect_.size().GetArea() + 2 * chroma_rect.size().GetArea());

  uint8_t* dst = snapshot_.data();
  for (auto plane : kYuvPlanes) {
    const gfx::Rect& plane_rect =
        (plane == media::VideoFrame::kYPlane) ? rect_ : chroma_rect;
    const int stride = frame.stride(plane);
    const uint8_t* src =
        frame.visible_data(plane) + plane_rect.y() * stride + plane_rect.x();
    for (int row = 0; row < plane_rect.height(); ++row) {
      memcpy(dst, src, plane_rect.width());
      src += stride;
      dst += plane_rect.width();
    }
  }
}

void CursorRendererUndoer::Undo(media::VideoFrame* frame) const {
  DCHECK(frame->visible_rect().Contains(rect_));

  const gfx::Rect chroma_rect = ToEncompassingChromaRect(rect_);

  const uint8_t* src = snapshot_.data();
  for (auto plane : kYuvPlanes) {
    const gfx::Rect& plane_rect =
        (plane == media::VideoFrame::kYPlane) ? rect_ : chroma_rect;
    const int stride = frame->stride(plane);
    uint8_t* dst =
        frame->visible_data(plane) + plane_rect.y() * stride + plane_rect.x();
    for (int row = 0; row < plane_rect.height(); ++row) {
      memcpy(dst, src, plane_rect.width());
      src += plane_rect.width();
      dst += stride;
    }
  }
}

}  // namespace content
