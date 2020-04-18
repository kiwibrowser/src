// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_MOJO_WEB_UI_BROWSER_TEST_H_
#define CHROME_TEST_BASE_MOJO_WEB_UI_BROWSER_TEST_H_

#include <string>

#include "chrome/test/base/web_ui_browser_test.h"
#include "chrome/test/data/webui/web_ui_test.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/service_manager/public/cpp/binder_registry.h"

// The runner of Mojo WebUI javascript based tests. The main difference between
// this and WebUIBrowserTest is that tests subclassing from this class use a
// mojo pipe to send the test result, so there is no reliance on chrome.send().
class MojoWebUIBrowserTest : public WebUIBrowserTest,
                             public content::WebContentsObserver {
 public:
  MojoWebUIBrowserTest();
  ~MojoWebUIBrowserTest() override;

  // WebUIBrowserTest:
  void BrowsePreload(const GURL& browse_to) override;
  void SetUpOnMainThread() override;
  void SetupHandlers() override;

  // content::WebContentsObserver:
  void OnInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;

 private:
  void BindTestRunner(web_ui_test::mojom::TestRunnerRequest request);

  service_manager::BinderRegistry registry_;
};

#endif  // CHROME_TEST_BASE_MOJO_WEB_UI_BROWSER_TEST_H_
