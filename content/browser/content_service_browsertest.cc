// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/content_browser_test.h"
#include "content/shell/browser/shell.h"
#include "services/content/public/cpp/view.h"
#include "services/content/public/mojom/constants.mojom.h"
#include "services/content/public/mojom/view_factory.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {
namespace {

class ContentServiceBrowserTest : public ContentBrowserTest {
 public:
  ContentServiceBrowserTest() = default;
  ~ContentServiceBrowserTest() override = default;

  void SetUpOnMainThread() override {
    ContentBrowserTest::SetUpOnMainThread();

    base::FilePath test_data_path;
    CHECK(base::PathService::Get(DIR_TEST_DATA, &test_data_path));
    embedded_test_server()->ServeFilesFromDirectory(test_data_path);
    CHECK(embedded_test_server()->Start());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentServiceBrowserTest);
};

// Verifies that the embedded Content Service is reachable. Does a basic
// end-to-end sanity check to also verify that a ContentView client is backed
// by a WebContents instance in the browser.
IN_PROC_BROWSER_TEST_F(ContentServiceBrowserTest, EmbeddedContentService) {
  auto* browser_context = shell()->web_contents()->GetBrowserContext();
  auto* connector = BrowserContext::GetConnectorFor(browser_context);

  content::mojom::ViewFactoryPtr factory;
  connector->BindInterface(content::mojom::kServiceName, &factory);
  auto view = std::make_unique<content::View>(factory.get());

  base::RunLoop loop;
  view->set_did_stop_loading_callback_for_testing(loop.QuitClosure());
  view->Navigate(embedded_test_server()->GetURL("/hello.html"));
  loop.Run();
}

}  // namespace
}  // namespace content
