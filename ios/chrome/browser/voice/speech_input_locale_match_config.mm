// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/voice/speech_input_locale_match_config.h"

#import "base/mac/foundation_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Keys used in SpeechInputLocaleMatches.plist:
NSString* const kMatchedLocaleKey = @"Locale";
NSString* const kMatchingLocalesKey = @"MatchingLocales";
NSString* const kMatchingLanguagesKey = @"MatchingLanguages";

}  // namespace

#pragma mark - SpeechInputLocaleMatchConfig

@interface SpeechInputLocaleMatchConfig ()

// Loads |_matches| from config file |plistFileName|.
- (void)loadConfigFile:(NSString*)plistFileName;

@end

@implementation SpeechInputLocaleMatchConfig
@synthesize matches = _matches;

+ (instancetype)sharedInstance {
  static SpeechInputLocaleMatchConfig* matchConfig;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    matchConfig = [[SpeechInputLocaleMatchConfig alloc] init];
  });
  return matchConfig;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    [self loadConfigFile:@"SpeechInputLocaleMatches"];
  }
  return self;
}


#pragma mark - Private

- (void)loadConfigFile:(NSString*)plistFileName {
  NSString* path = [[NSBundle mainBundle] pathForResource:plistFileName
                                                   ofType:@"plist"
                                              inDirectory:@"gm-config/ANY"];
  NSArray* configData = [NSArray arrayWithContentsOfFile:path];
  NSMutableArray* matches = [NSMutableArray array];
  for (id item in configData) {
    NSDictionary* matchDict = base::mac::ObjCCastStrict<NSDictionary>(item);
    SpeechInputLocaleMatch* match =
        [[SpeechInputLocaleMatch alloc] initWithDictionary:matchDict];
    [matches addObject:match];
  }
  _matches = [matches copy];
}

@end

#pragma mark - SpeechInputLocaleMatch

@implementation SpeechInputLocaleMatch
@synthesize matchedLocaleCode = _matchedLocaleCode;
@synthesize matchingLocaleCodes = _matchingLocaleCodes;
@synthesize matchingLanguages = _matchingLanguages;

- (instancetype)initWithDictionary:(NSDictionary*)matchDict {
  if ((self = [super init])) {
    _matchedLocaleCode = [matchDict[kMatchedLocaleKey] copy];
    _matchingLocaleCodes = [matchDict[kMatchingLocalesKey] copy];
    _matchingLanguages = [matchDict[kMatchingLanguagesKey] copy];
  }
  return self;
}

@end
