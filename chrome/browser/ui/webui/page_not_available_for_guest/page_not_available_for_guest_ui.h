// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_PAGE_NOT_AVAILABLE_FOR_GUEST_PAGE_NOT_AVAILABLE_FOR_GUEST_UI_H_
#define CHROME_BROWSER_UI_WEBUI_PAGE_NOT_AVAILABLE_FOR_GUEST_PAGE_NOT_AVAILABLE_FOR_GUEST_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

class PageNotAvailableForGuestUI : public content::WebUIController {
 public:
  explicit PageNotAvailableForGuestUI(content::WebUI* web_ui,
                                      const std::string& host_name);

 private:
  DISALLOW_COPY_AND_ASSIGN(PageNotAvailableForGuestUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_PAGE_NOT_AVAILABLE_FOR_GUEST_PAGE_NOT_AVAILABLE_FOR_GUEST_UI_H_
