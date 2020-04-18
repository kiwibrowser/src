// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/translate/fake_web_view_translate_client.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

FakeWebViewTranslateClient::FakeWebViewTranslateClient(web::WebState* web_state,
                                                       std::string page_lang)
    : WebViewTranslateClient(web_state),
      page_lang_(page_lang),
      current_lang_(page_lang){};

FakeWebViewTranslateClient::~FakeWebViewTranslateClient(){};

void FakeWebViewTranslateClient::TranslatePage(const std::string& source_lang,
                                               const std::string& target_lang,
                                               bool triggered_from_menu) {
  last_translate_page_invocation_.source_lang = source_lang;
  last_translate_page_invocation_.target_lang = target_lang;
  last_translate_page_invocation_.triggered_from_menu = triggered_from_menu;
  current_lang_ = target_lang;
}

void FakeWebViewTranslateClient::RevertTranslation() {
  current_lang_ = page_lang_;
}

}  // namespace ios_web_view
