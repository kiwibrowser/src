// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_COLOR_SPACE_WIN_H_
#define UI_GFX_COLOR_SPACE_WIN_H_

#include <d3d11.h>
#include <d3d9.h>

// Must be included after d3d headers, use #if to avoid lint errors.
#if 1
#include <DXGIType.h>
#endif

// Work around bug in this header by disabling the relevant warning for it.
// https://connect.microsoft.com/VisualStudio/feedback/details/911260/dxva2api-h-in-win8-sdk-triggers-c4201-with-w4
#pragma warning(push)
#pragma warning(disable : 4201)
#include <dxva2api.h>
#pragma warning(pop)

#include "ui/gfx/color_space.h"

namespace gfx {

class COLOR_SPACE_EXPORT ColorSpaceWin {
 public:
  static DXVA2_ExtendedFormat GetExtendedFormat(const ColorSpace& color_space);
  static DXGI_COLOR_SPACE_TYPE GetDXGIColorSpace(const ColorSpace& color_space);
  static D3D11_VIDEO_PROCESSOR_COLOR_SPACE GetD3D11ColorSpace(
      const ColorSpace& color_space);
};

}  // namespace gfx
#endif
