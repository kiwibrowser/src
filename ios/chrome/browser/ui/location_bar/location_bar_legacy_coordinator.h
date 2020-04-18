// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_LEGACY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_LEGACY_COORDINATOR_H_

#import "ios/chrome/browser/ui/location_bar/location_bar_generic_coordinator.h"

// Location bar coordinator for the pre-UI Refresh
@interface LocationBarLegacyCoordinator
    : NSObject<LocationBarGenericCoordinator>

// Perform animations for expanding the omnibox. This animation can be seen on
// an iPhone when the omnibox is focused. It involves sliding the leading button
// out and fading its alpha.
// The trailing button is faded-in in the |completionAnimator| animations.
- (void)addExpandOmniboxAnimations:(UIViewPropertyAnimator*)animator
                completionAnimator:(UIViewPropertyAnimator*)completionAnimator;

// Perform animations for expanding the omnibox. This animation can be seen on
// an iPhone when the omnibox is defocused. It involves sliding the leading
// button in and fading its alpha.
- (void)addContractOmniboxAnimations:(UIViewPropertyAnimator*)animator;

// Focuses omnibox with fakebox as the focus event source.
- (void)focusOmniboxFromFakebox;

// Updates omnibox state, including the displayed text and the cursor position.
- (void)updateOmniboxState;

@end

#endif  // IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_LEGACY_COORDINATOR_H_
