// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_UI_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_ui_controller.h"

// A custom WebUI that defines datasource for mobile setup registration page
// that is used in Chrome OS activate modem and perform plan subscription tasks.
class MobileSetupUI : public content::WebUIController,
                      public content::WebContentsObserver,
                      public base::SupportsWeakPtr<MobileSetupUI> {
 public:
  explicit MobileSetupUI(content::WebUI* web_ui);

 private:
  // content::WebContentsObserver overrides.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  DISALLOW_COPY_AND_ASSIGN(MobileSetupUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_UI_H_
