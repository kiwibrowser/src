// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CURSOR_CURSOR_DATA_H_
#define UI_BASE_CURSOR_CURSOR_DATA_H_

#include <vector>

#include "base/time/time.h"
#include "build/build_config.h"
#include "ui/base/ui_base_export.h"
#include "ui/gfx/geometry/point.h"

class SkBitmap;

namespace ui {
enum class CursorType;

// The new Cursor class. (aka, Cursor2)
//
// Contains all data for a cursor. Its type, along with any custom bitmap
// images, hotspot data, scaling factors, etc.
//
// Why a new class? ui::Cursor currently wraps a PlatformCursor, which is a
// platform specific representation, which is generated in //content/. This
// previously was OK, as a WebCursor was sent over chrome IPC from the renderer
// to the browser process, and then the data in WebCursor was turned into the
// an opaque platform specific structure, stuffed inside ui::Cursor, and then
// read by win32 or x11. Now, the windowing server can be in a separate
// process, so this doesn't work.
//
// Using a raw mojo struct is not convenient; we want to have copyable classes
// which are internally copy-on-write for large data, like the internally used
// SkBitmap, as we cache this data at multiple layers.
//
// TODO(erg): Rename this to ui::Cursor once we've mojoified the entire chain
// from the renderer to the window server.
class UI_BASE_EXPORT CursorData {
 public:
  CursorData();
  explicit CursorData(CursorType type);
  CursorData(const gfx::Point& hostpot_point,
             const std::vector<SkBitmap>& cursor_frames,
             float scale_factor,
             const base::TimeDelta& frame_delay);
  CursorData(const CursorData& cursor);
  CursorData(CursorData&& cursor);
  ~CursorData();

  CursorData& operator=(const CursorData& cursor);
  CursorData& operator=(CursorData&& cursor);

  CursorType cursor_type() const { return cursor_type_; }
  const base::TimeDelta& frame_delay() const { return frame_delay_; }
  float scale_factor() const { return scale_factor_; }
  const gfx::Point& hotspot_in_pixels() const { return hotspot_; }
  const std::vector<SkBitmap>& cursor_frames() const { return cursor_frames_; }

  // Returns true if this CursorData instance is of |cursor_type|.
  bool IsType(CursorType cursor_type) const;

  // Checks if the data in |rhs| was created from the same input data.
  //
  // This is subtly different from operator==, as we need this to be a
  // lightweight operation instead of performing pixel equality checks on
  // arbitrary sized SkBitmaps. So we check the internal SkBitmap generation
  // IDs, which are per-process, monotonically increasing ids which get changed
  // whenever there's a modification to the pixel data. This means that this
  // method can have false negatives: two SkBitmap instances made with the same
  // input data (but which weren't copied from each other) can have equal pixel
  // data, but different generation ids.
  bool IsSameAs(const CursorData& rhs) const;

 private:
  // A native type constant from cursor.h.
  CursorType cursor_type_;

  // The delay between cursor frames.
  base::TimeDelta frame_delay_;

  // The scale factor of the images in |cursor_frames_|.
  float scale_factor_;

  // The hotspot in cursor frames.
  gfx::Point hotspot_;

  // The frames of a cursor.
  std::vector<SkBitmap> cursor_frames_;

  // Generator IDs. The size of |generator_ids_| must be equal to the size of
  // cursor_frames_, and is generated when we set the bitmaps. We produce these
  // unique IDs so we can do quick equality checks.
  std::vector<uint32_t> generator_ids_;
};

}  // namespace ui

#endif  // UI_BASE_CURSOR_CURSOR_DATA_H_
