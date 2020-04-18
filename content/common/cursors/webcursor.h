// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_CURSORS_WEBCURSOR_H_
#define CONTENT_COMMON_CURSORS_WEBCURSOR_H_

#include <vector>

#include "build/build_config.h"
#include "content/common/content_export.h"
#include "content/public/common/cursor_info.h"
#include "ui/display/display.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"

#if defined(USE_AURA)
#include "ui/base/cursor/cursor.h"
#endif

#if defined(OS_WIN)
typedef struct HINSTANCE__* HINSTANCE;
typedef struct HICON__* HICON;
typedef HICON HCURSOR;
#elif defined(OS_MACOSX)
#ifdef __OBJC__
@class NSCursor;
#else
class NSCursor;
#endif
#endif

namespace base {
class Pickle;
class PickleIterator;
}

namespace content {

// This class encapsulates a cross-platform description of a cursor.  Platform
// specific methods are provided to translate the cross-platform cursor into a
// platform specific cursor.  It is also possible to serialize / de-serialize a
// WebCursor.
class CONTENT_EXPORT WebCursor {
 public:
  WebCursor();
  ~WebCursor();

  // Copy constructor/assignment operator combine.
  WebCursor(const WebCursor& other);
  const WebCursor& operator=(const WebCursor& other);

  // Conversion from/to CursorInfo.
  void InitFromCursorInfo(const CursorInfo& cursor_info);
  void GetCursorInfo(CursorInfo* cursor_info) const;

  // Serialization / De-serialization
  bool Deserialize(base::PickleIterator* iter);
  void Serialize(base::Pickle* pickle) const;

  // Returns true if GetCustomCursor should be used to allocate a platform
  // specific cursor object.  Otherwise GetCursor should be used.
  bool IsCustom() const;

  // Returns true if the current cursor object contains the same cursor as the
  // cursor object passed in. If the current cursor is a custom cursor, we also
  // compare the bitmaps to verify whether they are equal.
  bool IsEqual(const WebCursor& other) const;

  // Returns a native cursor representing the current WebCursor instance.
  gfx::NativeCursor GetNativeCursor();

#if defined(USE_AURA)
  ui::PlatformCursor GetPlatformCursor();

  // Updates |device_scale_factor_| and |rotation_| based on |display|.
  void SetDisplayInfo(const display::Display& display);

  float GetCursorScaleFactor();

  void CreateScaledBitmapAndHotspotFromCustomData(
      SkBitmap* bitmap,
      gfx::Point* hotspot);

#elif defined(OS_WIN)
  // Returns a HCURSOR representing the current WebCursor instance.
  // The ownership of the HCURSOR remains with the WebCursor instance.
  HCURSOR GetCursor(HINSTANCE module_handle);

#elif defined(OS_MACOSX)
  // Initialize this from the given Cocoa NSCursor.
  void InitFromNSCursor(NSCursor* cursor);
#endif

 private:
  // Copies the contents of the WebCursor instance passed in.
  void Copy(const WebCursor& other);

  // Cleans up the WebCursor instance.
  void Clear();

  // Platform specific initialization goes here.
  void InitPlatformData();

  // Returns true if the platform data in the current cursor object
  // matches that of the cursor passed in.
  bool IsPlatformDataEqual(const WebCursor& other) const ;

  // Copies platform specific data from the WebCursor instance passed in.
  void CopyPlatformData(const WebCursor& other);

  // Platform specific cleanup.
  void CleanupPlatformData();

  void SetCustomData(const SkBitmap& image);

  // Fills the custom_data vector and custom_size object with the image data
  // taken from the bitmap.
  void CreateCustomData(const SkBitmap& bitmap,
                        std::vector<char>* custom_data,
                        gfx::Size* custom_size);

  void ImageFromCustomData(SkBitmap* image) const;

  // Clamp the hotspot to the custom image's bounds, if this is a custom cursor.
  void ClampHotspot();

  // WebCore::PlatformCursor type.
  int type_;

  // Hotspot in cursor image in pixels.
  gfx::Point hotspot_;

  // Custom cursor data, as 32-bit RGBA.
  // Platform-inspecific because it can be serialized.
  gfx::Size custom_size_;  // In pixels.
  float custom_scale_;
  std::vector<char> custom_data_;

#if defined(USE_AURA) && (defined(USE_X11) || defined(USE_OZONE))
  // Only used for custom cursors.
  ui::PlatformCursor platform_cursor_;
#elif defined(OS_WIN)
  // A custom cursor created from custom bitmap data by Webkit.
  HCURSOR custom_cursor_;
#endif
#if defined(USE_AURA)
  float device_scale_factor_;
#endif

#if defined(USE_OZONE)
  display::Display::Rotation rotation_;
  gfx::Size maximum_cursor_size_;
#endif
};

}  // namespace content

#endif  // CONTENT_COMMON_CURSORS_WEBCURSOR_H_
