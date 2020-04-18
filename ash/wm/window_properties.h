// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WINDOW_PROPERTIES_H_
#define ASH_WM_WINDOW_PROPERTIES_H_

#include "ash/ash_export.h"
#include "ui/base/class_property.h"
#include "ui/base/ui_base_types.h"

namespace aura {
template <typename T>
using WindowProperty = ui::ClassProperty<T>;
}

namespace ash {
namespace wm {
class WindowState;
}  // namespace wm

// Used with kWidgetCreationType to indicate source of the widget creation.
enum class WidgetCreationType {
  // The widget was created internally, and not at the request of a client.
  // For example, overview mode creates a number of widgets. These widgets are
  // created with a type of INTERNAL. This is the default.
  INTERNAL,

  // The widget was created for a client. In other words there is a client
  // embedded in the aura::Window. For example, when Chrome creates a new
  // browser window the window manager is asked to create the aura::Window.
  // The window manager creates an aura::Window and a views::Widget to show
  // the non-client frame decorations. In this case the creation type is
  // FOR_CLIENT.
  FOR_CLIENT,
};

// Shell-specific window property keys; some keys are exported for use in tests.

// Alphabetical sort.

// If this is set to true, the window stays in the same root window even if the
// bounds outside of its root window is set.
ASH_EXPORT extern const aura::WindowProperty<bool>* const kLockedToRootKey;

// Containers with this property (true) are aligned with physical pixel
// boundary.
extern const aura::WindowProperty<bool>* const kSnapChildrenToPixelBoundary;

ASH_EXPORT extern const aura::WindowProperty<WidgetCreationType>* const
    kWidgetCreationTypeKey;

// Set to true if the window server tells us the window is janky (see
// WindowManagerDelegate::OnWmClientJankinessChanged()).
ASH_EXPORT extern const aura::WindowProperty<bool>* const kWindowIsJanky;

// A property key to store WindowState in the window. The window state
// is owned by the window.
extern const aura::WindowProperty<wm::WindowState*>* const kWindowStateKey;

// Alphabetical sort.

}  // namespace ash

#endif  // ASH_WM_WINDOW_PROPERTIES_H_
