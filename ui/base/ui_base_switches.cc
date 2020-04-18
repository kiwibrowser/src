// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ui_base_switches.h"

namespace switches {

#if defined(OS_MACOSX) && !defined(OS_IOS)
// Disable use of AVFoundation to draw video content.
const char kDisableAVFoundationOverlays[] = "disable-avfoundation-overlays";

// Fall back to using CAOpenGLLayers display content, instead of the IOSurface
// based overlay display path.
const char kDisableMacOverlays[] = "disable-mac-overlays";

// Disable animations for showing and hiding modal dialogs.
const char kDisableModalAnimations[] = "disable-modal-animations";

// Disable use of cross-process CALayers to display content directly from the
// GPU process on Mac.
const char kDisableRemoteCoreAnimation[] = "disable-remote-core-animation";

// Show borders around CALayers corresponding to overlays and partial damage.
const char kShowMacOverlayBorders[] = "show-mac-overlay-borders";
#endif

// Disables layer-edge anti-aliasing in the compositor.
const char kDisableCompositedAntialiasing[] = "disable-composited-antialiasing";

// Disables use of DWM composition for top level windows.
const char kDisableDwmComposition[] = "disable-dwm-composition";

// Disables touch adjustment.
const char kDisableTouchAdjustment[] = "disable-touch-adjustment";

// Disables touch event based drag and drop.
const char kDisableTouchDragDrop[] = "disable-touch-drag-drop";

// Enables touch event based drag and drop.
const char kEnableTouchDragDrop[] = "enable-touch-drag-drop";

// Enables touchable app context menus and notification indicators.
const char kEnableTouchableAppContextMenu[] =
    "enable-touchable-app-context-menus";

// Forces high-contrast mode in native UI drawing, regardless of system
// settings. Note that this has limited effect on Windows: only Aura colors will
// be switched to high contrast, not other system colors.
const char kForceHighContrast[] = "force-high-contrast";

// The language file that we want to try to open. Of the form
// language[-country] where language is the 2 letter code from ISO-639.
const char kLang[] = "lang";

// Defines the speed of Material Design visual feedback animations.
const char kMaterialDesignInkDropAnimationSpeed[] =
    "material-design-ink-drop-animation-speed";

// Defines that Material Design visual feedback animations should be fast.
const char kMaterialDesignInkDropAnimationSpeedFast[] = "fast";

// Defines that Material Design visual feedback animations should be slow.
const char kMaterialDesignInkDropAnimationSpeedSlow[] = "slow";

// Enables top Chrome material design elements.
const char kTopChromeMD[] = "top-chrome-md";

// Material design mode for the |kTopChromeMD| switch.
const char kTopChromeMDMaterial[] = "material";

// Auto-switching mode |kTopChromeMD| switch. This mode toggles between
// material and material-hybrid depending on device.
const char kTopChromeMDMaterialAuto[] = "material-auto";

// Material design hybrid mode for the |kTopChromeMD| switch. Targeted for
// mouse/touch hybrid devices.
const char kTopChromeMDMaterialHybrid[] = "material-hybrid";

// Material design mode that is more optimized for touch devices for the
// |kTopChromeMD| switch.
const char kTopChromeMDMaterialTouchOptimized[] = "material-touch-optimized";

// Material design mode that represents a refresh of the Chrome UI for the
// |kTopChromeMD| switch.
const char kTopChromeMDMaterialRefresh[] = "material-refresh";

// Material design mode that represents a touchable version of material-refresh
// for the |kTopChromeMD| switch.
const char kTopChromeMDMaterialRefreshTouchOptimized[] =
    "material-refresh-touch-optimized";

// Disable partial swap which is needed for some OpenGL drivers / emulators.
const char kUIDisablePartialSwap[] = "ui-disable-partial-swap";

// Visualize overdraw by color-coding elements based on if they have other
// elements drawn underneath. This is good for showing where the UI might be
// doing more rendering work than necessary. The colors are hinting at the
// amount of overdraw on your screen for each pixel, as follows:
//
// True color: No overdraw.
// Blue: Overdrawn once.
// Green: Overdrawn twice.
// Pink: Overdrawn three times.
// Red: Overdrawn four or more times.
const char kShowOverdrawFeedback[] = "show-overdraw-feedback";

// Disable re-use of non-exact resources to fulfill ResourcePool requests.
// Intended only for use in layout or pixel tests to reduce noise.
const char kDisallowNonExactResourceReuse[] =
    "disallow-non-exact-resource-reuse";

// Transform localized strings to be longer, with beginning and end markers to
// make truncation visually apparent.
const char kMangleLocalizedStrings[] = "mangle-localized-strings";

// Re-draw everything multiple times to simulate a much slower machine.
// Give a slow down factor to cause renderer to take that many times longer to
// complete, such as --slow-down-compositing-scale-factor=2.
const char kSlowDownCompositingScaleFactor[] =
    "slow-down-compositing-scale-factor";

// Tint GL-composited color.
const char kTintGlCompositedContent[] = "tint-gl-composited-content";

}  // namespace switches
