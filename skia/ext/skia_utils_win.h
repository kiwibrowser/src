// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_SKIA_UTILS_WIN_H_
#define SKIA_EXT_SKIA_UTILS_WIN_H_

#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkRefCnt.h"

#include "build/build_config.h"
#include <windows.h>

struct SkIRect;
struct SkPoint;
struct SkRect;
class SkSurface;
typedef unsigned long DWORD;
typedef DWORD COLORREF;
typedef struct tagPOINT POINT;
typedef struct tagRECT RECT;

namespace skia {

// Converts a Skia point to a Windows POINT.
POINT SkPointToPOINT(const SkPoint& point);

// Converts a Windows RECT to a Skia rect.
SkRect RECTToSkRect(const RECT& rect);

// Converts a Windows RECT to a Skia rect.
// Both use same in-memory format. Verified by static_assert in
// skia_utils_win.cc.
inline const SkIRect& RECTToSkIRect(const RECT& rect) {
  return reinterpret_cast<const SkIRect&>(rect);
}

// Converts a Skia rect to a Windows RECT.
// Both use same in-memory format. Verified by static_assert in
// skia_utils_win.cc.
inline const RECT& SkIRectToRECT(const SkIRect& rect) {
  return reinterpret_cast<const RECT&>(rect);
}

// Converts COLORREFs (0BGR) to the ARGB layout Skia expects.
SK_API SkColor COLORREFToSkColor(COLORREF color);

// Converts ARGB to COLORREFs (0BGR).
SK_API COLORREF SkColorToCOLORREF(SkColor color);

// Initializes the default settings and colors in a device context.
SK_API void InitializeDC(HDC context);

// Converts scale, skew, and translation to Windows format and sets it on the
// HDC.
SK_API void LoadTransformToDC(HDC dc, const SkMatrix& matrix);

// Copies |src_rect| from source into destination.
//   Takes a potentially-slower path if |is_opaque| is false.
//   Sets |transform| on source afterwards!
SK_API void CopyHDC(HDC source, HDC destination, int x, int y, bool is_opaque,
                    const RECT& src_rect, const SkMatrix& transform);

// Creates a surface writing to the pixels backing |context|'s bitmap.
// Returns null on error.
SK_API sk_sp<SkSurface> MapPlatformSurface(HDC context);

// Creates a bitmap backed by the same pixels backing the HDC's bitmap.
// Returns an empty bitmap on error.
SK_API SkBitmap MapPlatformBitmap(HDC context);

// Fills in a BITMAPINFOHEADER structure given the bitmap's size.
SK_API void CreateBitmapHeader(int width, int height, BITMAPINFOHEADER* hdr);

SK_API HBITMAP CreateHBitmap(int width, int height, bool is_opaque,
                             HANDLE shared_section = nullptr,
                             void** data = nullptr);

}  // namespace skia

#endif  // SKIA_EXT_SKIA_UTILS_WIN_H_

