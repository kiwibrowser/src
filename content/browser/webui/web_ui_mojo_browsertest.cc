// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "content/browser/webui/web_ui_controller_factory_registry.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/data/web_ui_test_mojo_bindings.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
namespace {

bool g_got_message = false;

base::FilePath GetFilePathForJSResource(const std::string& path) {
  base::ThreadRestrictions::ScopedAllowIO allow_io_from_test_callbacks;

  std::string binding_path = "gen/" + path;
#if defined(OS_WIN)
  base::ReplaceChars(binding_path, "//", "\\", &binding_path);
#endif
  base::FilePath exe_dir;
  base::PathService::Get(base::DIR_EXE, &exe_dir);
  return exe_dir.AppendASCII(binding_path);
}

// The bindings for the page are generated from a .mojom file. This code looks
// up the generated file from disk and returns it.
bool GetResource(const std::string& id,
                 const WebUIDataSource::GotDataCallback& callback) {
  base::ThreadRestrictions::ScopedAllowIO allow_io_from_test_callbacks;

  std::string contents;
  if (base::EndsWith(id, ".mojom.js", base::CompareCase::SENSITIVE)) {
    CHECK(base::ReadFileToString(GetFilePathForJSResource(id), &contents))
        << id;
  } else {
    base::FilePath path;
    CHECK(base::PathService::Get(content::DIR_TEST_DATA, &path));
    path = path.AppendASCII(id.substr(0, id.find("?")));
    CHECK(base::ReadFileToString(path, &contents)) << path.value();
  }

  base::RefCountedString* ref_contents = new base::RefCountedString;
  ref_contents->data() = contents;
  callback.Run(ref_contents);
  return true;
}

class BrowserTargetImpl : public mojom::BrowserTarget {
 public:
  BrowserTargetImpl(base::RunLoop* run_loop,
                    mojo::InterfaceRequest<mojom::BrowserTarget> request)
      : run_loop_(run_loop), binding_(this, std::move(request)) {}

  ~BrowserTargetImpl() override {}

  // mojom::BrowserTarget overrides:
  void Start(StartCallback closure) override { std::move(closure).Run(); }
  void Stop() override {
    g_got_message = true;
    run_loop_->Quit();
  }

 protected:
  base::RunLoop* const run_loop_;

 private:
  mojo::Binding<mojom::BrowserTarget> binding_;
  DISALLOW_COPY_AND_ASSIGN(BrowserTargetImpl);
};

// WebUIController that sets up mojo bindings.
class TestWebUIController : public WebUIController {
 public:
  TestWebUIController(WebUI* web_ui, base::RunLoop* run_loop)
      : WebUIController(web_ui), run_loop_(run_loop) {
    WebUIDataSource* data_source = WebUIDataSource::Create("mojo-web-ui");
    data_source->SetRequestFilter(base::Bind(&GetResource));
    WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                         data_source);
  }

 protected:
  base::RunLoop* const run_loop_;
  std::unique_ptr<BrowserTargetImpl> browser_target_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestWebUIController);
};

