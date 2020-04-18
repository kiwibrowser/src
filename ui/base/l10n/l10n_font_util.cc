// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/l10n/l10n_font_util.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/font.h"

namespace ui {

int GetLocalizedContentsWidthForFont(int col_resource_id,
                                     const gfx::Font& font) {
  int chars = 0;
  base::StringToInt(l10n_util::GetStringUTF8(col_resource_id), &chars);
  int width = font.GetExpectedTextWidth(chars);
  DCHECK_GT(width, 0);
  return width;
}

int GetLocalizedContentsHeightForFont(int row_resource_id,
                                      const gfx::Font& font) {
  int lines = 0;
  base::StringToInt(l10n_util::GetStringUTF8(row_resource_id), &lines);
  int height = font.GetHeight() * lines;
  DCHECK_GT(height, 0);
  return height;
}

gfx::Size GetLocalizedContentsSizeForFont(int col_resource_id,
                                          int row_resource_id,
                                          const gfx::Font& font) {
  return gfx::Size(GetLocalizedContentsWidthForFont(col_resource_id, font),
                   GetLocalizedContentsHeightForFont(row_resource_id, font));
}

}  // namespace ui
