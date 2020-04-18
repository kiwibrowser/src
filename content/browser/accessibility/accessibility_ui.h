// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ACCESSIBILITY_ACCESSIBILITY_UI_H_
#define CONTENT_BROWSER_ACCESSIBILITY_ACCESSIBILITY_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
  class ListValue;
}  // namespace base

namespace content {

class AccessibilityUI : public WebUIController {
 public:
  explicit AccessibilityUI(WebUI* web_ui);
  ~AccessibilityUI() override;
};

class AccessibilityUIMessageHandler : public content::WebUIMessageHandler {
 public:
  AccessibilityUIMessageHandler();
  ~AccessibilityUIMessageHandler() override;

  void RegisterMessages() override;

 private:
  void ToggleAccessibility(const base::ListValue* args);
  void SetGlobalFlag(const base::ListValue* args);
  void RequestWebContentsTree(const base::ListValue* args);
  void RequestNativeUITree(const base::ListValue* args);

  DISALLOW_COPY_AND_ASSIGN(AccessibilityUIMessageHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ACCESSIBILITY_ACCESSIBILITY_UI_H_
