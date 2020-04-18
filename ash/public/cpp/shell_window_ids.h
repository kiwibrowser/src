// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_SHELL_WINDOW_IDS_H_
#define ASH_PUBLIC_CPP_SHELL_WINDOW_IDS_H_

#include <stddef.h>
#include <stdint.h>

#include "ash/public/cpp/ash_public_export.h"

// Declarations of ids of special shell windows.

namespace ash {

enum ShellWindowId {
  // Used to indicate no shell window id.
  kShellWindowId_Invalid = -1,

  // The screen rotation container in between root window and its children, used
  // for screen rotation animation.
  kShellWindowId_ScreenRotationContainer = 0,

  // A higher-level container that holds all of the containers stacked below
  // kShellWindowId_LockScreenContainer.  Only used by PowerButtonController for
  // animating lower-level containers.
  kShellWindowId_NonLockScreenContainersContainer,

  // A higher-level container that holds containers that hold lock-screen
  // windows.  Only used by PowerButtonController for animating lower-level
  // containers.
  kShellWindowId_LockScreenContainersContainer,

  // A higher-level container that holds containers that hold
  // lock-screen-related windows (which are displayed regardless of the screen
  // lock state, effectively containers stacked above
  // kShellWindowId_LockSystemModalContainer). Used by the shelf, status area,
  // virtual keyboard, settings bubble, menus, Docked Magnifier viewport, etc.
  // Also used by the PowerButtonController for animating lower-level
  // containers.
  kShellWindowId_LockScreenRelatedContainersContainer,

  // A container used for windows of WINDOW_TYPE_CONTROL that have no parent.
  // This container is not visible.
  kShellWindowId_UnparentedControlContainer,

  // The wallpaper (desktop background) window.
  kShellWindowId_WallpaperContainer,

  // The container for standard top-level windows.
  kShellWindowId_DefaultContainer,

  // The container for top-level windows with the 'always-on-top' flag set.
  kShellWindowId_AlwaysOnTopContainer,

  // The container for the app list.
  kShellWindowId_AppListContainer,

  // The container for the app list in tablet mode.
  kShellWindowId_AppListTabletModeContainer,

  // The container for the shelf.
  kShellWindowId_ShelfContainer,

  // The container for bubbles which float over the shelf.
  kShellWindowId_ShelfBubbleContainer,

  // The container for panel windows.
  kShellWindowId_PanelContainer,

  // The container for user-specific modal windows.
  kShellWindowId_SystemModalContainer,

  // The container for the lock screen wallpaper (lock screen background).
  kShellWindowId_LockScreenWallpaperContainer,

  // The container for the lock screen.
  kShellWindowId_LockScreenContainer,

  // The container for windows that handle lock tray actions (e.g. new note
  // action). The action handler container's children should be visible on lock
  // screen, but only when an action is being handled - i.e. action handling
  // state is either:
  //  *  active - the container is stacked above LockScreenContainer
  //  *  background - the container is stacked below LockScreenContainer
  kShellWindowId_LockActionHandlerContainer,

  // The container for the lock screen modal windows.
  kShellWindowId_LockSystemModalContainer,

  // The container for the status area.
  kShellWindowId_StatusContainer,

  // A parent container that holds the virtual keyboard container and ime
  // windows if any. This is to ensure that the virtual keyboard or ime window
  // is stacked above most containers but below the mouse cursor and the power
  // off animation.
  kShellWindowId_ImeWindowParentContainer,

  // The virtual keyboard container.
  kShellWindowId_VirtualKeyboardContainer,

  // The container for menus.
  kShellWindowId_MenuContainer,

  // The container for drag/drop images and tooltips.
  kShellWindowId_DragImageAndTooltipContainer,

  // The container for the fullscreen power button menu.
  kShellWindowId_PowerMenuContainer,

  // The container for bubbles briefly overlaid onscreen to show settings
  // changes (volume, brightness, input method bubbles, etc.).
  kShellWindowId_SettingBubbleContainer,

  // Contains special accessibility windows that can inset the display work area
  // (e.g. the ChromeVox spoken feedback window).
  // TODO(jamescook): Consolidate this with DockedMagnifierContainer.
  kShellWindowId_AccessibilityPanelContainer,

  // The container for special components overlaid onscreen, such as the
  // region selector for partial screenshots.
  kShellWindowId_OverlayContainer,

  // The container for the Docked Magnifier viewport widget and the separator.
  kShellWindowId_DockedMagnifierContainer,

  // The container for mouse cursor.
  kShellWindowId_MouseCursorContainer,

  // The topmost container, used for power off animation.
  kShellWindowId_PowerButtonAnimationContainer,

  kShellWindowId_MinContainer = kShellWindowId_ScreenRotationContainer,
  kShellWindowId_MaxContainer = kShellWindowId_PowerButtonAnimationContainer,
};

// Special shell windows that are not containers.
enum NonContainerWindowId {
  // The window created by PhantomWindowController or DragWindowController.
  kShellWindowId_PhantomWindow = kShellWindowId_MaxContainer + 1
};

// A list of all the above valid container IDs. Add any new ID to this list.
// This list is needed to validate we have no duplicate IDs.
const int32_t kAllShellContainerIds[] = {
    kShellWindowId_ScreenRotationContainer,
    kShellWindowId_NonLockScreenContainersContainer,
    kShellWindowId_LockScreenContainersContainer,
    kShellWindowId_LockScreenRelatedContainersContainer,
    kShellWindowId_UnparentedControlContainer,
    kShellWindowId_WallpaperContainer,
    kShellWindowId_VirtualKeyboardContainer,
    kShellWindowId_DefaultContainer,
    kShellWindowId_AlwaysOnTopContainer,
    kShellWindowId_AppListContainer,
    kShellWindowId_AppListTabletModeContainer,
    kShellWindowId_ShelfContainer,
    kShellWindowId_ShelfBubbleContainer,
    kShellWindowId_PanelContainer,
    kShellWindowId_SystemModalContainer,
    kShellWindowId_LockScreenWallpaperContainer,
    kShellWindowId_LockScreenContainer,
    kShellWindowId_LockActionHandlerContainer,
    kShellWindowId_LockSystemModalContainer,
    kShellWindowId_StatusContainer,
    kShellWindowId_ImeWindowParentContainer,
    kShellWindowId_MenuContainer,
    kShellWindowId_DragImageAndTooltipContainer,
    kShellWindowId_PowerMenuContainer,
    kShellWindowId_SettingBubbleContainer,
    kShellWindowId_AccessibilityPanelContainer,
    kShellWindowId_OverlayContainer,
    kShellWindowId_DockedMagnifierContainer,
    kShellWindowId_MouseCursorContainer,
    kShellWindowId_PowerButtonAnimationContainer,
};

// These are the list of container ids of containers which may contain windows
// that need to be activated.
ASH_PUBLIC_EXPORT extern const int32_t kActivatableShellWindowIds[];
ASH_PUBLIC_EXPORT extern const size_t kNumActivatableShellWindowIds;

// Returns true if |id| is in |kActivatableShellWindowIds|.
ASH_PUBLIC_EXPORT bool IsActivatableShellWindowId(int32_t id);

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_SHELL_WINDOW_IDS_H_
