/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/platform/web_cursor_info.h"
#include "third_party/blink/renderer/platform/cursor.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

IntPoint DetermineHotSpot(Image* image,
                          bool hot_spot_specified,
                          const IntPoint& specified_hot_spot) {
  if (image->IsNull())
    return IntPoint();

  IntRect image_rect = image->Rect();

  // Hot spot must be inside cursor rectangle.
  if (hot_spot_specified) {
    if (image_rect.Contains(specified_hot_spot)) {
      return specified_hot_spot;
    }

    return IntPoint(clampTo<int>(specified_hot_spot.X(), image_rect.X(),
                                 image_rect.MaxX() - 1),
                    clampTo<int>(specified_hot_spot.Y(), image_rect.Y(),
                                 image_rect.MaxY() - 1));
  }

  // If hot spot is not specified externally, it can be extracted from some
  // image formats (e.g. .cur).
  IntPoint intrinsic_hot_spot;
  bool image_has_intrinsic_hot_spot = image->GetHotSpot(intrinsic_hot_spot);
  if (image_has_intrinsic_hot_spot && image_rect.Contains(intrinsic_hot_spot))
    return intrinsic_hot_spot;

  // If neither is provided, use a default value of (0, 0).
  return IntPoint();
}

Cursor::Cursor(Image* image, bool hot_spot_specified, const IntPoint& hot_spot)
    : type_(kCustom),
      image_(image),
      hot_spot_(DetermineHotSpot(image, hot_spot_specified, hot_spot)),
      image_scale_factor_(1) {}

Cursor::Cursor(Image* image,
               bool hot_spot_specified,
               const IntPoint& hot_spot,
               float scale)
    : type_(kCustom),
      image_(image),
      hot_spot_(DetermineHotSpot(image, hot_spot_specified, hot_spot)),
      image_scale_factor_(scale) {}

Cursor::Cursor(Type type) : type_(type), image_scale_factor_(1) {}

Cursor::Cursor(const Cursor& other) = default;

Cursor& Cursor::operator=(const Cursor& other) {
  type_ = other.type_;
  image_ = other.image_;
  hot_spot_ = other.hot_spot_;
  image_scale_factor_ = other.image_scale_factor_;
  return *this;
}

Cursor::~Cursor() = default;

const Cursor& PointerCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kPointer));
  return c;
}

const Cursor& CrossCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kCross));
  return c;
}

const Cursor& HandCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kHand));
  return c;
}

const Cursor& MoveCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kMove));
  return c;
}

const Cursor& VerticalTextCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kVerticalText));
  return c;
}

const Cursor& CellCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kCell));
  return c;
}

const Cursor& ContextMenuCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kContextMenu));
  return c;
}

const Cursor& AliasCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kAlias));
  return c;
}

const Cursor& ZoomInCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kZoomIn));
  return c;
}

const Cursor& ZoomOutCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kZoomOut));
  return c;
}

const Cursor& CopyCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kCopy));
  return c;
}

const Cursor& NoneCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNone));
  return c;
}

const Cursor& ProgressCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kProgress));
  return c;
}

const Cursor& NoDropCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNoDrop));
  return c;
}

const Cursor& NotAllowedCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNotAllowed));
  return c;
}

const Cursor& IBeamCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kIBeam));
  return c;
}

const Cursor& WaitCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kWait));
  return c;
}

const Cursor& HelpCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kHelp));
  return c;
}

const Cursor& EastResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kEastResize));
  return c;
}

const Cursor& NorthResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthResize));
  return c;
}

const Cursor& NorthEastResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthEastResize));
  return c;
}

const Cursor& NorthWestResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthWestResize));
  return c;
}

const Cursor& SouthResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kSouthResize));
  return c;
}

const Cursor& SouthEastResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kSouthEastResize));
  return c;
}

const Cursor& SouthWestResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kSouthWestResize));
  return c;
}

const Cursor& WestResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kWestResize));
  return c;
}

const Cursor& NorthSouthResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthSouthResize));
  return c;
}

const Cursor& EastWestResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kEastWestResize));
  return c;
}

const Cursor& NorthEastSouthWestResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthEastSouthWestResize));
  return c;
}

const Cursor& NorthWestSouthEastResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthWestSouthEastResize));
  return c;
}

const Cursor& ColumnResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kColumnResize));
  return c;
}

const Cursor& RowResizeCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kRowResize));
  return c;
}

const Cursor& MiddlePanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kMiddlePanning));
  return c;
}

const Cursor& EastPanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kEastPanning));
  return c;
}

const Cursor& NorthPanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthPanning));
  return c;
}

const Cursor& NorthEastPanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthEastPanning));
  return c;
}

const Cursor& NorthWestPanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kNorthWestPanning));
  return c;
}

const Cursor& SouthPanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kSouthPanning));
  return c;
}

const Cursor& SouthEastPanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kSouthEastPanning));
  return c;
}

const Cursor& SouthWestPanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kSouthWestPanning));
  return c;
}

const Cursor& WestPanningCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kWestPanning));
  return c;
}

const Cursor& GrabCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kGrab));
  return c;
}

const Cursor& GrabbingCursor() {
  DEFINE_STATIC_LOCAL(Cursor, c, (Cursor::kGrabbing));
  return c;
}

STATIC_ASSERT_ENUM(WebCursorInfo::kTypePointer, Cursor::kPointer);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeCross, Cursor::kCross);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeHand, Cursor::kHand);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeIBeam, Cursor::kIBeam);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeWait, Cursor::kWait);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeHelp, Cursor::kHelp);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeEastResize, Cursor::kEastResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthResize, Cursor::kNorthResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthEastResize,
                   Cursor::kNorthEastResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthWestResize,
                   Cursor::kNorthWestResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeSouthResize, Cursor::kSouthResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeSouthEastResize,
                   Cursor::kSouthEastResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeSouthWestResize,
                   Cursor::kSouthWestResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeWestResize, Cursor::kWestResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthSouthResize,
                   Cursor::kNorthSouthResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeEastWestResize, Cursor::kEastWestResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthEastSouthWestResize,
                   Cursor::kNorthEastSouthWestResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthWestSouthEastResize,
                   Cursor::kNorthWestSouthEastResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeColumnResize, Cursor::kColumnResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeRowResize, Cursor::kRowResize);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeMiddlePanning, Cursor::kMiddlePanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeEastPanning, Cursor::kEastPanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthPanning, Cursor::kNorthPanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthEastPanning,
                   Cursor::kNorthEastPanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNorthWestPanning,
                   Cursor::kNorthWestPanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeSouthPanning, Cursor::kSouthPanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeSouthEastPanning,
                   Cursor::kSouthEastPanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeSouthWestPanning,
                   Cursor::kSouthWestPanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeWestPanning, Cursor::kWestPanning);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeMove, Cursor::kMove);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeVerticalText, Cursor::kVerticalText);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeCell, Cursor::kCell);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeContextMenu, Cursor::kContextMenu);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeAlias, Cursor::kAlias);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeProgress, Cursor::kProgress);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNoDrop, Cursor::kNoDrop);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeCopy, Cursor::kCopy);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNone, Cursor::kNone);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeNotAllowed, Cursor::kNotAllowed);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeZoomIn, Cursor::kZoomIn);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeZoomOut, Cursor::kZoomOut);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeGrab, Cursor::kGrab);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeGrabbing, Cursor::kGrabbing);
STATIC_ASSERT_ENUM(WebCursorInfo::kTypeCustom, Cursor::kCustom);
}  // namespace blink
