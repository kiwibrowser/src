// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_WIN_DIRECT_WRITE_H_
#define UI_GFX_WIN_DIRECT_WRITE_H_

#include <dwrite.h>

#include "ui/gfx/gfx_export.h"

namespace gfx {
namespace win {

GFX_EXPORT void MaybeInitializeDirectWrite();

// Creates a DirectWrite factory.
GFX_EXPORT void CreateDWriteFactory(IDWriteFactory** factory);

}  // namespace win
}  // namespace gfx

#endif  // UI_GFX_WIN_DIRECT_WRITE_H_
