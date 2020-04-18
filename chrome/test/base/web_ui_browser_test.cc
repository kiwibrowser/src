// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/web_ui_browser_test.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/web_ui_test_handler.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/test_chrome_web_ui_controller_factory.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/filename_util.h"
#include "printing/buildflags/buildflags.h"
#include "ui/base/resource/resource_handle.h"

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#endif

using content::RenderViewHost;
using content::WebContents;
using content::WebUIController;
using content::WebUIMessageHandler;

namespace {

base::LazyInstance<std::vector<std::string>>::DestructorAtExit error_messages_ =
    LAZY_INSTANCE_INITIALIZER;

// Intercepts all log messages.
bool LogHandler(int severity,
                const char* file,
                int line,
                size_t message_start,
                const std::string& str) {
  if (severity == logging::LOG_ERROR && file &&
      std::string("CONSOLE") == file) {
    error_messages_.Get().push_back(str);
  }

  return false;
}

class WebUIJsInjectionReadyObserver : public content::WebContentsObserver {
 public:
  WebUIJsInjectionReadyObserver(content::WebContents* web_contents,
                                WebUIBrowserTest* browser_test,
                                const std::string& preload_test_fixture,
                                const std::string& preload_test_name)
      : content::WebContentsObserver(web_contents),
        browser_test_(browser_test),
        preload_test_fixture_(preload_test_fixture),
        preload_test_name_(preload_test_name) {}

  void RenderViewCreated(content::RenderViewHost* rvh) override {
    browser_test_->PreLoadJavascriptLibraries(
        preload_test_fixture_, preload_test_name_, rvh);
  }

 private:
  WebUIBrowserTest* browser_test_;
  std::string preload_test_fixture_;
  std::string preload_test_name_;
};

}  // namespace

WebUIBrowserTest::~WebUIBrowserTest() {
}

bool WebUIBrowserTest::RunJavascriptFunction(const std::string& function_name) {
  std::vector<base::Value> empty_args;
  return RunJavascriptFunction(function_name, std::move(empty_args));
}

bool WebUIBrowserTest::RunJavascriptFunction(const std::string& function_name,
                                             base::Value arg) {
  std::vector<base::Value> args;
  args.push_back(std::move(arg));
  return RunJavascriptFunction(function_name, std::move(args));
}

bool WebUIBrowserTest::RunJavascriptFunction(const std::string& function_name,
                                             base::Value arg1,
                                             base::Value arg2) {
  std::vector<base::Value> args;
  args.push_back(std::move(arg1));
  args.push_back(std::move(arg2));
  return RunJavascriptFunction(function_name, std::move(args));
}

bool WebUIBrowserTest::RunJavascriptFunction(
    const std::string& function_name,
    std::vector<base::Value> function_arguments) {
  return RunJavascriptUsingHandler(function_name, std::move(function_arguments),
                                   false, false, nullptr);
}

bool WebUIBrowserTest::RunJavascriptTestF(bool is_async,
                                          const std::string& test_fixture,
                                          const std::string& test_name) {
  std::vector<base::Value> args;
  args.push_back(base::Value(test_fixture));
  args.push_back(base::Value(test_name));

  if (is_async)
    return RunJavascriptAsyncTest("RUN_TEST_F", std::move(args));
  else
    return RunJavascriptTest("RUN_TEST_F", std::move(args));
}

bool WebUIBrowserTest::RunJavascriptTest(const std::string& test_name) {
  std::vector<base::Value> empty_args;
  return RunJavascriptTest(test_name, std::move(empty_args));
}

bool WebUIBrowserTest::RunJavascriptTest(const std::string& test_name,
                                         base::Value arg) {
  std::vector<base::Value> args;
  args.push_back(std::move(arg));
  return RunJavascriptTest(test_name, std::move(args));
}

bool WebUIBrowserTest::RunJavascriptTest(const std::string& test_name,
                                         base::Value arg1,
                                         base::Value arg2) {
  std::vector<base::Value> args;
  args.push_back(std::move(arg1));
  args.push_back(std::move(arg2));
  return RunJavascriptTest(test_name, std::move(args));
}

