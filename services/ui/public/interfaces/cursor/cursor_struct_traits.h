// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_INTERFACES_CURSOR_CURSOR_STRUCT_TRAITS_H_
#define SERVICES_UI_PUBLIC_INTERFACES_CURSOR_CURSOR_STRUCT_TRAITS_H_

#include "services/ui/public/interfaces/cursor/cursor.mojom-shared.h"
#include "ui/base/cursor/cursor_data.h"
#include "ui/base/cursor/cursor_type.h"

namespace mojo {

template <>
struct EnumTraits<ui::mojom::CursorType, ui::CursorType> {
  static ui::mojom::CursorType ToMojom(ui::CursorType input);
  static bool FromMojom(ui::mojom::CursorType input, ui::CursorType* out);
};

template <>
struct EnumTraits<ui::mojom::CursorSize, ui::CursorSize> {
  static ui::mojom::CursorSize ToMojom(ui::CursorSize input);
  static bool FromMojom(ui::mojom::CursorSize input, ui::CursorSize* out);
};

template <>
struct StructTraits<ui::mojom::CursorDataDataView, ui::CursorData> {
  static ui::CursorType cursor_type(const ui::CursorData& c) {
    return c.cursor_type();
  }
  static const base::TimeDelta& frame_delay(const ui::CursorData& c);
  static const gfx::Point& hotspot_in_pixels(const ui::CursorData& c);
  static const std::vector<SkBitmap>& cursor_frames(const ui::CursorData& c);
  static float scale_factor(const ui::CursorData& c) {
    return c.scale_factor();
  }
  static bool Read(ui::mojom::CursorDataDataView data, ui::CursorData* out);
};

}  // namespace mojo

#endif  // SERVICES_UI_PUBLIC_INTERFACES_CURSOR_CURSOR_STRUCT_TRAITS_H_
