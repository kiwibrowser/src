// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABS_REQUIREMENTS_TAB_STRIP_PLACEHOLDER_H_
#define IOS_CHROME_BROWSER_UI_TABS_REQUIREMENTS_TAB_STRIP_PLACEHOLDER_H_

#include "base/ios/block_types.h"

// Objects that conform to this protocol are able to perform a fold and unfold
// animation.
@protocol TabStripFoldAnimation

// Triggers a fold animation resulting in hiding all the tabs views.
// |completion| is called at the end of the animation.
- (void)foldWithCompletion:(ProceduralBlock)completion;

// Triggers an unfold animation resulting in showing the tabs views.
// |completion| is called at the end of the animation.
- (void)unfoldWithCompletion:(ProceduralBlock)completion;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABS_REQUIREMENTS_TAB_STRIP_PLACEHOLDER_H_