bool WebUIBrowserTest::RunJavascriptTest(
    const std::string& test_name,
    std::vector<base::Value> test_arguments) {
  return RunJavascriptUsingHandler(test_name, std::move(test_arguments), true,
                                   false, nullptr);
}

bool WebUIBrowserTest::RunJavascriptAsyncTest(const std::string& test_name) {
  std::vector<base::Value> empty_args;
  return RunJavascriptAsyncTest(test_name, std::move(empty_args));
}

bool WebUIBrowserTest::RunJavascriptAsyncTest(const std::string& test_name,
                                              base::Value arg) {
  std::vector<base::Value> args;
  args.push_back(std::move(arg));
  return RunJavascriptAsyncTest(test_name, std::move(args));
}

bool WebUIBrowserTest::RunJavascriptAsyncTest(const std::string& test_name,
                                              base::Value arg1,
                                              base::Value arg2) {
  std::vector<base::Value> args;
  args.push_back(std::move(arg1));
  args.push_back(std::move(arg2));
  return RunJavascriptAsyncTest(test_name, std::move(args));
}

bool WebUIBrowserTest::RunJavascriptAsyncTest(const std::string& test_name,
                                              base::Value arg1,
                                              base::Value arg2,
                                              base::Value arg3) {
  std::vector<base::Value> args;
  args.push_back(std::move(arg1));
  args.push_back(std::move(arg2));
  args.push_back(std::move(arg3));
  return RunJavascriptAsyncTest(test_name, std::move(args));
}

bool WebUIBrowserTest::RunJavascriptAsyncTest(
    const std::string& test_name,
    std::vector<base::Value> test_arguments) {
  return RunJavascriptUsingHandler(test_name, std::move(test_arguments), true,
                                   true, nullptr);
}

void WebUIBrowserTest::PreLoadJavascriptLibraries(
    const std::string& preload_test_fixture,
    const std::string& preload_test_name,
    RenderViewHost* preload_host) {
  ASSERT_FALSE(libraries_preloaded_);
  std::vector<base::Value> args;
  args.push_back(base::Value(preload_test_fixture));
  args.push_back(base::Value(preload_test_name));
  RunJavascriptUsingHandler("preloadJavascriptLibraries", std::move(args),
                            false, false, preload_host);
  libraries_preloaded_ = true;
}

void WebUIBrowserTest::BrowsePreload(const GURL& browse_to) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WebUIJsInjectionReadyObserver injection_observer(
      web_contents, this, preload_test_fixture_, preload_test_name_);
  content::TestNavigationObserver navigation_observer(web_contents);
  NavigateParams params(browser(), GURL(browse_to), ui::PAGE_TRANSITION_TYPED);
  params.disposition = WindowOpenDisposition::CURRENT_TAB;

  Navigate(&params);
  navigation_observer.Wait();
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)

// This custom ContentBrowserClient is used to get notified when a WebContents
// for the print preview dialog gets created.
class PrintContentBrowserClient : public ChromeContentBrowserClient {
 public:
  PrintContentBrowserClient(WebUIBrowserTest* browser_test,
                            const std::string& preload_test_fixture,
                            const std::string& preload_test_name)
      : browser_test_(browser_test),
        preload_test_fixture_(preload_test_fixture),
        preload_test_name_(preload_test_name),
        preview_dialog_(nullptr),
        message_loop_runner_(new content::MessageLoopRunner) {}

  void Wait() {
    message_loop_runner_->Run();
    content::WaitForLoadStop(preview_dialog_);
  }

 private:
  // ChromeContentBrowserClient implementation:
  content::WebContentsViewDelegate* GetWebContentsViewDelegate(
      content::WebContents* web_contents) override {
    preview_dialog_ = web_contents;
    observer_.reset(new WebUIJsInjectionReadyObserver(preview_dialog_,
                                                      browser_test_,
                                                      preload_test_fixture_,
                                                      preload_test_name_));
    message_loop_runner_->Quit();
    return nullptr;
  }

