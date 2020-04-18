// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFOBARS_BEFORE_TRANSLATE_INFOBAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_INFOBARS_BEFORE_TRANSLATE_INFOBAR_CONTROLLER_H_

#import "chrome/browser/ui/cocoa/infobars/translate_infobar_base.h"

@interface BeforeTranslateInfobarController : TranslateInfoBarControllerBase {
  base::scoped_nsobject<NSButton> alwaysTranslateButton_;
  base::scoped_nsobject<NSButton> neverTranslateButton_;
}

// Creates and initializes the alwaysTranslate and neverTranslate buttons.
- (void)initializeExtraControls;

@end

@interface BeforeTranslateInfobarController (TestingAPI)

- (NSButton*)alwaysTranslateButton;
- (NSButton*)neverTranslateButton;

@end

#endif  // CHROME_BROWSER_UI_COCOA_INFOBARS_BEFORE_TRANSLATE_INFOBAR_CONTROLLER_H_
