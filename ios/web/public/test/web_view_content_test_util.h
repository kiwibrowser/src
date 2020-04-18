// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_WEB_VIEW_CONTENT_TEST_UTIL_H_
#define IOS_WEB_PUBLIC_TEST_WEB_VIEW_CONTENT_TEST_UTIL_H_

#import "ios/web/public/web_state/web_state.h"

namespace web {
namespace test {

// Enum describing loaded/blocked state of an image html element.
enum ImageStateElement {
  // Element was not loaded by WebState.
  IMAGE_STATE_BLOCKED = 1,
  // Element was fullt loaded by WebState.
  IMAGE_STATE_LOADED,
};

// Returns true if there is a web view for |web_state| that contains |text|.
// Otherwise, returns false.
bool IsWebViewContainingText(web::WebState* web_state, const std::string& text);

// Waits for the given web state to contain |text|. If the condition is not met
// within a timeout false is returned.
bool WaitForWebViewContainingText(web::WebState* web_state,
                                  std::string text) WARN_UNUSED_RESULT;

// Waits for a web view with the corresponding |image_id| and |image_state|, in
// the given |web_state|.
bool WaitForWebViewContainingImage(std::string image_id,
                                   web::WebState* web_state,
                                   ImageStateElement image_state);

// Returns true if there is a web view for |web_state| that contains the CSS
// selector |css_selector|.
bool IsWebViewContainingCssSelector(web::WebState* web_state,
                                    const std::string& css_selector);

}  // namespace test
}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_WEB_VIEW_CONTENT_TEST_UTIL_H_
