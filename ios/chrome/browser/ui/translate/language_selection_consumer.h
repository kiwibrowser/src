// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_CONSUMER_H_

#import <Foundation/Foundation.h>

@protocol LanguageSelectionProvider;

// Consumer protocol for a view controller providing a language selection
// interface.
@protocol LanguageSelectionConsumer
// The language provider that the consumer should use to fetch language
// information for display.
@property(nonatomic, weak) id<LanguageSelectionProvider> provider;
// The number of languages available for display in the interface.
@property(nonatomic) int languageCount;
// The index of the initially selected language.
@property(nonatomic) int initialLanguageIndex;
// The index of a language unavailable for selection (because it has already
// been selected, for example).
@property(nonatomic) int disabledLanguageIndex;
@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_CONSUMER_H_
