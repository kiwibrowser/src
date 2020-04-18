// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/voice/text_to_speech_parser.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/google_toolbox_for_mac/src/Foundation/GTMStringEncoding.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Expose internal parser function for testing.
NSData* ExtractVoiceSearchAudioDataFromPageHTML(NSString* pageHTML);

namespace {
NSString* const kValidVoiceSearchHTML =
    @"<script>(function(){var _a_tts='dGVzdGF1ZG8zMm9pbw==';var _m_tts= {}}";
NSString* const kInvalidVoiceSearchHTML = @"no TTS data";
}  // namespace

using TextToSpeechParser = PlatformTest;

TEST_F(TextToSpeechParser, ExtractAudioDataValid) {
  NSData* result =
      ExtractVoiceSearchAudioDataFromPageHTML(kValidVoiceSearchHTML);

  EXPECT_TRUE(result != nil);

  GTMStringEncoding* base64 = [GTMStringEncoding rfc4648Base64StringEncoding];
  NSData* expectedData = [base64 decode:@"dGVzdGF1ZG8zMm9pbw==" error:nullptr];
  EXPECT_TRUE([expectedData isEqualToData:result]);
}

TEST_F(TextToSpeechParser, ExtractAudioDataNotFound) {
  NSData* result =
      ExtractVoiceSearchAudioDataFromPageHTML(kInvalidVoiceSearchHTML);
  EXPECT_TRUE(result == nil);
}
