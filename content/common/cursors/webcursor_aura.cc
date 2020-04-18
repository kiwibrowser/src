// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/cursors/webcursor.h"

#include "base/logging.h"
#include "third_party/blink/public/platform/web_cursor_info.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/cursor_util.h"

using blink::WebCursorInfo;

namespace content {

gfx::NativeCursor WebCursor::GetNativeCursor() {
  switch (type_) {
    case WebCursorInfo::kTypePointer:
      return ui::CursorType::kPointer;
    case WebCursorInfo::kTypeCross:
      return ui::CursorType::kCross;
    case WebCursorInfo::kTypeHand:
      return ui::CursorType::kHand;
    case WebCursorInfo::kTypeIBeam:
      return ui::CursorType::kIBeam;
    case WebCursorInfo::kTypeWait:
      return ui::CursorType::kWait;
    case WebCursorInfo::kTypeHelp:
      return ui::CursorType::kHelp;
    case WebCursorInfo::kTypeEastResize:
      return ui::CursorType::kEastResize;
    case WebCursorInfo::kTypeNorthResize:
      return ui::CursorType::kNorthResize;
    case WebCursorInfo::kTypeNorthEastResize:
      return ui::CursorType::kNorthEastResize;
    case WebCursorInfo::kTypeNorthWestResize:
      return ui::CursorType::kNorthWestResize;
    case WebCursorInfo::kTypeSouthResize:
      return ui::CursorType::kSouthResize;
    case WebCursorInfo::kTypeSouthEastResize:
      return ui::CursorType::kSouthEastResize;
    case WebCursorInfo::kTypeSouthWestResize:
      return ui::CursorType::kSouthWestResize;
    case WebCursorInfo::kTypeWestResize:
      return ui::CursorType::kWestResize;
    case WebCursorInfo::kTypeNorthSouthResize:
      return ui::CursorType::kNorthSouthResize;
    case WebCursorInfo::kTypeEastWestResize:
      return ui::CursorType::kEastWestResize;
    case WebCursorInfo::kTypeNorthEastSouthWestResize:
      return ui::CursorType::kNorthEastSouthWestResize;
    case WebCursorInfo::kTypeNorthWestSouthEastResize:
      return ui::CursorType::kNorthWestSouthEastResize;
    case WebCursorInfo::kTypeColumnResize:
      return ui::CursorType::kColumnResize;
    case WebCursorInfo::kTypeRowResize:
      return ui::CursorType::kRowResize;
    case WebCursorInfo::kTypeMiddlePanning:
      return ui::CursorType::kMiddlePanning;
    case WebCursorInfo::kTypeEastPanning:
      return ui::CursorType::kEastPanning;
    case WebCursorInfo::kTypeNorthPanning:
      return ui::CursorType::kNorthPanning;
    case WebCursorInfo::kTypeNorthEastPanning:
      return ui::CursorType::kNorthEastPanning;
    case WebCursorInfo::kTypeNorthWestPanning:
      return ui::CursorType::kNorthWestPanning;
    case WebCursorInfo::kTypeSouthPanning:
      return ui::CursorType::kSouthPanning;
    case WebCursorInfo::kTypeSouthEastPanning:
      return ui::CursorType::kSouthEastPanning;
    case WebCursorInfo::kTypeSouthWestPanning:
      return ui::CursorType::kSouthWestPanning;
    case WebCursorInfo::kTypeWestPanning:
      return ui::CursorType::kWestPanning;
    case WebCursorInfo::kTypeMove:
      return ui::CursorType::kMove;
    case WebCursorInfo::kTypeVerticalText:
      return ui::CursorType::kVerticalText;
    case WebCursorInfo::kTypeCell:
      return ui::CursorType::kCell;
    case WebCursorInfo::kTypeContextMenu:
      return ui::CursorType::kContextMenu;
    case WebCursorInfo::kTypeAlias:
      return ui::CursorType::kAlias;
    case WebCursorInfo::kTypeProgress:
      return ui::CursorType::kProgress;
    case WebCursorInfo::kTypeNoDrop:
      return ui::CursorType::kNoDrop;
    case WebCursorInfo::kTypeCopy:
      return ui::CursorType::kCopy;
    case WebCursorInfo::kTypeNone:
      return ui::CursorType::kNone;
    case WebCursorInfo::kTypeNotAllowed:
      return ui::CursorType::kNotAllowed;
    case WebCursorInfo::kTypeZoomIn:
      return ui::CursorType::kZoomIn;
    case WebCursorInfo::kTypeZoomOut:
      return ui::CursorType::kZoomOut;
    case WebCursorInfo::kTypeGrab:
      return ui::CursorType::kGrab;
    case WebCursorInfo::kTypeGrabbing:
      return ui::CursorType::kGrabbing;
    case WebCursorInfo::kTypeCustom: {
      ui::Cursor cursor(ui::CursorType::kCustom);
      cursor.SetPlatformCursor(GetPlatformCursor());
      SkBitmap bitmap;
      gfx::Point hotspot;
      CreateScaledBitmapAndHotspotFromCustomData(&bitmap, &hotspot);
      cursor.set_custom_bitmap(bitmap);
      cursor.set_custom_hotspot(hotspot);
      return cursor;
    }
    default:
      NOTREACHED();
      return gfx::kNullCursor;
  }
}

float WebCursor::GetCursorScaleFactor() {
  DCHECK(custom_scale_ != 0);
  return device_scale_factor_ / custom_scale_;
}

void WebCursor::CreateScaledBitmapAndHotspotFromCustomData(
    SkBitmap* bitmap,
    gfx::Point* hotspot) {
  if (custom_data_.empty())
    return;
  ImageFromCustomData(bitmap);
  *hotspot = hotspot_;
  ui::ScaleAndRotateCursorBitmapAndHotpoint(
      GetCursorScaleFactor(), display::Display::ROTATE_0, bitmap, hotspot);
}

// ozone has its own SetDisplayInfo that takes rotation into account
#if !defined(USE_OZONE)
void WebCursor::SetDisplayInfo(const display::Display& display) {
  if (device_scale_factor_ == display.device_scale_factor())
    return;

  device_scale_factor_ = display.device_scale_factor();
  CleanupPlatformData();
}
#endif

}  // namespace content
