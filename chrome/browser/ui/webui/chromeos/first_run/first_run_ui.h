// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_FIRST_RUN_FIRST_RUN_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_FIRST_RUN_FIRST_RUN_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}

namespace chromeos {

class FirstRunActor;

// WebUI controller for first-run tutorial.
class FirstRunUI : public content::WebUIController {
 public:
  explicit FirstRunUI(content::WebUI* web_ui);
  FirstRunActor* get_actor() { return actor_; }
 private:
  FirstRunActor* actor_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunUI);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_FIRST_RUN_FIRST_RUN_UI_H_

