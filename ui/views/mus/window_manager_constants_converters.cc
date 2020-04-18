// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/window_manager_constants_converters.h"

namespace mojo {

// static
ui::mojom::WindowType
TypeConverter<ui::mojom::WindowType, views::Widget::InitParams::Type>::Convert(
    views::Widget::InitParams::Type type) {
  switch (type) {
    case views::Widget::InitParams::TYPE_WINDOW:
      return ui::mojom::WindowType::WINDOW;
    case views::Widget::InitParams::TYPE_PANEL:
      return ui::mojom::WindowType::PANEL;
    case views::Widget::InitParams::TYPE_WINDOW_FRAMELESS:
      return ui::mojom::WindowType::WINDOW_FRAMELESS;
    case views::Widget::InitParams::TYPE_CONTROL:
      return ui::mojom::WindowType::CONTROL;
    case views::Widget::InitParams::TYPE_POPUP:
      return ui::mojom::WindowType::POPUP;
    case views::Widget::InitParams::TYPE_MENU:
      return ui::mojom::WindowType::MENU;
    case views::Widget::InitParams::TYPE_TOOLTIP:
      return ui::mojom::WindowType::TOOLTIP;
    case views::Widget::InitParams::TYPE_BUBBLE:
      return ui::mojom::WindowType::BUBBLE;
    case views::Widget::InitParams::TYPE_DRAG:
      return ui::mojom::WindowType::DRAG;
  }
  return ui::mojom::WindowType::POPUP;
}

}  // namespace mojo