// TestWebUIController that additionally creates the ping test BrowserTarget
// implementation at the right time.
class PingTestWebUIController : public TestWebUIController,
                                public WebContentsObserver {
 public:
  PingTestWebUIController(WebUI* web_ui, base::RunLoop* run_loop)
      : TestWebUIController(web_ui, run_loop),
        WebContentsObserver(web_ui->GetWebContents()) {
    registry_.AddInterface(base::Bind(&PingTestWebUIController::CreateHandler,
                                      base::Unretained(this)));
  }
  ~PingTestWebUIController() override {}

  // WebContentsObserver implementation:
  void OnInterfaceRequestFromFrame(
      RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override {
    registry_.TryBindInterface(interface_name, interface_pipe);
  }

  void CreateHandler(mojom::BrowserTargetRequest request) {
    browser_target_ =
        std::make_unique<BrowserTargetImpl>(run_loop_, std::move(request));
  }

 private:
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(PingTestWebUIController);
};

// WebUIControllerFactory that creates TestWebUIController.
class TestWebUIControllerFactory : public WebUIControllerFactory {
 public:
  TestWebUIControllerFactory() : run_loop_(nullptr) {}

  void set_run_loop(base::RunLoop* run_loop) { run_loop_ = run_loop; }

  WebUIController* CreateWebUIControllerForURL(WebUI* web_ui,
                                               const GURL& url) const override {
    if (url.query() == "ping")
      return new PingTestWebUIController(web_ui, run_loop_);
    return new TestWebUIController(web_ui, run_loop_);
  }
  WebUI::TypeID GetWebUIType(BrowserContext* browser_context,
                             const GURL& url) const override {
    if (!web_ui_enabled_)
      return WebUI::kNoWebUI;
    return reinterpret_cast<WebUI::TypeID>(1);
  }
  bool UseWebUIForURL(BrowserContext* browser_context,
                      const GURL& url) const override {
    return true;
  }
  bool UseWebUIBindingsForURL(BrowserContext* browser_context,
                              const GURL& url) const override {
    return true;
  }

  void set_web_ui_enabled(bool enabled) { web_ui_enabled_ = enabled; }

 private:
  base::RunLoop* run_loop_;
  bool web_ui_enabled_ = true;

  DISALLOW_COPY_AND_ASSIGN(TestWebUIControllerFactory);
};

class WebUIMojoTest : public ContentBrowserTest {
 public:
  WebUIMojoTest() {
    WebUIControllerFactory::RegisterFactory(&factory_);
  }

  ~WebUIMojoTest() override {
    WebUIControllerFactory::UnregisterFactoryForTesting(&factory_);
  }

  TestWebUIControllerFactory* factory() { return &factory_; }

 private:
  TestWebUIControllerFactory factory_;

  DISALLOW_COPY_AND_ASSIGN(WebUIMojoTest);
};

bool IsGeneratedResourceAvailable(const std::string& resource_path) {
  // Currently there is no way to have a generated file included in the isolate
  // files. If the bindings file doesn't exist assume we're on such a bot and
  // pass.
  // TODO(sky): remove this conditional when isolates support copying from gen.
  base::ThreadRestrictions::ScopedAllowIO allow_io_for_file_existence_check;
  const base::FilePath test_file_path(GetFilePathForJSResource(resource_path));
  if (base::PathExists(test_file_path))
    return true;
  LOG(WARNING) << " mojom binding file doesn't exist, assuming on isolate";
  return false;
}

// Loads a webui page that contains mojo bindings and verifies a message makes
// it from the browser to the page and back.
IN_PROC_BROWSER_TEST_F(WebUIMojoTest, EndToEndPing) {
  if (!IsGeneratedResourceAvailable(
          "content/test/data/web_ui_test_mojo_bindings.mojom.js"))
    return;

  g_got_message = false;
  ASSERT_TRUE(embedded_test_server()->Start());
  base::RunLoop run_loop;
  factory()->set_run_loop(&run_loop);
  GURL test_url("chrome://mojo-web-ui/web_ui_mojo.html?ping");
  NavigateToURL(shell(), test_url);
  // RunLoop is quit when message received from page.
  run_loop.Run();
  EXPECT_TRUE(g_got_message);

  // Check that a second render frame in the same renderer process works
  // correctly.
  Shell* other_shell = CreateBrowser();
  g_got_message = false;
  base::RunLoop other_run_loop;
  factory()->set_run_loop(&other_run_loop);
  NavigateToURL(other_shell, test_url);
  // RunLoop is quit when message received from page.
  other_run_loop.Run();
  EXPECT_TRUE(g_got_message);
  EXPECT_EQ(shell()->web_contents()->GetMainFrame()->GetProcess(),
            other_shell->web_contents()->GetMainFrame()->GetProcess());
}

IN_PROC_BROWSER_TEST_F(WebUIMojoTest, NativeMojoAvailable) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL kTestWebUIUrl("chrome://mojo-web-ui/web_ui_mojo_native.html");
  NavigateToURL(shell(), kTestWebUIUrl);

  bool is_native_mojo_available = false;
  const std::string kTestScript("isNativeMojoAvailable()");
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "domAutomationController.send(" + kTestScript + ")",
      &is_native_mojo_available));
  EXPECT_TRUE(is_native_mojo_available);

  // Now navigate again with WebUI disabled and ensure the native bindings are
  // not available.
  factory()->set_web_ui_enabled(false);
  const std::string kTestNonWebUIUrl("/web_ui_mojo_native.html");
  NavigateToURL(shell(), embedded_test_server()->GetURL(kTestNonWebUIUrl));
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "domAutomationController.send(" + kTestScript + ")",
      &is_native_mojo_available));
  EXPECT_FALSE(is_native_mojo_available);
}

}  // namespace
}  // namespace content
