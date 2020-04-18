// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_WINDOW_PROPERTIES_H_
#define ASH_PUBLIC_CPP_WINDOW_PROPERTIES_H_

#include <stdint.h>
#include <string>

#include "ash/public/cpp/ash_public_export.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/class_property.h"

namespace aura {
class PropertyConverter;
class Window;
template <typename T>
using WindowProperty = ui::ClassProperty<T>;
}

namespace gfx {
class ImageSkia;
class Rect;
}

namespace ash {

namespace mojom {
enum class WindowPinType;
enum class WindowStateType;
}

enum class BackdropWindowMode {
  kEnabled,   // The window needs a backdrop shown behind it.
  kDisabled,  // The window should never have a backdrop.
  kAuto,  // The window manager decides if the window should have a backdrop.
};

// Registers Ash's properties with the given PropertyConverter. This allows Ash
// and other services (eg. Chrome) to exchange Ash window property values.
ASH_PUBLIC_EXPORT void RegisterWindowProperties(
    aura::PropertyConverter* property_converter);

// Shell-specific window property keys for use by ash and its clients.

// Alphabetical sort.

// A property key to specify if the window should (or should not) have a
// backdrop window (typically black) that covers the desktop behind the window.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<BackdropWindowMode>* const
    kBackdropWindowMode;

// If true, will send system keys to the window for dispatch.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<bool>* const
    kCanConsumeSystemKeysKey;

// The frame's active image. Only set on themed windows.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<gfx::ImageSkia*>* const
    kFrameImageActiveKey;

// Whether the shelf should be hidden when this window is put into fullscreen.
// Exposed because some windows want to explicitly opt-out of this.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<bool>* const
    kHideShelfWhenFullscreenKey;

// If true, the window is a browser window and its tab(s) are currently being
// dragged.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<bool>* const
    kIsDraggingTabsKey;

// If true (and the window is a panel), it's attached to its shelf item.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<bool>* const
    kPanelAttachedKey;

// Maps to ui::mojom::WindowManager::kRenderParentTitleArea_Property.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<bool>* const
    kRenderTitleAreaProperty;

// A property key which stores the bounds in screen coordinates to restore a
// window to. These take preference over the current bounds. This is used by
// e.g. the tablet mode window manager.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<gfx::Rect*>* const
    kRestoreBoundsOverrideKey;

// A property key which stores the window state to restore a window to. These
// take preference over the current state if
// |kRestoreWindowStateTypeOverrideKey| is set. This is used by e.g. the tablet
// mode window manager.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<
    mojom::WindowStateType>* const kRestoreWindowStateTypeOverrideKey;

// A property key to store the id for a window's shelf item.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<std::string*>* const
    kShelfIDKey;

// A property key to store the type of a window's shelf item.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<int32_t>* const
    kShelfItemTypeKey;

// A property key to indicate whether we should hide this window in overview
// mode and Alt + Tab.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<bool>* const
    kShowInOverviewKey;

// A property key to store the address of the source window that the drag
// originated from if the window is currenlty in tab-dragging process.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<aura::Window*>* const
    kTabDraggingSourceWindowKey;

// A property key to store the active color on the window frame.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<SkColor>* const
    kFrameActiveColorKey;
// A property key to store the inactive color on the window frame.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<SkColor>* const
    kFrameInactiveColorKey;

// A property key to store ash::WindowPinType for a window.
// When setting this property to PINNED or TRUSTED_PINNED, the window manager
// will try to fullscreen the window and pin it on the top of the screen. If the
// window manager failed to do it, the property will be restored to NONE. When
// setting this property to NONE, the window manager will restore the window.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<mojom::WindowPinType>* const
    kWindowPinTypeKey;

// A property key to indicate whether ash should perform auto management of
// window positions; when you open a second browser, ash will move the two to
// minimize overlap.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<bool>* const
    kWindowPositionManagedTypeKey;

// A property key to indicate ash's extended window state.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<
    mojom::WindowStateType>* const kWindowStateTypeKey;

// Determines whether the window title should be drawn. For example, app and
// non-tabbed, trusted source windows (such as Settings) will not show a title.
ASH_PUBLIC_EXPORT extern const aura::WindowProperty<bool>* const
    kWindowTitleShownKey;

// Alphabetical sort.

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_WINDOW_PROPERTIES_H_
