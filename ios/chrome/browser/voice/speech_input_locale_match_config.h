// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_VOICE_SPEECH_INPUT_LOCALE_MATCH_CONFIG_H_
#define IOS_CHROME_BROWSER_VOICE_SPEECH_INPUT_LOCALE_MATCH_CONFIG_H_

#import <Foundation/Foundation.h>

// Object containing matching locales for unsupported regional variants, loaded
// from SpeechInputLocaleMatches.plist.
@interface SpeechInputLocaleMatchConfig : NSObject

// Access to singleton object.
+ (instancetype)sharedInstance;

// The SpeechInputLocaleMatches loaded from the plist.
@property(nonatomic, readonly) NSArray* matches;

@end

// Object describing the locale codes that match a single supported locale.
@interface SpeechInputLocaleMatch : NSObject

// Designated initializer.
- (instancetype)initWithDictionary:(NSDictionary*)matchDict
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// The locale code that should be used as the default code for the locale codes
// found in |matchingLocaleCodes|.
@property(nonatomic, readonly) NSString* matchedLocaleCode;

// The locale codes that should be matched with |matchedLocaleCode|.
@property(nonatomic, readonly) NSArray* matchingLocaleCodes;

// The languages that use |matchedLocaleCode| as a default.
@property(nonatomic, readonly) NSArray* matchingLanguages;

@end

#endif  // IOS_CHROME_BROWSER_VOICE_SPEECH_INPUT_LOCALE_MATCH_CONFIG_H_
