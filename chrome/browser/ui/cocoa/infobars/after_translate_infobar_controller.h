// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFOBARS_AFTER_TRANSLATE_INFOBAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_INFOBARS_AFTER_TRANSLATE_INFOBAR_CONTROLLER_H_

#import "chrome/browser/ui/cocoa/infobars/translate_infobar_base.h"

@interface AfterTranslateInfobarController : TranslateInfoBarControllerBase {
  bool autodeterminedSourceLanguage_;
  bool swappedLanugageButtons_;
}

@end

#endif  // CHROME_BROWSER_UI_COCOA_INFOBARS_AFTER_TRANSLATE_INFOBAR_CONTROLLER_H_
