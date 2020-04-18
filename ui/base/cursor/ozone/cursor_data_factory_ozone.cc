// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cursor/ozone/cursor_data_factory_ozone.h"

#include "ui/base/cursor/cursor.h"

namespace ui {

namespace {

// A magic value that we store at the start of an instance.
const uint32_t kCookie = 0xF60D214C;

const uint32_t kBadCookie = 0xBADBADCC;

CursorDataOzone* ToCursorDataOzone(PlatformCursor cursor) {
  CursorDataOzone* ozone = static_cast<CursorDataOzone*>(cursor);
#if DCHECK_IS_ON()
  ozone->AssertIsACursorDataOzone();
#endif
  return ozone;
}

PlatformCursor ToPlatformCursor(CursorDataOzone* cursor) {
  return static_cast<PlatformCursor>(cursor);
}

}  // namespace

CursorDataOzone::CursorDataOzone(const ui::CursorData& data)
    : magic_cookie_(kCookie), data_(data) {}

void CursorDataOzone::AssertIsACursorDataOzone() {
  CHECK_EQ(magic_cookie_, kCookie);
}

CursorDataOzone::~CursorDataOzone() {
  magic_cookie_ = kBadCookie;
}

CursorDataFactoryOzone::CursorDataFactoryOzone() {}

CursorDataFactoryOzone::~CursorDataFactoryOzone() {}

// static
const ui::CursorData& CursorDataFactoryOzone::GetCursorData(
    PlatformCursor platform_cursor) {
  return ToCursorDataOzone(platform_cursor)->data();
}

PlatformCursor CursorDataFactoryOzone::GetDefaultCursor(CursorType type) {
  // Unlike BitmapCursorFactoryOzone, we aren't making heavyweight bitmaps, but
  // we still have to cache these forever because objects that come out of the
  // GetDefaultCursor() method aren't treated as refcounted by the ozone
  // interfaces.
  return GetDefaultCursorInternal(type).get();
}

PlatformCursor CursorDataFactoryOzone::CreateImageCursor(
    const SkBitmap& bitmap,
    const gfx::Point& hotspot,
    float bitmap_dpi) {
  CursorDataOzone* cursor = new CursorDataOzone(
      ui::CursorData(hotspot, {bitmap}, bitmap_dpi, base::TimeDelta()));
  cursor->AddRef();  // Balanced by UnrefImageCursor.
  return ToPlatformCursor(cursor);
}

PlatformCursor CursorDataFactoryOzone::CreateAnimatedCursor(
    const std::vector<SkBitmap>& bitmaps,
    const gfx::Point& hotspot,
    int frame_delay_ms,
    float bitmap_dpi) {
  CursorDataOzone* cursor = new CursorDataOzone(
      ui::CursorData(hotspot, bitmaps, bitmap_dpi,
                     base::TimeDelta::FromMilliseconds(frame_delay_ms)));
  cursor->AddRef();  // Balanced by UnrefImageCursor.
  return ToPlatformCursor(cursor);
}

void CursorDataFactoryOzone::RefImageCursor(PlatformCursor cursor) {
  ToCursorDataOzone(cursor)->AddRef();
}

void CursorDataFactoryOzone::UnrefImageCursor(PlatformCursor cursor) {
  ToCursorDataOzone(cursor)->Release();
}

scoped_refptr<CursorDataOzone> CursorDataFactoryOzone::GetDefaultCursorInternal(
    CursorType type) {
  if (type == CursorType::kNone)
    return nullptr;  // nullptr is used for hidden cursor.

  if (!default_cursors_.count(type)) {
    // We hold a ref forever because clients do not do refcounting for default
    // cursors.
    scoped_refptr<CursorDataOzone> cursor =
        base::MakeRefCounted<CursorDataOzone>(ui::CursorData(type));
    default_cursors_[type] = std::move(cursor);
  }

  // Returned owned default cursor for this type.
  return default_cursors_[type];
}

}  // namespace ui
