// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_L10N_FONT_UTIL_H_
#define UI_BASE_L10N_FONT_UTIL_H_

#include "ui/base/ui_base_export.h"
#include "ui/gfx/geometry/size.h"

namespace gfx {
class Font;
}

namespace ui {

// Returns the preferred size of the contents view of a window based on
// its localized size data and the given font. The width in cols is held in a
// localized string resource identified by |col_resource_id|, the height in the
// same fashion.
UI_BASE_EXPORT int GetLocalizedContentsWidthForFont(int col_resource_id,
                                                    const gfx::Font& font);
UI_BASE_EXPORT int GetLocalizedContentsHeightForFont(int row_resource_id,
                                                     const gfx::Font& font);
UI_BASE_EXPORT gfx::Size GetLocalizedContentsSizeForFont(int col_resource_id,
                                                         int row_resource_id,
                                                         const gfx::Font& font);

}  // namespace ui

#endif  // UI_BASE_L10N_FONT_UTIL_H_