  WebUIBrowserTest* browser_test_;
  std::unique_ptr<WebUIJsInjectionReadyObserver> observer_;
  std::string preload_test_fixture_;
  std::string preload_test_name_;
  content::WebContents* preview_dialog_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;
};
#endif

void WebUIBrowserTest::BrowsePrintPreload(const GURL& browse_to) {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  ui_test_utils::NavigateToURL(browser(), browse_to);

  PrintContentBrowserClient new_client(
      this, preload_test_fixture_, preload_test_name_);
  content::ContentBrowserClient* old_client =
      SetBrowserClientForTesting(&new_client);

  chrome::Print(browser());
  new_client.Wait();

  SetBrowserClientForTesting(old_client);

  printing::PrintPreviewDialogController* tab_controller =
      printing::PrintPreviewDialogController::GetInstance();
  ASSERT_TRUE(tab_controller);
  WebContents* preview_dialog = tab_controller->GetPrintPreviewForContents(
      browser()->tab_strip_model()->GetActiveWebContents());
  ASSERT_TRUE(preview_dialog);
  SetWebUIInstance(preview_dialog->GetWebUI());
#else
  NOTREACHED();
#endif
}

const char WebUIBrowserTest::kDummyURL[] = "chrome://DummyURL";

WebUIBrowserTest::WebUIBrowserTest()
    : test_handler_(new WebUITestHandler()),
      libraries_preloaded_(false),
      override_selected_web_ui_(nullptr) {}

void WebUIBrowserTest::set_preload_test_fixture(
    const std::string& preload_test_fixture) {
  preload_test_fixture_ = preload_test_fixture;
}

void WebUIBrowserTest::set_preload_test_name(
    const std::string& preload_test_name) {
  preload_test_name_ = preload_test_name;
}

namespace {

// DataSource for the dummy URL.  If no data source is provided then an error
// page is shown. While this doesn't matter for most tests, without it,
// navigation to different anchors cannot be listened to (via the hashchange
// event).
class MockWebUIDataSource : public content::URLDataSource {
 public:
  MockWebUIDataSource() {}

 private:
  ~MockWebUIDataSource() override {}

  std::string GetSource() const override { return "dummyurl"; }

  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override {
    std::string dummy_html = "<html><body>Dummy</body></html>";
    scoped_refptr<base::RefCountedString> response =
        base::RefCountedString::TakeString(&dummy_html);
    callback.Run(response.get());
  }

  std::string GetMimeType(const std::string& path) const override {
    return "text/html";
  }

  DISALLOW_COPY_AND_ASSIGN(MockWebUIDataSource);
};

// WebUIProvider to allow attaching the DataSource for the dummy URL when
// testing.
class MockWebUIProvider
    : public TestChromeWebUIControllerFactory::WebUIProvider {
 public:
  MockWebUIProvider() {}

  // Returns a new WebUI
  WebUIController* NewWebUI(content::WebUI* web_ui, const GURL& url) override {
    WebUIController* controller = new content::WebUIController(web_ui);
    Profile* profile = Profile::FromWebUI(web_ui);
    content::URLDataSource::Add(profile, new MockWebUIDataSource());
    return controller;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWebUIProvider);
};

base::LazyInstance<MockWebUIProvider>::DestructorAtExit mock_provider_ =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

void WebUIBrowserTest::SetUpCommandLine(base::CommandLine* command_line) {
  JavaScriptBrowserTest::SetUpCommandLine(command_line);

  // Enables the MojoJSTest bindings which are used for WebUI tests.
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kEnableBlinkFeatures, "MojoJSTest");
}

void WebUIBrowserTest::SetUpOnMainThread() {
  JavaScriptBrowserTest::SetUpOnMainThread();

  logging::SetLogMessageHandler(&LogHandler);

  AddLibrary(base::FilePath(kA11yAuditLibraryJSPath));

  content::WebUIControllerFactory::UnregisterFactoryForTesting(
      ChromeWebUIControllerFactory::GetInstance());

  test_factory_.reset(new TestChromeWebUIControllerFactory);

  content::WebUIControllerFactory::RegisterFactory(test_factory_.get());

  test_factory_->AddFactoryOverride(GURL(kDummyURL).host(),
                                    mock_provider_.Pointer());
  test_factory_->AddFactoryOverride(content::kChromeUIResourcesHost,
                                    mock_provider_.Pointer());
}

