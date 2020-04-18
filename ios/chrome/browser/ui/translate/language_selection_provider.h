// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_PROVIDER_H_

#import <Foundation/Foundation.h>

// Protocol for a provider that can map indexes to language names. An
// implementer of this protocol should be consistent over its lifetime, always
// returning the same language information for a given index.
@protocol LanguageSelectionProvider

// The name of the language (in the application's locale) of the language at
// index |languageIndex|, or nil if |languageIndex| is outside of the range
// of indices handled by the implementer.
- (NSString*)languageNameAtIndex:(int)languageIndex;

@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_PROVIDER_H_
