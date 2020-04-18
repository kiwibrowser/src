// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/ash_switches.h"

#include "base/command_line.h"

namespace ash {
namespace switches {

// Force the pointer (cursor) position to be kept inside root windows.
const char kAshConstrainPointerToRoot[] = "ash-constrain-pointer-to-root";

// Enable keyboard shortcuts useful for debugging.
const char kAshDebugShortcuts[] = "ash-debug-shortcuts";

// Enable keyboard shortcuts used by developers only.
const char kAshDeveloperShortcuts[] = "ash-dev-shortcuts";

// Disables the dimming and blur of the wallpaper on login and lock screens.
const char kAshDisableLoginDimAndBlur[] = "ash-disable-login-dim-and-blur";

// Disables a smoother animation for screen rotation.
const char kAshDisableSmoothScreenRotation[] =
    "ash-disable-smooth-screen-rotation";

// Disables the split view on tablet mode.
const char kAshDisableTabletSplitView[] = "disable-tablet-splitview";

// Disable the Touch Exploration Mode. Touch Exploration Mode will no longer be
// turned on automatically when spoken feedback is enabled when this flag is
// set.
const char kAshDisableTouchExplorationMode[] =
    "ash-disable-touch-exploration-mode";

// Enables Backbutton on frame for v1 apps.
// TODO(oshima): Remove this once the feature is launched. crbug.com/749713.
const char kAshEnableV1AppBackButton[] = "ash-enable-v1-app-back-button";

// Enable cursor motion blur.
const char kAshEnableCursorMotionBlur[] = "ash-enable-cursor-motion-blur";

// Enables key bindings to scroll magnified screen.
const char kAshEnableMagnifierKeyScroller[] =
    "ash-enable-magnifier-key-scroller";

// Enable the Night Light feature.
const char kAshEnableNightLight[] = "ash-enable-night-light";

// Enables the palette on every display, instead of only the internal one.
const char kAshEnablePaletteOnAllDisplays[] =
    "ash-enable-palette-on-all-displays";

// Enables the sidebar.
const char kAshSidebarEnabled[] = "enable-ash-sidebar";
const char kAshSidebarDisabled[] = "disable-ash-sidebar";

// Enables the observation of accelerometer events to enter tablet
// mode.  The flag is "enable-touchview" not "enable-tabletmode" as this
// is used to enable tablet mode on convertible devices.
const char kAshEnableTabletMode[] = "enable-touchview";

// Enable the wayland server.
const char kAshEnableWaylandServer[] = "enable-wayland-server";

// Enables mirrored screen.
const char kAshEnableMirroredScreen[] = "ash-enable-mirrored-screen";

// Enables display scale tray settings. This uses force-device-scale-factor flag
// to modify the dsf of the device to any non discrete value.
const char kAshEnableScaleSettingsTray[] = "ash-enable-scale-settings-tray";

// Enables the stylus tools next to the status area.
const char kAshForceEnableStylusTools[] = "force-enable-stylus-tools";

// Power button position includes the power button's physical display side and
// the percentage for power button center position to the display's
// width/height in landscape_primary screen orientation. The value is a JSON
// object containing a "position" property with the value "left", "right",
// "top", or "bottom". For "left" and "right", a "y" property specifies the
// button's center position as a fraction of the display's height (in [0.0,
// 1.0]) relative to the top of the display. For "top" and "bottom", an "x"
// property gives the position as a fraction of the display's width relative to
// the left side of the display.
const char kAshPowerButtonPosition[] = "ash-power-button-position";

// Enables required things for the selected UI mode, regardless of whether the
// Chromebook is currently in the selected UI mode.
const char kAshUiMode[] = "force-tablet-mode";

// Values for the kAshUiMode flag.
const char kAshUiModeAuto[] = "auto";
const char kAshUiModeClamshell[] = "clamshell";
const char kAshUiModeTablet[] = "touch_view";

// Hides notifications that are irrelevant to Chrome OS device factory testing,
// such as battery level updates.
const char kAshHideNotificationsForFactory[] =
    "ash-hide-notifications-for-factory";

// Enables the shelf color to be derived from the wallpaper.
const char kAshShelfColor[] = "ash-shelf-color";
const char kAshShelfColorEnabled[] = "enabled";
const char kAshShelfColorDisabled[] = "disabled";

// The color scheme to be used when the |kAshShelfColor| feature is enabled.
const char kAshShelfColorScheme[] = "ash-shelf-color-scheme";
const char kAshShelfColorSchemeLightMuted[] = "light_muted";
const char kAshShelfColorSchemeLightVibrant[] = "light_vibrant";
const char kAshShelfColorSchemeNormalMuted[] = "normal_muted";
const char kAshShelfColorSchemeNormalVibrant[] = "normal_vibrant";
const char kAshShelfColorSchemeDarkMuted[] = "dark_muted";
const char kAshShelfColorSchemeDarkVibrant[] = "dark_vibrant";

// Enables the heads-up display for tracking touch points.
const char kAshTouchHud[] = "ash-touch-hud";

// (Most) Chrome OS hardware reports ACPI power button releases correctly.
// Standard hardware reports releases immediately after presses.  If set, we
// lock the screen or shutdown the system immediately in response to a press
// instead of displaying an interactive animation.
const char kAuraLegacyPowerButton[] = "aura-legacy-power-button";

// Whether this device has an internal stylus.
const char kHasInternalStylus[] = "has-internal-stylus";

// Uses a mojo app to implement the Keyboard Shortcut Viewer feature. Exists so
// the mojo app version can be tested independently from the classic version.
const char kKeyboardShortcutViewerApp[] = "keyboard-shortcut-viewer-app";

// Draws a circle at each touch point, similar to the Android OS developer
// option "Show taps".
const char kShowTaps[] = "show-taps";

// If true, the webui lock screen wil be shown. This is deprecated and will be
// removed in the future.
const char kShowWebUiLock[] = "show-webui-lock";

// Forces the webui login implementation.
const char kShowWebUiLogin[] = "show-webui-login";

// Chromebases' touchscreens can be used to wake from suspend, unlike the
// touchscreens on other Chrome OS devices. If set, the touchscreen is kept
// enabled while the screen is off so that it can be used to turn the screen
// back on after it has been turned off for inactivity but before the system has
// suspended.
const char kTouchscreenUsableWhileScreenOff[] =
    "touchscreen-usable-while-screen-off";

// Hides all Message Center notification popups (toasts). Used for testing.
const char kSuppressMessageCenterPopups[] = "suppress-message-center-popups";

bool IsNightLightEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      kAshEnableNightLight);
}

bool IsSidebarEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kAshSidebarEnabled);
}

bool IsUsingViewsLock() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(kShowWebUiLock);
}

}  // namespace switches
}  // namespace ash
