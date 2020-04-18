// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cursor/cursor_loader_x11.h"

#include <float.h>

#include "base/logging.h"
#include "build/build_config.h"
#include "skia/ext/image_operations.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/cursor_util.h"
#include "ui/base/cursor/cursors_aura.h"
#include "ui/base/x/x11_util.h"
#include "ui/display/display.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/gfx/skia_util.h"

namespace {

// Returns CSS cursor name from an Aura cursor ID.
const char* CursorCssNameFromId(ui::CursorType id) {
  switch (id) {
    case ui::CursorType::kMiddlePanning:
      return "all-scroll";
    case ui::CursorType::kEastPanning:
      return "e-resize";
    case ui::CursorType::kNorthPanning:
      return "n-resize";
    case ui::CursorType::kNorthEastPanning:
      return "ne-resize";
    case ui::CursorType::kNorthWestPanning:
      return "nw-resize";
    case ui::CursorType::kSouthPanning:
      return "s-resize";
    case ui::CursorType::kSouthEastPanning:
      return "se-resize";
    case ui::CursorType::kSouthWestPanning:
      return "sw-resize";
    case ui::CursorType::kWestPanning:
      return "w-resize";
    case ui::CursorType::kNone:
      return "none";
    case ui::CursorType::kGrab:
      return "grab";
    case ui::CursorType::kGrabbing:
      return "grabbing";

#if defined(OS_CHROMEOS)
    case ui::CursorType::kNull:
    case ui::CursorType::kPointer:
    case ui::CursorType::kNoDrop:
    case ui::CursorType::kNotAllowed:
    case ui::CursorType::kCopy:
    case ui::CursorType::kMove:
    case ui::CursorType::kEastResize:
    case ui::CursorType::kNorthResize:
    case ui::CursorType::kSouthResize:
    case ui::CursorType::kWestResize:
    case ui::CursorType::kNorthEastResize:
    case ui::CursorType::kNorthWestResize:
    case ui::CursorType::kSouthWestResize:
    case ui::CursorType::kSouthEastResize:
    case ui::CursorType::kIBeam:
    case ui::CursorType::kAlias:
    case ui::CursorType::kCell:
    case ui::CursorType::kContextMenu:
    case ui::CursorType::kCross:
    case ui::CursorType::kHelp:
    case ui::CursorType::kWait:
    case ui::CursorType::kNorthSouthResize:
    case ui::CursorType::kEastWestResize:
    case ui::CursorType::kNorthEastSouthWestResize:
    case ui::CursorType::kNorthWestSouthEastResize:
    case ui::CursorType::kProgress:
    case ui::CursorType::kColumnResize:
    case ui::CursorType::kRowResize:
    case ui::CursorType::kVerticalText:
    case ui::CursorType::kZoomIn:
    case ui::CursorType::kZoomOut:
    case ui::CursorType::kHand:
    case ui::CursorType::kDndNone:
    case ui::CursorType::kDndMove:
    case ui::CursorType::kDndCopy:
    case ui::CursorType::kDndLink:
      // In some environments, the image assets are not set (e.g. in
      // content-browsertests, content-shell etc.).
      return "left_ptr";
#else  // defined(OS_CHROMEOS)
    case ui::CursorType::kNull:
      return "left_ptr";
    case ui::CursorType::kPointer:
      return "left_ptr";
    case ui::CursorType::kMove:
      // Returning "move" is the correct thing here, but Blink doesn't
      // make a distinction between move and all-scroll.  Other
      // platforms use a cursor more consistent with all-scroll, so
      // use that.
      return "all-scroll";
    case ui::CursorType::kCross:
      return "crosshair";
    case ui::CursorType::kHand:
      return "pointer";
    case ui::CursorType::kIBeam:
      return "text";
    case ui::CursorType::kProgress:
      return "progress";
    case ui::CursorType::kWait:
      return "wait";
    case ui::CursorType::kHelp:
      return "help";
    case ui::CursorType::kEastResize:
      return "e-resize";
    case ui::CursorType::kNorthResize:
      return "n-resize";
    case ui::CursorType::kNorthEastResize:
      return "ne-resize";
    case ui::CursorType::kNorthWestResize:
      return "nw-resize";
    case ui::CursorType::kSouthResize:
      return "s-resize";
    case ui::CursorType::kSouthEastResize:
      return "se-resize";
    case ui::CursorType::kSouthWestResize:
      return "sw-resize";
    case ui::CursorType::kWestResize:
      return "w-resize";
    case ui::CursorType::kNorthSouthResize:
      return "ns-resize";
    case ui::CursorType::kEastWestResize:
      return "ew-resize";
    case ui::CursorType::kColumnResize:
      return "col-resize";
    case ui::CursorType::kRowResize:
      return "row-resize";
    case ui::CursorType::kNorthEastSouthWestResize:
      return "nesw-resize";
    case ui::CursorType::kNorthWestSouthEastResize:
      return "nwse-resize";
    case ui::CursorType::kVerticalText:
      return "vertical-text";
    case ui::CursorType::kZoomIn:
      return "zoom-in";
    case ui::CursorType::kZoomOut:
      return "zoom-out";
    case ui::CursorType::kCell:
      return "cell";
    case ui::CursorType::kContextMenu:
      return "context-menu";
    case ui::CursorType::kAlias:
      return "alias";
    case ui::CursorType::kNoDrop:
      return "no-drop";
    case ui::CursorType::kCopy:
      return "copy";
    case ui::CursorType::kNotAllowed:
      return "not-allowed";
    case ui::CursorType::kDndNone:
      return "dnd-none";
    case ui::CursorType::kDndMove:
      return "dnd-move";
    case ui::CursorType::kDndCopy:
      return "dnd-copy";
    case ui::CursorType::kDndLink:
      return "dnd-link";
#endif  // defined(OS_CHROMEOS)
    case ui::CursorType::kCustom:
      NOTREACHED();
      return "left_ptr";
  }
  NOTREACHED() << "Case not handled for " << static_cast<int>(id);
  return "left_ptr";
}

static const struct {
  const char* css_name;
  const char* fallback_name;
  int fallback_shape;
} kCursorFallbacks[] = {
    // clang-format off
    { "pointer",     "hand",            XC_hand2 },
    { "progress",    "left_ptr_watch",  XC_watch },
    { "wait",        nullptr,           XC_watch },
    { "cell",        nullptr,           XC_plus },
    { "all-scroll",  nullptr,           XC_fleur},
    { "crosshair",   nullptr,           XC_cross },
    { "text",        nullptr,           XC_xterm },
    { "not-allowed", "crossed_circle",  x11::None },
    { "grabbing",    nullptr,           XC_hand2 },
    { "col-resize",  nullptr,           XC_sb_h_double_arrow },
    { "row-resize",  nullptr,           XC_sb_v_double_arrow},
    { "n-resize",    nullptr,           XC_top_side},
    { "e-resize",    nullptr,           XC_right_side},
    { "s-resize",    nullptr,           XC_bottom_side},
    { "w-resize",    nullptr,           XC_left_side},
    { "ne-resize",   nullptr,           XC_top_right_corner},
    { "nw-resize",   nullptr,           XC_top_left_corner},
    { "se-resize",   nullptr,           XC_bottom_right_corner},
    { "sw-resize",   nullptr,           XC_bottom_left_corner},
    { "ew-resize",   nullptr,           XC_sb_h_double_arrow},
    { "ns-resize",   nullptr,           XC_sb_v_double_arrow},
    { "nesw-resize", "fd_double_arrow", x11::None},
    { "nwse-resize", "bd_double_arrow", x11::None},
    { "dnd-none",    "grabbing",        XC_hand2 },
    { "dnd-move",    "grabbing",        XC_hand2 },
    { "dnd-copy",    "grabbing",        XC_hand2 },
    { "dnd-link",    "grabbing",        XC_hand2 },
    // clang-format on
};

}  // namespace

