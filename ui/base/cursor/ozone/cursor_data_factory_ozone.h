// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CURSOR_OZONE_CURSORD_DATA_FACTORY_OZONE_H_
#define UI_BASE_CURSOR_OZONE_CURSORD_DATA_FACTORY_OZONE_H_

#include <map>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/cursor/cursor_data.h"
#include "ui/base/ui_base_export.h"
#include "ui/gfx/geometry/point.h"
#include "ui/ozone/public/cursor_factory_ozone.h"

namespace ui {

// A refcounted wrapper around a ui::CursorData to obey CursorFactoryOzone's
// refcounting interface while building ui::CursorData objects for transport
// over mojo pipes.
//
// TODO(erg): In the long term, this should go away. When //content/ switches
// from webcursor.h to use ui::CursorData directly, we should be able to get
// rid of this class which is an adaptor for the existing ozone code.
class UI_BASE_EXPORT CursorDataOzone
    : public base::RefCounted<CursorDataOzone> {
 public:
  explicit CursorDataOzone(const ui::CursorData& data);

  const ui::CursorData& data() const { return data_; }

  // Instances of CursorDataOzone are passed around as void* because of the low
  // level CursorFactoryOzone interface. Even worse, there can be multiple
  // subclasses that map to this void* type. This asserts that a magic cookie
  // that we put at the start of valid CursorDataOzone objects is correct.
  void AssertIsACursorDataOzone();

 private:
  friend class base::RefCounted<CursorDataOzone>;
  ~CursorDataOzone();

  // This is always a magic constant value. This is set in the constructor and
  // unset in the destructor.
  uint32_t magic_cookie_;

  ui::CursorData data_;

  DISALLOW_COPY_AND_ASSIGN(CursorDataOzone);
};

// CursorFactoryOzone implementation for processes which use ui::CursorDatas.
//
// Inside some sandboxed processes, we need to save all source data so it can
// be processed in a remote process. This plugs into the current ozone cursor
// creating code, and builds the cross platform mojo data structure.
class UI_BASE_EXPORT CursorDataFactoryOzone : public CursorFactoryOzone {
 public:
  CursorDataFactoryOzone();
  ~CursorDataFactoryOzone() override;

  // Converts a PlatformCursor back to a ui::CursorData.
  static const ui::CursorData& GetCursorData(PlatformCursor platform_cursor);

  // CursorFactoryOzone:
  PlatformCursor GetDefaultCursor(CursorType type) override;
  PlatformCursor CreateImageCursor(const SkBitmap& bitmap,
                                   const gfx::Point& hotspot,
                                   float bitmap_dpi) override;
  PlatformCursor CreateAnimatedCursor(const std::vector<SkBitmap>& bitmaps,
                                      const gfx::Point& hotspot,
                                      int frame_delay_ms,
                                      float bitmap_dpi) override;
  void RefImageCursor(PlatformCursor cursor) override;
  void UnrefImageCursor(PlatformCursor cursor) override;

 private:
  // Get cached BitmapCursorOzone for a default cursor.
  scoped_refptr<CursorDataOzone> GetDefaultCursorInternal(CursorType type);

  // Default cursors are cached & owned by the factory.
  std::map<CursorType, scoped_refptr<CursorDataOzone>> default_cursors_;

  DISALLOW_COPY_AND_ASSIGN(CursorDataFactoryOzone);
};

}  // namespace ui

#endif  // UI_BASE_CURSOR_OZONE_CURSORD_DATA_FACTORY_OZONE_H_
