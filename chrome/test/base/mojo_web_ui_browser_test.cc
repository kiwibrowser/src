// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/mojo_web_ui_browser_test.h"

#include "base/macros.h"
#include "base/path_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/webui/web_ui_test_handler.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/data/grit/webui_test_resources.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

MojoWebUIBrowserTest::MojoWebUIBrowserTest() {
  registry_.AddInterface<web_ui_test::mojom::TestRunner>(base::BindRepeating(
      &MojoWebUIBrowserTest::BindTestRunner, base::Unretained(this)));
}

MojoWebUIBrowserTest::~MojoWebUIBrowserTest() = default;

void MojoWebUIBrowserTest::SetUpOnMainThread() {
  WebUIBrowserTest::SetUpOnMainThread();

  base::FilePath pak_path;
  ASSERT_TRUE(base::PathService::Get(base::DIR_MODULE, &pak_path));
  pak_path = pak_path.AppendASCII("browser_tests.pak");
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      pak_path, ui::SCALE_FACTOR_NONE);
}

void MojoWebUIBrowserTest::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  // Right now, this is expected to be called only for main frames.
  if (render_frame_host->GetParent()) {
    FAIL() << "Terminating renderer for requesting " << interface_name
           << " interface from subframe";
    render_frame_host->GetProcess()->ShutdownForBadMessage(
        content::RenderProcessHost::CrashReportMode::GENERATE_CRASH_DUMP);
    return;
  }
  registry_.TryBindInterface(interface_name, interface_pipe);
}

void MojoWebUIBrowserTest::BindTestRunner(
    web_ui_test::mojom::TestRunnerRequest request) {
  test_handler()->BindToTestRunnerRequest(std::move(request));
}

void MojoWebUIBrowserTest::SetupHandlers() {
  content::WebUI* web_ui_instance =
      override_selected_web_ui()
          ? override_selected_web_ui()
          : browser()->tab_strip_model()->GetActiveWebContents()->GetWebUI();
  ASSERT_TRUE(web_ui_instance != nullptr);

  test_handler()->set_web_ui(web_ui_instance);

  Observe(web_ui_instance->GetWebContents());
}

void MojoWebUIBrowserTest::BrowsePreload(const GURL& browse_to) {
  WebUIBrowserTest::BrowsePreload(browse_to);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  web_contents->GetMainFrame()->ExecuteJavaScriptForTests(
      l10n_util::GetStringUTF16(IDR_WEB_UI_TEST_MOJO_JS));
}
