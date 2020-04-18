// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_features.h"

namespace arc {

// Controls whether ARC is available for CHILD accounts.
const base::Feature kAvailableForChildAccountFeature{
    "ArcAvailableForChildAccount", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls ACTION_BOOT_COMPLETED broadcast for third party applications on ARC.
// When disabled, third party apps will not receive this broadcast.
const base::Feature kBootCompletedBroadcastFeature {
    "ArcBootCompletedBroadcast", base::FEATURE_ENABLED_BY_DEFAULT
};

// Controls whether we should delete all ARC data before transitioning a user
// from regular to child account.
const base::Feature kCleanArcDataOnRegularToChildTransitionFeature{
    "ArcCleanDataOnRegularToChildTransition", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether ARC handles child->regular account transition.
const base::Feature kEnableChildToRegularTransitionFeature{
    "ArcEnableChildToRegularTransition", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls whether ARC handles regular->child account transition.
const base::Feature kEnableRegularToChildTransitionFeature{
    "ArcEnableRegularToChildTransition", base::FEATURE_ENABLED_BY_DEFAULT};

// Controls experimental native bridge feature for ARC.
const base::Feature kNativeBridgeExperimentFeature {
    "ArcNativeBridgeExperiment", base::FEATURE_ENABLED_BY_DEFAULT
};

// Controls ARC USB host integration.
// When enabled, Android apps will be able to use usb host features.
const base::Feature kUsbHostFeature{"ArcUsbHost",
                                    base::FEATURE_ENABLED_BY_DEFAULT};

// Controls ARC VPN integration.
// When enabled, Chrome traffic will be routed through VPNs connected in
// Android apps.
const base::Feature kVpnFeature{"ArcVpn", base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace arc
