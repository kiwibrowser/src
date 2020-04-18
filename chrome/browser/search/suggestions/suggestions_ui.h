// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_SUGGESTIONS_SUGGESTIONS_UI_H_
#define CHROME_BROWSER_SEARCH_SUGGESTIONS_SUGGESTIONS_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}

namespace suggestions {

// The WebUIController for chrome://suggestions. Renders a webpage to list
// SuggestionsService data.
class SuggestionsUI : public content::WebUIController {
 public:
  explicit SuggestionsUI(content::WebUI* web_ui);
  ~SuggestionsUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SuggestionsUI);
};

}  // namespace suggestions

#endif  // CHROME_BROWSER_SEARCH_SUGGESTIONS_SUGGESTIONS_UI_H_
