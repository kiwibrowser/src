// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_TRANSLATE_FAKE_WEB_VIEW_TRANSLATE_CLIENT_H_
#define IOS_WEB_VIEW_INTERNAL_TRANSLATE_FAKE_WEB_VIEW_TRANSLATE_CLIENT_H_

#include <string>

#import "ios/web_view/internal/translate/web_view_translate_client.h"

namespace ios_web_view {

// Contains the list of parameters passed to |TranslatePage|.
struct TranslatePageInvocation {
  std::string source_lang;
  std::string target_lang;
  bool triggered_from_menu;
};

// Fake translate client used in unit tests.
class FakeWebViewTranslateClient : public WebViewTranslateClient {
 public:
  // |page_lang| The original language of the page.
  explicit FakeWebViewTranslateClient(web::WebState* web_state,
                                      std::string page_lang);
  ~FakeWebViewTranslateClient() override;

  // WebViewTranslateClient implementation.
  void TranslatePage(const std::string& source_lang,
                     const std::string& target_lang,
                     bool triggered_from_menu) override;
  void RevertTranslation() override;

  // Getters for ivars used for testing.
  std::string GetPageLang() { return page_lang_; }
  std::string GetCurrentLang() { return current_lang_; }
  TranslatePageInvocation GetLastTraslatePageInvocation() {
    return last_translate_page_invocation_;
  }

 private:
  std::string page_lang_;
  TranslatePageInvocation last_translate_page_invocation_;
  std::string current_lang_;
};

}  // namespace ios_web_view

#endif  // IOS_WEB_VIEW_INTERNAL_TRANSLATE_FAKE_WEB_VIEW_TRANSLATE_CLIENT_H_
