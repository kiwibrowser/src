// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_DELEGATE_H_
#define IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_DELEGATE_H_

#import <Foundation/Foundation.h>
#include <string>

// Protocol adopted by an object that can receive language selection information
// from the UI layer.
@protocol LanguageSelectionDelegate

// Tells the delegate that the language identified by |languageCode| was
// selected and the language selector was closed.
- (void)languageSelectorSelectedLanguage:(std::string)languageCode;

// Tells the delegate that the language selector was closed without a selection
// being made.
- (void)languageSelectorClosedWithoutSelection;

@end

#endif  // IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_DELEGATE_H_
