// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/voice/text_to_speech_listener.h"

#import "ios/web/public/web_state/web_state.h"
#include "ios/web/public/test/web_test_with_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#import "third_party/google_toolbox_for_mac/src/Foundation/GTMStringEncoding.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
NSString* const kHTMLFormat =
    @"<html><head><script>%@</script></head><body></body></html>";
NSString* const kValidVoiceSearchScript =
    @"(function(){var _a_tts='dGVzdGF1ZG8zMm9pbw==';var _m_tts= {}})";
}  // namespace

#pragma mark - TestTTSListenerDelegate

@interface TestTTSListenerDelegate : NSObject<TextToSpeechListenerDelegate> {
  // Backing objects for properties of the same name.
  NSData* _expectedAudioData;
}

// The expected audio data to be returned by the TextToSpeechListener.
@property(nonatomic, strong) NSData* expectedAudioData;

// Whether |-textToSpeechListener:didReceiveResult:| was called.
@property(nonatomic, assign) BOOL audioDataReceived;

@end

@implementation TestTTSListenerDelegate

@synthesize audioDataReceived = _audioDataReceived;

- (void)setExpectedAudioData:(NSData*)expectedAudioData {
  _expectedAudioData = expectedAudioData;
}

- (NSData*)expectedAudioData {
  return _expectedAudioData;
}

- (void)textToSpeechListener:(TextToSpeechListener*)listener
            didReceiveResult:(NSData*)result {
  EXPECT_NSEQ(self.expectedAudioData, result);
  self.audioDataReceived = YES;
}

- (void)textToSpeechListenerWebStateWasDestroyed:
    (TextToSpeechListener*)listener {
}

- (BOOL)shouldTextToSpeechListener:(TextToSpeechListener*)listener
                  parseDataFromURL:(const GURL&)URL {
  return YES;
}

@end

#pragma mark - TextToSpeechListenerTest

class TextToSpeechListenerTest : public web::WebTestWithWebState {
 public:
  void SetUp() override {
    web::WebTestWithWebState::SetUp();
    delegate_ = [[TestTTSListenerDelegate alloc] init];
    listener_ = [[TextToSpeechListener alloc] initWithWebState:web_state()
                                                      delegate:delegate_];
  }

  void TestExtraction(NSString* html, NSData* expected_audio_data) {
    [delegate_ setExpectedAudioData:expected_audio_data];
    LoadHtml(html);
    WaitForCondition(^bool {
      return [delegate_ audioDataReceived];
    });
  }

 private:
  TestTTSListenerDelegate* delegate_;
  TextToSpeechListener* listener_;
};

TEST_F(TextToSpeechListenerTest, ValidAudioDataTest) {
  GTMStringEncoding* encoder = [GTMStringEncoding rfc4648Base64StringEncoding];
  NSString* html =
      [NSString stringWithFormat:kHTMLFormat, kValidVoiceSearchScript];
  NSData* expected_audio_data =
      [encoder decode:@"dGVzdGF1ZG8zMm9pbw==" error:nullptr];
  TestExtraction(html, expected_audio_data);
}

TEST_F(TextToSpeechListenerTest, InvalidAudioDataTest) {
  TestExtraction([NSString stringWithFormat:kHTMLFormat, @""], nil);
}
