// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_WEB_UI_TEST_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_WEB_UI_TEST_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/test/data/webui/web_ui_test.mojom.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace base {
class ListValue;
class Value;
}

namespace content {
class RenderViewHost;
}

// This class registers test framework specific handlers on WebUI objects.
class WebUITestHandler : public content::WebUIMessageHandler,
                         public web_ui_test::mojom::TestRunner {
 public:
  WebUITestHandler();
  ~WebUITestHandler() override;

  // Sends a message through |preload_host| with the |js_text| to preload at the
  // appropriate time before the onload call is made.
  void PreloadJavaScript(const base::string16& js_text,
                         content::RenderViewHost* preload_host);

  // Runs |js_text| in this object's WebUI frame. Does not wait for any result.
  void RunJavaScript(const base::string16& js_text);

  // Runs |js_text| in this object's WebUI frame. Waits for result, logging an
  // error message on failure. Returns test pass/fail.
  bool RunJavaScriptTestWithResult(const base::string16& js_text);

  // Binds the Mojo test interface to this handler.
  void BindToTestRunnerRequest(web_ui_test::mojom::TestRunnerRequest request);

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

  // web_ui_test::mojom::TestRunner:
  void TestComplete(const base::Optional<std::string>& message) override;

 private:
  // Receives testResult messages.
  void HandleTestResult(const base::ListValue* test_result);

  // Gets the callback that Javascript execution is complete.
  void JavaScriptComplete(const base::Value* result);

  // Runs a message loop until test finishes. Returns the result of the
  // test.
  bool WaitForResult();

  // Received test pass/fail;
  bool test_done_;

  // Pass fail result of current test.
  bool test_succeeded_;

  // Test code finished trying to execute. This will be set to true when the
  // selected tab is done with this execution request whether it was able to
  // parse/execute the javascript or not.
  bool run_test_done_;

  // Test code was able to execute successfully. This is *NOT* the test
  // pass/fail.
  bool run_test_succeeded_;

  // Quits the currently running RunLoop.
  base::Closure quit_closure_;

  mojo::Binding<web_ui_test::mojom::TestRunner> binding_;

  DISALLOW_COPY_AND_ASSIGN(WebUITestHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_WEB_UI_TEST_HANDLER_H_
