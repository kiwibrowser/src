// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_WEBUI_SUGGESTIONS_UI_H_
#define IOS_CHROME_BROWSER_UI_WEBUI_SUGGESTIONS_UI_H_

#include "base/macros.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

namespace web {
class WebUIIOS;
}

namespace suggestions {

// The WebUIController for chrome://suggestions. Renders a webpage to list
// SuggestionsService data.
class SuggestionsUI : public web::WebUIIOSController {
 public:
  explicit SuggestionsUI(web::WebUIIOS* web_ui);
  ~SuggestionsUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SuggestionsUI);
};

}  // namespace suggestions

#endif  // IOS_CHROME_BROWSER_UI_WEBUI_SUGGESTIONS_UI_H_
