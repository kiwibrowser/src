// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ui_base_features.h"

#include "ui/base/ui_base_switches_util.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace features {

// If enabled, the emoji picker context menu item may be shown for editable
// text areas.
const base::Feature kEnableEmojiContextMenu{"EnableEmojiContextMenu",
                                            base::FEATURE_DISABLED_BY_DEFAULT};

// Enables the floating virtual keyboard behavior.
const base::Feature kEnableFloatingVirtualKeyboard = {
    "enable-floating-virtual-keyboard", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables the full screen handwriting virtual keyboard behavior.
const base::Feature kEnableFullscreenHandwritingVirtualKeyboard = {
    "enable-fullscreen-handwriting-virtual-keyboard",
    base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kEnableStylusVirtualKeyboard = {
    "enable-stylus-virtual-keyboard", base::FEATURE_DISABLED_BY_DEFAULT};

// Applies the material design mode to elements throughout Chrome (not just top
// Chrome).
const base::Feature kSecondaryUiMd = {"SecondaryUiMd",
// Enabled by default on Windows, Mac and Desktop Linux.
// http://crbug.com/775847.
#if defined(OS_WIN) || defined(OS_MACOSX) || \
    (defined(OS_LINUX) && !defined(OS_CHROMEOS))
                                      base::FEATURE_ENABLED_BY_DEFAULT
#else
                                      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

// Allows system keyboard event capture when |features::kKeyboardLockApi| is on.
const base::Feature kSystemKeyboardLock{"SystemKeyboardLock",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kTouchableAppContextMenu = {
    "EnableTouchableAppContextMenu", base::FEATURE_DISABLED_BY_DEFAULT};

bool IsTouchableAppContextMenuEnabled() {
  return base::FeatureList::IsEnabled(kTouchableAppContextMenu) ||
         switches::IsTouchableAppContextMenuEnabled();
}

// Enables GPU rasterization for all UI drawing (where not blacklisted).
const base::Feature kUiGpuRasterization = {"UiGpuRasterization",
                                           base::FEATURE_DISABLED_BY_DEFAULT};

bool IsUiGpuRasterizationEnabled() {
  return base::FeatureList::IsEnabled(kUiGpuRasterization);
}

// Enables scrolling with layers under ui using the ui::Compositor.
const base::Feature kUiCompositorScrollWithLayers = {
    "UiCompositorScrollWithLayers",
// TODO(https://crbug.com/615948): Use composited scrolling on all platforms.
#if defined(OS_MACOSX)
    base::FEATURE_ENABLED_BY_DEFAULT
#else
    base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

#if defined(OS_WIN)
// Enables stylus appearing as touch when in contact with digitizer.
const base::Feature kDirectManipulationStylus = {
    "DirectManipulationStylus", base::FEATURE_ENABLED_BY_DEFAULT};

// Enables InputPane API for controlling on screen keyboard.
const base::Feature kInputPaneOnScreenKeyboard = {
    "InputPaneOnScreenKeyboard", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables using WM_POINTER instead of WM_TOUCH for touch events.
const base::Feature kPointerEventsForTouch = {"PointerEventsForTouch",
                                              base::FEATURE_ENABLED_BY_DEFAULT};
// Enables using TSF (over IMM32) for IME.
const base::Feature kTSFImeSupport = {"TSFImeSupport",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

bool IsUsingWMPointerForTouch() {
  return base::win::GetVersion() >= base::win::VERSION_WIN8 &&
         base::FeatureList::IsEnabled(kPointerEventsForTouch);
}

// Enables DirectManipulation API for processing Precision Touchpad events.
const base::Feature kPrecisionTouchpad{"PrecisionTouchpad",
                                       base::FEATURE_ENABLED_BY_DEFAULT};
// Enables Swipe left/right to navigation back/forward API for processing
// Precision Touchpad events.
const base::Feature kPrecisionTouchpadScrollPhase{
    "PrecisionTouchpadScrollPhase", base::FEATURE_ENABLED_BY_DEFAULT};
#endif  // defined(OS_WIN)

// Used to have ash (Chrome OS system UI) run in its own process.
// TODO(jamescook): Make flag only available in Chrome OS.
const base::Feature kMash = {"Mash", base::FEATURE_DISABLED_BY_DEFAULT};

bool IsMashEnabled() {
  return base::FeatureList::IsEnabled(features::kMash);
}

#if defined(OS_MACOSX)
// When enabled, the NSWindows for apps will be created in the app's process,
// and will forward input to the browser process.
const base::Feature kHostWindowsInAppShimProcess{
    "HostWindowsInAppShimProcess", base::FEATURE_DISABLED_BY_DEFAULT};

bool HostWindowsInAppShimProcess() {
  return base::FeatureList::IsEnabled(kHostWindowsInAppShimProcess);
}

#if BUILDFLAG(MAC_VIEWS_BROWSER)
// Causes Views browser builds to use Views browser windows by default rather
// than Cocoa browser windows.
const base::Feature kViewsBrowserWindows{"ViewsBrowserWindows",
                                         base::FEATURE_DISABLED_BY_DEFAULT};

// Returns whether a Views-capable browser build should use the Cocoa browser
// UI.
bool IsViewsBrowserCocoa() {
  return !base::FeatureList::IsEnabled(kViewsBrowserWindows);
}
#endif  //  BUILDFLAG(MAC_VIEWS_BROWSER)
#endif  //  defined(OS_MACOSX)

}  // namespace features
