// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/translate/ios/browser/translate_controller.h"

#include <memory>

#include "base/values.h"
#import "components/translate/ios/browser/js_translate_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace translate {

class TranslateControllerTest : public PlatformTest,
                                public TranslateController::Observer {
 protected:
  TranslateControllerTest()
      : test_web_state_(new web::TestWebState),
        success_(false),
        ready_time_(0),
        load_time_(0),
        translation_time_(0),
        on_script_ready_called_(false),
        on_translate_complete_called_(false) {
    mock_js_translate_manager_ =
        [OCMockObject niceMockForClass:[JsTranslateManager class]];
    translate_controller_ = std::make_unique<TranslateController>(
        test_web_state_.get(), mock_js_translate_manager_);
    translate_controller_->set_observer(this);
  }

  // TranslateController::Observer methods.
  void OnTranslateScriptReady(bool success,
                              double load_time,
                              double ready_time) override {
    on_script_ready_called_ = true;
    success_ = success;
    load_time_ = load_time;
    ready_time_ = ready_time;
  }

  void OnTranslateComplete(bool success,
                           const std::string& original_language,
                           double translation_time) override {
    on_translate_complete_called_ = true;
    success_ = success;
    original_language_ = original_language;
    translation_time_ = translation_time;
  }

  std::unique_ptr<web::TestWebState> test_web_state_;
  id mock_js_translate_manager_;
  std::unique_ptr<TranslateController> translate_controller_;
  bool success_;
  double ready_time_;
  double load_time_;
  std::string original_language_;
  double translation_time_;
  bool on_script_ready_called_;
  bool on_translate_complete_called_;
};

// Tests that OnJavascriptCommandReceived() returns false to malformed commands.
TEST_F(TranslateControllerTest, OnJavascriptCommandReceived) {
  base::DictionaryValue malformed_command;
  EXPECT_FALSE(translate_controller_->OnJavascriptCommandReceived(
      malformed_command, GURL("http://google.com"), false));
}

// Tests that OnTranslateScriptReady() is called when a timeout message is
// recieved from the JS side.
TEST_F(TranslateControllerTest, OnTranslateScriptReadyTimeoutCalled) {
  base::DictionaryValue command;
  command.SetString("command", "translate.ready");
  command.SetBoolean("timeout", true);
  command.SetDouble("loadTime", .0);
  command.SetDouble("readyTime", .0);
  EXPECT_TRUE(translate_controller_->OnJavascriptCommandReceived(
      command, GURL("http://google.com"), false));
  EXPECT_TRUE(on_script_ready_called_);
  EXPECT_FALSE(on_translate_complete_called_);
  EXPECT_FALSE(success_);
}

// Tests that OnTranslateScriptReady() is called with the right parameters when
// a |translate.ready| message is recieved from the JS side.
TEST_F(TranslateControllerTest, OnTranslateScriptReadyCalled) {
  // Arbitrary values.
  double some_load_time = 23.1;
  double some_ready_time = 12.2;

  base::DictionaryValue command;
  command.SetString("command", "translate.ready");
  command.SetBoolean("timeout", false);
  command.SetDouble("loadTime", some_load_time);
  command.SetDouble("readyTime", some_ready_time);
  EXPECT_TRUE(translate_controller_->OnJavascriptCommandReceived(
      command, GURL("http://google.com"), false));
  EXPECT_TRUE(on_script_ready_called_);
  EXPECT_FALSE(on_translate_complete_called_);
  EXPECT_TRUE(success_);
  EXPECT_EQ(some_load_time, load_time_);
  EXPECT_EQ(some_ready_time, ready_time_);
}

// Tests that OnTranslateComplete() is called with the right parameters when a
// |translate.status| message is recieved from the JS side.
TEST_F(TranslateControllerTest, TranslationSuccess) {
  // Arbitrary values.
  std::string some_original_language("en");
  double some_translation_time = 12.9;

  base::DictionaryValue command;
  command.SetString("command", "translate.status");
  command.SetBoolean("success", true);
  command.SetString("originalPageLanguage", some_original_language);
  command.SetDouble("translationTime", some_translation_time);
  EXPECT_TRUE(translate_controller_->OnJavascriptCommandReceived(
      command, GURL("http://google.com"), false));
  EXPECT_FALSE(on_script_ready_called_);
  EXPECT_TRUE(on_translate_complete_called_);
  EXPECT_TRUE(success_);
  EXPECT_EQ(some_original_language, original_language_);
  EXPECT_EQ(some_translation_time, translation_time_);
}

// Tests that OnTranslateComplete() is called with the right parameters when a
// |translate.status| message is recieved from the JS side.
TEST_F(TranslateControllerTest, TranslationFailure) {
  base::DictionaryValue command;
  command.SetString("command", "translate.status");
  command.SetBoolean("success", false);
  EXPECT_TRUE(translate_controller_->OnJavascriptCommandReceived(
      command, GURL("http://google.com"), false));
  EXPECT_FALSE(on_script_ready_called_);
  EXPECT_TRUE(on_translate_complete_called_);
  EXPECT_FALSE(success_);
}

}  // namespace translate