void WebUIBrowserTest::TearDownOnMainThread() {
  logging::SetLogMessageHandler(nullptr);

  test_factory_->RemoveFactoryOverride(GURL(kDummyURL).host());
  content::WebUIControllerFactory::UnregisterFactoryForTesting(
      test_factory_.get());

  // This is needed to avoid a debug assert after the test completes, see stack
  // trace in http://crrev.com/179347
  content::WebUIControllerFactory::RegisterFactory(
      ChromeWebUIControllerFactory::GetInstance());

  test_factory_.reset();
}

void WebUIBrowserTest::SetWebUIInstance(content::WebUI* web_ui) {
  override_selected_web_ui_ = web_ui;
}

WebUIMessageHandler* WebUIBrowserTest::GetMockMessageHandler() {
  return nullptr;
}

bool WebUIBrowserTest::RunJavascriptUsingHandler(
    const std::string& function_name,
    std::vector<base::Value> function_arguments,
    bool is_test,
    bool is_async,
    RenderViewHost* preload_host) {
  // Get the user libraries. Preloading them individually is best, then
  // we can assign each one a filename for better stack traces. Otherwise
  // append them all to |content|.
  base::string16 content;
  std::vector<base::string16> libraries;
  if (!libraries_preloaded_) {
    BuildJavascriptLibraries(&libraries);
    if (!preload_host) {
      content = base::JoinString(libraries, base::ASCIIToUTF16("\n"));
      libraries.clear();
    }
  }

  if (!function_name.empty()) {
    base::string16 called_function;
    if (is_test) {
      called_function = BuildRunTestJSCall(is_async, function_name,
                                           std::move(function_arguments));
    } else {
      std::vector<const base::Value*> ptr_vector(function_arguments.size());
      for (size_t i = 0; i < function_arguments.size(); ++i)
        ptr_vector[i] = &function_arguments[i];
      called_function =
          content::WebUI::GetJavascriptCall(function_name, ptr_vector);
    }
    content.append(called_function);
  }

  if (!preload_host)
    SetupHandlers();

  bool result = true;

  for (size_t i = 0; i < libraries.size(); ++i)
    test_handler_->PreloadJavaScript(libraries[i], preload_host);

  if (is_test)
    result = test_handler_->RunJavaScriptTestWithResult(content);
  else if (preload_host)
    test_handler_->PreloadJavaScript(content, preload_host);
  else
    test_handler_->RunJavaScript(content);

  if (error_messages_.Get().size() > 0) {
    LOG(ERROR) << "CONDITION FAILURE: encountered javascript console error(s):";
    for (const auto& msg : error_messages_.Get()) {
      LOG(ERROR) << "JS ERROR: '" << msg << "'";
    }
    LOG(ERROR) << "JS call assumed failed, because JS console error(s) found.";

    result = false;
    error_messages_.Get().clear();
  }
  return result;
}

void WebUIBrowserTest::SetupHandlers() {
  content::WebUI* web_ui_instance =
      override_selected_web_ui_
          ? override_selected_web_ui_
          : browser()->tab_strip_model()->GetActiveWebContents()->GetWebUI();
  ASSERT_TRUE(web_ui_instance != nullptr);

  test_handler_->set_web_ui(web_ui_instance);
  test_handler_->RegisterMessages();

  if (GetMockMessageHandler()) {
    GetMockMessageHandler()->set_web_ui(web_ui_instance);
    GetMockMessageHandler()->RegisterMessages();
  }
}

GURL WebUIBrowserTest::WebUITestDataPathToURL(
    const base::FilePath::StringType& path) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath dir_test_data;
  EXPECT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &dir_test_data));
  base::FilePath test_path(dir_test_data.Append(kWebUITestFolder).Append(path));
  EXPECT_TRUE(base::PathExists(test_path));
  return net::FilePathToFileURL(test_path);
}