namespace ui {

CursorLoader* CursorLoader::Create() {
  return new CursorLoaderX11;
}

CursorLoaderX11::ImageCursor::ImageCursor(XcursorImage* x_image,
                                          float scale,
                                          display::Display::Rotation rotation)
    : scale(scale), rotation(rotation) {
  cursor = CreateReffedCustomXCursor(x_image);
}

CursorLoaderX11::ImageCursor::~ImageCursor() {
  UnrefCustomXCursor(cursor);
}

CursorLoaderX11::CursorLoaderX11()
    : display_(gfx::GetXDisplay()),
      invisible_cursor_(CreateInvisibleCursor(), gfx::GetXDisplay()) {}

CursorLoaderX11::~CursorLoaderX11() {
  UnloadAll();
}

void CursorLoaderX11::LoadImageCursor(CursorType id,
                                      int resource_id,
                                      const gfx::Point& hot) {
  SkBitmap bitmap;
  gfx::Point hotspot = hot;

  GetImageCursorBitmap(resource_id, scale(), rotation(), &hotspot, &bitmap);
  XcursorImage* x_image = SkBitmapToXcursorImage(&bitmap, hotspot);
  image_cursors_[id].reset(new ImageCursor(x_image, scale(), rotation()));
}

void CursorLoaderX11::LoadAnimatedCursor(CursorType id,
                                         int resource_id,
                                         const gfx::Point& hot,
                                         int frame_delay_ms) {
  std::vector<SkBitmap> bitmaps;
  gfx::Point hotspot = hot;

  GetAnimatedCursorBitmaps(resource_id, scale(), rotation(), &hotspot,
                           &bitmaps);

  XcursorImages* x_images = XcursorImagesCreate(bitmaps.size());
  x_images->nimage = bitmaps.size();

  for (unsigned int frame = 0; frame < bitmaps.size(); ++frame) {
    XcursorImage* x_image = SkBitmapToXcursorImage(&bitmaps[frame], hotspot);
    x_image->delay = frame_delay_ms;
    x_images->images[frame] = x_image;
  }

  animated_cursors_[id] = std::make_pair(
      XcursorImagesLoadCursor(gfx::GetXDisplay(), x_images), x_images);
}

void CursorLoaderX11::UnloadAll() {
  image_cursors_.clear();

  // Free animated cursors and images.
  for (const auto& cursor : animated_cursors_) {
    XcursorImagesDestroy(
        cursor.second.second);  // also frees individual frames.
    XFreeCursor(gfx::GetXDisplay(), cursor.second.first);
  }
}

void CursorLoaderX11::SetPlatformCursor(gfx::NativeCursor* cursor) {
  DCHECK(cursor);

  if (*cursor == CursorType::kNone) {
    cursor->SetPlatformCursor(invisible_cursor_.get());
    return;
  }

  if (*cursor == CursorType::kCustom)
    return;

  cursor->set_device_scale_factor(scale());
  cursor->SetPlatformCursor(CursorFromId(cursor->native_type()));
}

const XcursorImage* CursorLoaderX11::GetXcursorImageForTest(CursorType id) {
  return test::GetCachedXcursorImage(image_cursors_[id]->cursor);
}

bool CursorLoaderX11::IsImageCursor(gfx::NativeCursor native_cursor) {
  CursorType type = native_cursor.native_type();
  return image_cursors_.count(type) || animated_cursors_.count(type);
}

::Cursor CursorLoaderX11::CursorFromId(CursorType id) {
  const char* css_name = CursorCssNameFromId(id);

  auto font_it = font_cursors_.find(id);
  if (font_it != font_cursors_.end())
    return font_it->second;
  auto image_it = image_cursors_.find(id);
  if (image_it != image_cursors_.end()) {
    if (image_it->second->scale == scale() &&
        image_it->second->rotation == rotation()) {
      return image_it->second->cursor;
    } else {
      image_cursors_.erase(image_it);
    }
  }

  // First try to load the cursor directly.
  ::Cursor cursor = XcursorLibraryLoadCursor(display_, css_name);
  if (cursor == x11::None) {
    // Try a similar cursor supplied by the native cursor theme.
    for (const auto& mapping : kCursorFallbacks) {
      if (strcmp(mapping.css_name, css_name) == 0) {
        if (mapping.fallback_name)
          cursor = XcursorLibraryLoadCursor(display_, mapping.fallback_name);
        if (cursor == x11::None && mapping.fallback_shape)
          cursor = XCreateFontCursor(display_, mapping.fallback_shape);
      }
    }
  }
  if (cursor != x11::None) {
    font_cursors_[id] = cursor;
    return cursor;
  }

  // If the theme is missing the desired cursor, use a chromium-supplied
  // fallback icon.
  int resource_id;
  gfx::Point point;
  if (ui::GetCursorDataFor(ui::CursorSize::kNormal, id, scale(), &resource_id,
                           &point)) {
    LoadImageCursor(id, resource_id, point);
    return image_cursors_[id]->cursor;
  }

  // As a last resort, return a left pointer.
  cursor = XCreateFontCursor(display_, XC_left_ptr);
  DCHECK(cursor);
  font_cursors_[id] = cursor;
  return cursor;
}

}  // namespace ui
