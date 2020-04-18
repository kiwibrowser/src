// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_WINDOW_MANAGER_CONSTANTS_CONVERTERS_H_
#define UI_VIEWS_MUS_WINDOW_MANAGER_CONSTANTS_CONVERTERS_H_

#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/views/mus/mus_export.h"
#include "ui/views/widget/widget.h"

namespace mojo {

template <>
struct VIEWS_MUS_EXPORT
    TypeConverter<ui::mojom::WindowType, views::Widget::InitParams::Type> {
  static ui::mojom::WindowType Convert(views::Widget::InitParams::Type type);
};

}  // namespace mojo

#endif  // UI_VIEWS_MUS_WINDOW_MANAGER_CONSTANTS_CONVERTERS_H_
