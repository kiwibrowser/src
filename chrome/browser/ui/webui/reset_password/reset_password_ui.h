// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_RESET_PASSWORD_RESET_PASSWORD_UI_H_
#define CHROME_BROWSER_UI_WEBUI_RESET_PASSWORD_RESET_PASSWORD_UI_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/reset_password/reset_password.mojom.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace base {
class DictionaryValue;
}

namespace content {
class WebContents;
}

// The WebUI for chrome://reset-password/.
class ResetPasswordUI : public ui::MojoWebUIController {
 public:
  explicit ResetPasswordUI(content::WebUI* web_ui);
  ~ResetPasswordUI() override;

 private:
  void BindResetPasswordHandler(mojom::ResetPasswordHandlerRequest request);

  void PopulateStrings(content::WebContents* web_content,
                       base::DictionaryValue* load_time_data);

  std::unique_ptr<mojom::ResetPasswordHandler> ui_handler_;

  DISALLOW_COPY_AND_ASSIGN(ResetPasswordUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_RESET_PASSWORD_RESET_PASSWORD_UI_H_
