// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_ASH_FEATURES_H_
#define ASH_PUBLIC_CPP_ASH_FEATURES_H_

#include "ash/public/cpp/ash_public_export.h"
#include "base/feature_list.h"

namespace ash {
namespace features {

// Enables the keyboard shortcut to move active window between displays.
// TODO(warx): Remove this after the feature is fully launched.
// https://crbug.com/773749.
ASH_PUBLIC_EXPORT extern const base::Feature kDisplayMoveWindowAccels;

// Enables the docked (a.k.a. picture-in-picture) magnifier.
// TODO(afakhry): Remove this after the feature is fully launched.
// https://crbug.com/709824.
ASH_PUBLIC_EXPORT extern const base::Feature kDockedMagnifier;

// Enables dragging one or more tabs out of a browser window in tablet mode.
// TODO(xdai): Remove this after the feature is launched.
// https://crbug.com/823769.
ASH_PUBLIC_EXPORT extern const base::Feature kDragTabsInTabletMode;

// Enables the keyboard shortcut viewer.
// TODO(wutao): Remove this after the feature is fully launched.
// https://crbug.com/755448.
ASH_PUBLIC_EXPORT extern const base::Feature kKeyboardShortcutViewer;

// Enables notifications on the lock screen.
ASH_PUBLIC_EXPORT extern const base::Feature kLockScreenNotifications;

// Enables the new overview animations.
// TODO(wutao): Remove this after the feature is fully launched.
// https://crbug.com/801465.
ASH_PUBLIC_EXPORT extern const base::Feature kNewOverviewAnimations;

// Enables swipe to close in overview mode.
// TODO(sammiequon): Remove this after the feature is fully launched.
// https://crbug.com/828646.
ASH_PUBLIC_EXPORT extern const base::Feature kOverviewSwipeToClose;

// Enables persistent window bounds in multi-displays scenario.
// TODO(warx): Remove this after the feature is fully launched.
// https://crbug.com/805046.
ASH_PUBLIC_EXPORT extern const base::Feature kPersistentWindowBounds;

// Enables new system menu.
ASH_PUBLIC_EXPORT extern const base::Feature kSystemTrayUnified;

// Enables the mojo app implementation of 'Show taps'.
ASH_PUBLIC_EXPORT extern const base::Feature kTapVisualizerApp;

// Enables trilinear filtering.
ASH_PUBLIC_EXPORT extern const base::Feature kTrilinearFiltering;

// Enables views login.
ASH_PUBLIC_EXPORT extern const base::Feature kViewsLogin;

ASH_PUBLIC_EXPORT bool IsDisplayMoveWindowAccelsEnabled();

ASH_PUBLIC_EXPORT bool IsDockedMagnifierEnabled();

ASH_PUBLIC_EXPORT bool IsKeyboardShortcutViewerEnabled();

ASH_PUBLIC_EXPORT bool IsLockScreenNotificationsEnabled();

ASH_PUBLIC_EXPORT bool IsPersistentWindowBoundsEnabled();

ASH_PUBLIC_EXPORT bool IsSystemTrayUnifiedEnabled();

ASH_PUBLIC_EXPORT bool IsTrilinearFilteringEnabled();

ASH_PUBLIC_EXPORT bool IsViewsLoginEnabled();

}  // namespace features
}  // namespace ash

#endif  // ASH_PUBLIC_CPP_ASH_FEATURES_H_
