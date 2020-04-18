// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "headless/lib/browser/headless_web_contents_impl.h"
#include "headless/public/devtools/domains/browser.h"
#include "headless/public/devtools/domains/dom.h"
#include "headless/public/devtools/domains/dom_snapshot.h"
#include "headless/public/devtools/domains/emulation.h"
#include "headless/public/devtools/domains/inspector.h"
#include "headless/public/devtools/domains/network.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/devtools/domains/target.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_devtools_target.h"
#include "headless/test/headless_browser_test.h"
#include "headless/test/test_protocol_handler.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#define EXPECT_SIZE_EQ(expected, actual)               \
  do {                                                 \
    EXPECT_EQ((expected).width(), (actual).width());   \
    EXPECT_EQ((expected).height(), (actual).height()); \
  } while (false)

using testing::ElementsAre;
using testing::NotNull;
using testing::UnorderedElementsAre;

namespace headless {

namespace {

std::vector<HeadlessWebContents*> GetAllWebContents(HeadlessBrowser* browser) {
  std::vector<HeadlessWebContents*> result;

  for (HeadlessBrowserContext* browser_context :
       browser->GetAllBrowserContexts()) {
    std::vector<HeadlessWebContents*> web_contents =
        browser_context->GetAllWebContents();
    result.insert(result.end(), web_contents.begin(), web_contents.end());
  }

  return result;
}

}  // namespace

class HeadlessDevToolsClientNavigationTest
    : public HeadlessAsyncDevTooledBrowserTest,
      page::ExperimentalObserver {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    std::unique_ptr<page::NavigateParams> params =
        page::NavigateParams::Builder()
            .SetUrl(embedded_test_server()->GetURL("/hello.html").spec())
            .Build();
    devtools_client_->GetPage()->GetExperimental()->AddObserver(this);
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->Enable(run_loop.QuitClosure());
    run_loop.Run();
    devtools_client_->GetPage()->Navigate(std::move(params));
  }

  void OnLoadEventFired(const page::LoadEventFiredParams& params) override {
    devtools_client_->GetPage()->Disable();
    devtools_client_->GetPage()->GetExperimental()->RemoveObserver(this);
    FinishAsynchronousTest();
  }

  // Check that events with no parameters still get a parameters object.
  void OnFrameResized(const page::FrameResizedParams& params) override {}
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientNavigationTest);

class HeadlessDevToolsClientWindowManagementTest
    : public HeadlessAsyncDevTooledBrowserTest {
 public:
  void SetWindowBounds(
      const gfx::Rect& rect,
      base::Callback<void(std::unique_ptr<browser::SetWindowBoundsResult>)>
          callback) {
    std::unique_ptr<browser::Bounds> bounds =
        browser::Bounds::Builder()
            .SetLeft(rect.x())
            .SetTop(rect.y())
            .SetWidth(rect.width())
            .SetHeight(rect.height())
            .SetWindowState(browser::WindowState::NORMAL)
            .Build();
    int window_id = HeadlessWebContentsImpl::From(web_contents_)->window_id();
    std::unique_ptr<browser::SetWindowBoundsParams> params =
        browser::SetWindowBoundsParams::Builder()
            .SetWindowId(window_id)
            .SetBounds(std::move(bounds))
            .Build();
    browser_devtools_client_->GetBrowser()->GetExperimental()->SetWindowBounds(
        std::move(params), callback);
  }

  void SetWindowState(
      const browser::WindowState state,
      base::Callback<void(std::unique_ptr<browser::SetWindowBoundsResult>)>
          callback) {
    std::unique_ptr<browser::Bounds> bounds =
        browser::Bounds::Builder().SetWindowState(state).Build();
    int window_id = HeadlessWebContentsImpl::From(web_contents_)->window_id();
    std::unique_ptr<browser::SetWindowBoundsParams> params =
        browser::SetWindowBoundsParams::Builder()
            .SetWindowId(window_id)
            .SetBounds(std::move(bounds))
            .Build();
    browser_devtools_client_->GetBrowser()->GetExperimental()->SetWindowBounds(
        std::move(params), callback);
  }

  void GetWindowBounds(
      base::Callback<void(std::unique_ptr<browser::GetWindowBoundsResult>)>
          callback) {
    int window_id = HeadlessWebContentsImpl::From(web_contents_)->window_id();
    std::unique_ptr<browser::GetWindowBoundsParams> params =
        browser::GetWindowBoundsParams::Builder()
            .SetWindowId(window_id)
            .Build();

    browser_devtools_client_->GetBrowser()->GetExperimental()->GetWindowBounds(
        std::move(params), callback);
  }

  void CheckWindowBounds(
      const gfx::Rect& bounds,
      const browser::WindowState state,
      std::unique_ptr<browser::GetWindowBoundsResult> result) {
    const browser::Bounds* actual_bounds = result->GetBounds();
// Mac does not support repositioning, as we don't show any actual window.
#if !defined(OS_MACOSX)
    EXPECT_EQ(bounds.x(), actual_bounds->GetLeft());
    EXPECT_EQ(bounds.y(), actual_bounds->GetTop());
#endif  // !defined(OS_MACOSX)
    EXPECT_EQ(bounds.width(), actual_bounds->GetWidth());
    EXPECT_EQ(bounds.height(), actual_bounds->GetHeight());
    EXPECT_EQ(state, actual_bounds->GetWindowState());
  }
};

class HeadlessDevToolsClientChangeWindowBoundsTest
    : public HeadlessDevToolsClientWindowManagementTest {
  void RunDevTooledTest() override {
    SetWindowBounds(
        gfx::Rect(100, 200, 300, 400),
        base::Bind(
            &HeadlessDevToolsClientChangeWindowBoundsTest::OnSetWindowBounds,
            base::Unretained(this)));
  }

  void OnSetWindowBounds(
      std::unique_ptr<browser::SetWindowBoundsResult> result) {
    GetWindowBounds(base::Bind(
        &HeadlessDevToolsClientChangeWindowBoundsTest::OnGetWindowBounds,
        base::Unretained(this)));
  }

  void OnGetWindowBounds(
      std::unique_ptr<browser::GetWindowBoundsResult> result) {
    CheckWindowBounds(gfx::Rect(100, 200, 300, 400),
                      browser::WindowState::NORMAL, std::move(result));
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientChangeWindowBoundsTest);

class HeadlessDevToolsClientChangeWindowStateTest
    : public HeadlessDevToolsClientWindowManagementTest {
 public:
  explicit HeadlessDevToolsClientChangeWindowStateTest(
      browser::WindowState state)
      : state_(state){};

  void RunDevTooledTest() override {
    SetWindowState(
        state_,
        base::Bind(
            &HeadlessDevToolsClientChangeWindowStateTest::OnSetWindowState,
            base::Unretained(this)));
  }

  void OnSetWindowState(
      std::unique_ptr<browser::SetWindowBoundsResult> result) {
    GetWindowBounds(base::Bind(
        &HeadlessDevToolsClientChangeWindowStateTest::OnGetWindowState,
        base::Unretained(this)));
  }

  void OnGetWindowState(
      std::unique_ptr<browser::GetWindowBoundsResult> result) {
    HeadlessBrowser::Options::Builder builder;
    const HeadlessBrowser::Options kDefaultOptions = builder.Build();
    CheckWindowBounds(gfx::Rect(kDefaultOptions.window_size), state_,
                      std::move(result));
    FinishAsynchronousTest();
  }

 protected:
  browser::WindowState state_;
};

class HeadlessDevToolsClientMinimizeWindowTest
    : public HeadlessDevToolsClientChangeWindowStateTest {
 public:
  HeadlessDevToolsClientMinimizeWindowTest()
      : HeadlessDevToolsClientChangeWindowStateTest(
            browser::WindowState::MINIMIZED){};
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientMinimizeWindowTest);

class HeadlessDevToolsClientMaximizeWindowTest
    : public HeadlessDevToolsClientChangeWindowStateTest {
 public:
  HeadlessDevToolsClientMaximizeWindowTest()
      : HeadlessDevToolsClientChangeWindowStateTest(
            browser::WindowState::MAXIMIZED){};
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientMaximizeWindowTest);

class HeadlessDevToolsClientFullscreenWindowTest
    : public HeadlessDevToolsClientChangeWindowStateTest {
 public:
  HeadlessDevToolsClientFullscreenWindowTest()
      : HeadlessDevToolsClientChangeWindowStateTest(
            browser::WindowState::FULLSCREEN){};
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientFullscreenWindowTest);

class HeadlessDevToolsClientEvalTest
    : public HeadlessAsyncDevTooledBrowserTest {
 public:
  void RunDevTooledTest() override {
    std::unique_ptr<runtime::EvaluateParams> params =
        runtime::EvaluateParams::Builder().SetExpression("1 + 2").Build();
    devtools_client_->GetRuntime()->Evaluate(
        std::move(params),
        base::BindOnce(&HeadlessDevToolsClientEvalTest::OnFirstResult,
                       base::Unretained(this)));
    // Test the convenience overload which only takes the required command
    // parameters.
    devtools_client_->GetRuntime()->Evaluate(
        "24 * 7",
        base::BindOnce(&HeadlessDevToolsClientEvalTest::OnSecondResult,
                       base::Unretained(this)));
  }

  void OnFirstResult(std::unique_ptr<runtime::EvaluateResult> result) {
    EXPECT_TRUE(result->GetResult()->HasValue());
    EXPECT_EQ(3, result->GetResult()->GetValue()->GetInt());
  }

  void OnSecondResult(std::unique_ptr<runtime::EvaluateResult> result) {
    EXPECT_TRUE(result->GetResult()->HasValue());
    EXPECT_EQ(168, result->GetResult()->GetValue()->GetInt());
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientEvalTest);

class HeadlessDevToolsClientCallbackTest
    : public HeadlessAsyncDevTooledBrowserTest {
 public:
  HeadlessDevToolsClientCallbackTest() : first_result_received_(false) {}

  void RunDevTooledTest() override {
    // Null callback without parameters.
    devtools_client_->GetPage()->Enable();
    // Null callback with parameters.
    devtools_client_->GetRuntime()->Evaluate("true");
    // Non-null callback without parameters.
    devtools_client_->GetPage()->Disable(
        base::BindOnce(&HeadlessDevToolsClientCallbackTest::OnFirstResult,
                       base::Unretained(this)));
    // Non-null callback with parameters.
    devtools_client_->GetRuntime()->Evaluate(
        "true",
        base::BindOnce(&HeadlessDevToolsClientCallbackTest::OnSecondResult,
                       base::Unretained(this)));
  }

  void OnFirstResult() {
    EXPECT_FALSE(first_result_received_);
    first_result_received_ = true;
  }

  void OnSecondResult(std::unique_ptr<runtime::EvaluateResult> result) {
    EXPECT_TRUE(first_result_received_);
    FinishAsynchronousTest();
  }

 private:
  bool first_result_received_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientCallbackTest);

class HeadlessDevToolsClientObserverTest
    : public HeadlessAsyncDevTooledBrowserTest,
      network::Observer {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetNetwork()->AddObserver(this);
    devtools_client_->GetNetwork()->Enable(run_loop.QuitClosure());
    run_loop.Run();

    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/hello.html").spec());
  }

  void OnRequestWillBeSent(
      const network::RequestWillBeSentParams& params) override {
    EXPECT_EQ("GET", params.GetRequest()->GetMethod());
    EXPECT_EQ(embedded_test_server()->GetURL("/hello.html").spec(),
              params.GetRequest()->GetUrl());
  }

  void OnResponseReceived(
      const network::ResponseReceivedParams& params) override {
    EXPECT_EQ(200, params.GetResponse()->GetStatus());
    EXPECT_EQ("OK", params.GetResponse()->GetStatusText());
    const base::Value* content_type_value =
        params.GetResponse()->GetHeaders()->FindKey("Content-Type");
    ASSERT_THAT(content_type_value, NotNull());
    EXPECT_EQ("text/html", content_type_value->GetString());

    devtools_client_->GetNetwork()->Disable();
    devtools_client_->GetNetwork()->RemoveObserver(this);
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientObserverTest);

class HeadlessDevToolsClientExperimentalTest
    : public HeadlessAsyncDevTooledBrowserTest,
      page::ExperimentalObserver {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->GetExperimental()->AddObserver(this);
    devtools_client_->GetPage()->Enable(run_loop.QuitClosure());
    run_loop.Run();
    // Check that experimental commands require parameter objects.
    devtools_client_->GetRuntime()
        ->GetExperimental()
        ->SetCustomObjectFormatterEnabled(
            runtime::SetCustomObjectFormatterEnabledParams::Builder()
                .SetEnabled(false)
                .Build());

    // Check that a previously experimental command which takes no parameters
    // still works by giving it a parameter object.
    devtools_client_->GetRuntime()->GetExperimental()->RunIfWaitingForDebugger(
        runtime::RunIfWaitingForDebuggerParams::Builder().Build());

    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/hello.html").spec());
  }

  void OnFrameStoppedLoading(
      const page::FrameStoppedLoadingParams& params) override {
    devtools_client_->GetPage()->Disable();
    devtools_client_->GetPage()->GetExperimental()->RemoveObserver(this);

    // Check that a non-experimental command which has no return value can be
    // called with a void() callback.
    devtools_client_->GetPage()->Reload(
        page::ReloadParams::Builder().Build(),
        base::BindOnce(&HeadlessDevToolsClientExperimentalTest::OnReloadStarted,
                       base::Unretained(this)));
  }

  void OnReloadStarted() { FinishAsynchronousTest(); }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientExperimentalTest);

class TargetDomainCreateAndDeletePageTest
    : public HeadlessAsyncDevTooledBrowserTest {
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());

    EXPECT_EQ(1u, GetAllWebContents(browser()).size());

    devtools_client_->GetTarget()->GetExperimental()->CreateTarget(
        target::CreateTargetParams::Builder()
            .SetUrl(embedded_test_server()->GetURL("/hello.html").spec())
            .SetWidth(1)
            .SetHeight(1)
            .Build(),
        base::BindOnce(
            &TargetDomainCreateAndDeletePageTest::OnCreateTargetResult,
            base::Unretained(this)));
  }

  void OnCreateTargetResult(
      std::unique_ptr<target::CreateTargetResult> result) {
    EXPECT_EQ(2u, GetAllWebContents(browser()).size());

    HeadlessWebContentsImpl* contents = HeadlessWebContentsImpl::From(
        browser()->GetWebContentsForDevToolsAgentHostId(result->GetTargetId()));
    EXPECT_SIZE_EQ(gfx::Size(1, 1), contents->web_contents()
                                        ->GetRenderWidgetHostView()
                                        ->GetViewBounds()
                                        .size());

    devtools_client_->GetTarget()->GetExperimental()->CloseTarget(
        target::CloseTargetParams::Builder()
            .SetTargetId(result->GetTargetId())
            .Build(),
        base::BindOnce(
            &TargetDomainCreateAndDeletePageTest::OnCloseTargetResult,
            base::Unretained(this)));
  }

  void OnCloseTargetResult(std::unique_ptr<target::CloseTargetResult> result) {
    EXPECT_TRUE(result->GetSuccess());
    EXPECT_EQ(1u, GetAllWebContents(browser()).size());
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(TargetDomainCreateAndDeletePageTest);

class TargetDomainCreateAndDeleteBrowserContextTest
    : public HeadlessAsyncDevTooledBrowserTest {
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());

    EXPECT_EQ(1u, GetAllWebContents(browser()).size());

    devtools_client_->GetTarget()->GetExperimental()->CreateBrowserContext(
        target::CreateBrowserContextParams::Builder().Build(),
        base::BindOnce(&TargetDomainCreateAndDeleteBrowserContextTest::
                           OnCreateContextResult,
                       base::Unretained(this)));
  }

  void OnCreateContextResult(
      std::unique_ptr<target::CreateBrowserContextResult> result) {
    browser_context_id_ = result->GetBrowserContextId();

    devtools_client_->GetTarget()->GetExperimental()->CreateTarget(
        target::CreateTargetParams::Builder()
            .SetUrl(embedded_test_server()->GetURL("/hello.html").spec())
            .SetBrowserContextId(result->GetBrowserContextId())
            .SetWidth(1)
            .SetHeight(1)
            .Build(),
        base::BindOnce(&TargetDomainCreateAndDeleteBrowserContextTest::
                           OnCreateTargetResult,
                       base::Unretained(this)));
  }

  void OnCreateTargetResult(
      std::unique_ptr<target::CreateTargetResult> result) {
    EXPECT_EQ(2u, GetAllWebContents(browser()).size());

    devtools_client_->GetTarget()->GetExperimental()->CloseTarget(
        target::CloseTargetParams::Builder()
            .SetTargetId(result->GetTargetId())
            .Build(),
        base::BindOnce(
            &TargetDomainCreateAndDeleteBrowserContextTest::OnCloseTargetResult,
            base::Unretained(this)));
  }

  void OnCloseTargetResult(std::unique_ptr<target::CloseTargetResult> result) {
    EXPECT_EQ(1u, GetAllWebContents(browser()).size());
    EXPECT_TRUE(result->GetSuccess());

    devtools_client_->GetTarget()->GetExperimental()->DisposeBrowserContext(
        target::DisposeBrowserContextParams::Builder()
            .SetBrowserContextId(browser_context_id_)
            .Build(),
        base::BindOnce(&TargetDomainCreateAndDeleteBrowserContextTest::
                           OnDisposeBrowserContextResult,
                       base::Unretained(this)));
  }

  void OnDisposeBrowserContextResult(
      std::unique_ptr<target::DisposeBrowserContextResult> result) {
    devtools_client_->GetTarget()->GetExperimental()->GetBrowserContexts(
        target::GetBrowserContextsParams::Builder().Build(),
        base::BindOnce(&TargetDomainCreateAndDeleteBrowserContextTest::
                           OnGetBrowserContexts,
                       base::Unretained(this)));
  }

  void OnGetBrowserContexts(
      std::unique_ptr<target::GetBrowserContextsResult> result) {
    const std::vector<std::string>* contexts = result->GetBrowserContextIds();
    EXPECT_EQ(0u, contexts->size());
    FinishAsynchronousTest();
  }

 private:
  std::string browser_context_id_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(TargetDomainCreateAndDeleteBrowserContextTest);

class TargetDomainDisposeContextSucceedsIfInUse
    : public target::Observer,
      public HeadlessAsyncDevTooledBrowserTest {
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    EXPECT_EQ(1u, GetAllWebContents(browser()).size());

    devtools_client_->GetTarget()->AddObserver(this);
    devtools_client_->GetTarget()->SetDiscoverTargets(
        target::SetDiscoverTargetsParams::Builder().SetDiscover(true).Build(),
        base::BindOnce(&TargetDomainDisposeContextSucceedsIfInUse::
                           OnDiscoverTargetsEnabled,
                       base::Unretained(this)));
  }

  void OnDiscoverTargetsEnabled(
      std::unique_ptr<target::SetDiscoverTargetsResult> result) {
    devtools_client_->GetTarget()->GetExperimental()->CreateBrowserContext(
        target::CreateBrowserContextParams::Builder().Build(),
        base::BindOnce(
            &TargetDomainDisposeContextSucceedsIfInUse::OnContextCreated,
            base::Unretained(this)));
  }

  void OnContextCreated(
      std::unique_ptr<target::CreateBrowserContextResult> result) {
    context_id_ = result->GetBrowserContextId();

    devtools_client_->GetTarget()->GetExperimental()->CreateTarget(
        target::CreateTargetParams::Builder()
            .SetUrl(embedded_test_server()->GetURL("/hello.html").spec())
            .SetBrowserContextId(context_id_)
            .Build(),
        base::BindOnce(
            &TargetDomainDisposeContextSucceedsIfInUse::OnCreateTargetResult,
            base::Unretained(this)));
  }

  void OnCreateTargetResult(
      std::unique_ptr<target::CreateTargetResult> result) {
    page_id_ = result->GetTargetId();

    destroyed_targets_.clear();
    devtools_client_->GetTarget()->GetExperimental()->DisposeBrowserContext(
        target::DisposeBrowserContextParams::Builder()
            .SetBrowserContextId(context_id_)
            .Build(),
        base::BindOnce(&TargetDomainDisposeContextSucceedsIfInUse::
                           OnDisposeBrowserContextResult,
                       base::Unretained(this)));
  }

  void OnTargetDestroyed(const target::TargetDestroyedParams& params) override {
    destroyed_targets_.push_back(params.GetTargetId());
  }

  void OnDisposeBrowserContextResult(
      std::unique_ptr<target::DisposeBrowserContextResult> result) {
    EXPECT_EQ(destroyed_targets_.size(), 1u);
    EXPECT_EQ(destroyed_targets_[0], page_id_);
    devtools_client_->GetTarget()->RemoveObserver(this);
    FinishAsynchronousTest();
  }

 private:
  std::vector<std::string> destroyed_targets_;
  std::string context_id_;
  std::string page_id_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(TargetDomainDisposeContextSucceedsIfInUse);

class TargetDomainCreateTwoContexts : public HeadlessAsyncDevTooledBrowserTest,
                                      public target::Observer,
                                      public page::Observer {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());

    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->AddObserver(this);
    devtools_client_->GetPage()->Enable(run_loop.QuitClosure());
    run_loop.Run();

    devtools_client_->GetTarget()->AddObserver(this);
    devtools_client_->GetTarget()->SetDiscoverTargets(
        target::SetDiscoverTargetsParams::Builder().SetDiscover(true).Build(),
        base::BindOnce(&TargetDomainCreateTwoContexts::OnDiscoverTargetsEnabled,
                       base::Unretained(this)));
  }

  void OnDiscoverTargetsEnabled(
      std::unique_ptr<target::SetDiscoverTargetsResult> result) {
    devtools_client_->GetTarget()->GetExperimental()->CreateBrowserContext(
        target::CreateBrowserContextParams::Builder().Build(),
        base::BindOnce(&TargetDomainCreateTwoContexts::OnContextOneCreated,
                       base::Unretained(this)));

    devtools_client_->GetTarget()->GetExperimental()->CreateBrowserContext(
        target::CreateBrowserContextParams::Builder().Build(),
        base::BindOnce(&TargetDomainCreateTwoContexts::OnContextTwoCreated,
                       base::Unretained(this)));
  }

  void OnContextOneCreated(
      std::unique_ptr<target::CreateBrowserContextResult> result) {
    context_id_one_ = result->GetBrowserContextId();
    MaybeCreatePages();
  }

  void OnContextTwoCreated(
      std::unique_ptr<target::CreateBrowserContextResult> result) {
    context_id_two_ = result->GetBrowserContextId();
    MaybeCreatePages();
  }

  void MaybeCreatePages() {
    if (context_id_one_.empty() || context_id_two_.empty())
      return;

    devtools_client_->GetTarget()->GetExperimental()->GetBrowserContexts(
        target::GetBrowserContextsParams::Builder().Build(),
        base::BindOnce(&TargetDomainCreateTwoContexts::OnGetBrowserContexts,
                       base::Unretained(this)));
  }

  void OnGetBrowserContexts(
      std::unique_ptr<target::GetBrowserContextsResult> result) {
    const std::vector<std::string>* contexts = result->GetBrowserContextIds();
    EXPECT_EQ(2u, contexts->size());
    EXPECT_TRUE(std::find(contexts->begin(), contexts->end(),
                          context_id_one_) != contexts->end());
    EXPECT_TRUE(std::find(contexts->begin(), contexts->end(),
                          context_id_two_) != contexts->end());

    devtools_client_->GetTarget()->GetExperimental()->CreateTarget(
        target::CreateTargetParams::Builder()
            .SetUrl("about://blank")
            .SetBrowserContextId(context_id_one_)
            .Build(),
        base::BindOnce(&TargetDomainCreateTwoContexts::OnCreateTargetOneResult,
                       base::Unretained(this)));

    devtools_client_->GetTarget()->GetExperimental()->CreateTarget(
        target::CreateTargetParams::Builder()
            .SetUrl("about://blank")
            .SetBrowserContextId(context_id_two_)
            .Build(),
        base::BindOnce(&TargetDomainCreateTwoContexts::OnCreateTargetTwoResult,
                       base::Unretained(this)));
  }

  void OnCreateTargetOneResult(
      std::unique_ptr<target::CreateTargetResult> result) {
    page_id_one_ = result->GetTargetId();
    MaybeTestIsolation();
  }

  void OnCreateTargetTwoResult(
      std::unique_ptr<target::CreateTargetResult> result) {
    page_id_two_ = result->GetTargetId();
    MaybeTestIsolation();
  }

  void MaybeTestIsolation() {
    if (page_id_one_.empty() || page_id_two_.empty())
      return;

    devtools_client_->GetTarget()->GetExperimental()->AttachToTarget(
        target::AttachToTargetParams::Builder()
            .SetTargetId(page_id_one_)
            .Build(),
        base::BindOnce(&TargetDomainCreateTwoContexts::OnAttachedToTargetOne,
                       base::Unretained(this)));

    devtools_client_->GetTarget()->GetExperimental()->AttachToTarget(
        target::AttachToTargetParams::Builder()
            .SetTargetId(page_id_two_)
            .Build(),
        base::BindOnce(&TargetDomainCreateTwoContexts::OnAttachedToTargetTwo,
                       base::Unretained(this)));
  }

  void OnAttachedToTargetOne(
      std::unique_ptr<target::AttachToTargetResult> result) {
    StopNavigationOnTarget(101, page_id_one_);
  }

  void OnAttachedToTargetTwo(
      std::unique_ptr<target::AttachToTargetResult> result) {
    StopNavigationOnTarget(102, page_id_two_);
  }

  void StopNavigationOnTarget(int message_id, std::string target_id) {
    // Avoid triggering Page.loadEventFired for about://blank if loading hasn't
    // finished yet.
    devtools_client_->GetTarget()->GetExperimental()->SendMessageToTarget(
        target::SendMessageToTargetParams::Builder()
            .SetTargetId(target_id)
            .SetMessage("{\"id\":" + std::to_string(message_id) +
                        ", \"method\": \"Page.stopLoading\"}")
            .Build());
  }

  void EnablePageOnTarget(int message_id, std::string target_id) {
    devtools_client_->GetTarget()->GetExperimental()->SendMessageToTarget(
        target::SendMessageToTargetParams::Builder()
            .SetTargetId(target_id)
            .SetMessage("{\"id\":" + std::to_string(message_id) +
                        ", \"method\": \"Page.enable\"}")
            .Build());
  }

  void NavigateTarget(int message_id, std::string target_id) {
    devtools_client_->GetTarget()->GetExperimental()->SendMessageToTarget(
        target::SendMessageToTargetParams::Builder()
            .SetTargetId(target_id)
            .SetMessage(
                "{\"id\":" + std::to_string(message_id) +
                ", \"method\": \"Page.navigate\", \"params\": {\"url\": \"" +
                embedded_test_server()->GetURL("/hello.html").spec() + "\"}}")
            .Build());
  }

  void MaybeSetCookieOnPageOne() {
    if (!page_one_loaded_ || !page_two_loaded_)
      return;

    devtools_client_->GetTarget()->GetExperimental()->SendMessageToTarget(
        target::SendMessageToTargetParams::Builder()
            .SetTargetId(page_id_one_)
            .SetMessage("{\"id\":401, \"method\": \"Runtime.evaluate\", "
                        "\"params\": {\"expression\": "
                        "\"document.cookie = 'foo=bar';\"}}")
            .Build());
  }

  void OnReceivedMessageFromTarget(
      const target::ReceivedMessageFromTargetParams& params) override {
    std::unique_ptr<base::Value> message =
        base::JSONReader::Read(params.GetMessage(), base::JSON_PARSE_RFC);
    const base::DictionaryValue* message_dict;
    if (!message || !message->GetAsDictionary(&message_dict)) {
      return;
    }

    const base::Value* method_value = message_dict->FindKey("method");
    if (method_value && method_value->GetString() == "Page.loadEventFired") {
      if (params.GetTargetId() == page_id_one_) {
        page_one_loaded_ = true;
      } else if (params.GetTargetId() == page_id_two_) {
        page_two_loaded_ = true;
      }
      MaybeSetCookieOnPageOne();
      return;
    }

    const base::Value* id_value = message_dict->FindKey("id");
    if (!id_value)
      return;
    int message_id = id_value->GetInt();
    const base::DictionaryValue* result_dict;
    if (message_dict->GetDictionary("result", &result_dict)) {
      if (message_id == 101) {
        // 101: Page.stopNavigation on target one.
        EXPECT_EQ(page_id_one_, params.GetTargetId());
        EnablePageOnTarget(201, page_id_one_);
      } else if (message_id == 102) {
        // 102: Page.stopNavigation on target two.
        EXPECT_EQ(page_id_two_, params.GetTargetId());
        EnablePageOnTarget(202, page_id_two_);
      } else if (message_id == 201) {
        // 201: Page.enable on target one.
        EXPECT_EQ(page_id_one_, params.GetTargetId());
        NavigateTarget(301, page_id_one_);
      } else if (message_id == 202) {
        // 202: Page.enable on target two.
        EXPECT_EQ(page_id_two_, params.GetTargetId());
        NavigateTarget(302, page_id_two_);
      } else if (message_id == 401) {
        // 401: Runtime.evaluate on target one.
        EXPECT_EQ(page_id_one_, params.GetTargetId());

        // TODO(alexclarke): Make some better bindings
        // for Target.SendMessageToTarget.
        devtools_client_->GetTarget()->GetExperimental()->SendMessageToTarget(
            target::SendMessageToTargetParams::Builder()
                .SetTargetId(page_id_two_)
                .SetMessage("{\"id\":402, \"method\": \"Runtime.evaluate\", "
                            "\"params\": {\"expression\": "
                            "\"document.cookie;\"}}")
                .Build());
      } else if (message_id == 402) {
        // 402: Runtime.evaluate on target two.
        EXPECT_EQ(page_id_two_, params.GetTargetId());

        // There's a nested result. We want the inner one.
        EXPECT_TRUE(result_dict->GetDictionary("result", &result_dict));

        const base::Value* value_value = result_dict->FindKey("value");
        ASSERT_THAT(value_value, NotNull());
        EXPECT_EQ("", value_value->GetString())
            << "Page 2 should not share cookies from page one";

        devtools_client_->GetTarget()->GetExperimental()->DisposeBrowserContext(
            target::DisposeBrowserContextParams::Builder()
                .SetBrowserContextId(context_id_one_)
                .Build(),
            base::BindOnce(&TargetDomainCreateTwoContexts::OnCloseContext,
                           base::Unretained(this)));

        devtools_client_->GetTarget()->GetExperimental()->DisposeBrowserContext(
            target::DisposeBrowserContextParams::Builder()
                .SetBrowserContextId(context_id_two_)
                .Build(),
            base::BindOnce(&TargetDomainCreateTwoContexts::OnCloseContext,
                           base::Unretained(this)));
      }
    }
  }

  void OnTargetDestroyed(const target::TargetDestroyedParams& params) override {
    ++page_close_count_;
  }

  void OnCloseContext(
      std::unique_ptr<target::DisposeBrowserContextResult> result) {
    if (++context_closed_count_ < 2)
      return;
    EXPECT_EQ(page_close_count_, 2);

    devtools_client_->GetTarget()->RemoveObserver(this);
    FinishAsynchronousTest();
  }

 private:
  std::string context_id_one_;
  std::string context_id_two_;
  std::string page_id_one_;
  std::string page_id_two_;
  bool page_one_loaded_ = false;
  bool page_two_loaded_ = false;
  int page_close_count_ = 0;
  int context_closed_count_ = 0;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(TargetDomainCreateTwoContexts);

class HeadlessDevToolsNavigationControlTest
    : public HeadlessAsyncDevTooledBrowserTest,
      network::ExperimentalObserver,
      page::ExperimentalObserver {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->GetExperimental()->AddObserver(this);
    devtools_client_->GetNetwork()->GetExperimental()->AddObserver(this);
    devtools_client_->GetPage()->Enable(run_loop.QuitClosure());
    run_loop.Run();
    devtools_client_->GetNetwork()->Enable();

    std::unique_ptr<headless::network::RequestPattern> match_all =
        headless::network::RequestPattern::Builder().SetUrlPattern("*").Build();
    std::vector<std::unique_ptr<headless::network::RequestPattern>> patterns;
    patterns.push_back(std::move(match_all));
    devtools_client_->GetNetwork()->GetExperimental()->SetRequestInterception(
        network::SetRequestInterceptionParams::Builder()
            .SetPatterns(std::move(patterns))
            .Build());
    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/hello.html").spec());
  }

  void OnRequestIntercepted(
      const network::RequestInterceptedParams& params) override {
    if (params.GetIsNavigationRequest())
      navigation_requested_ = true;
    // Allow the navigation to proceed.
    devtools_client_->GetNetwork()
        ->GetExperimental()
        ->ContinueInterceptedRequest(
            network::ContinueInterceptedRequestParams::Builder()
                .SetInterceptionId(params.GetInterceptionId())
                .Build());
  }

  void OnFrameStoppedLoading(
      const page::FrameStoppedLoadingParams& params) override {
    EXPECT_TRUE(navigation_requested_);
    FinishAsynchronousTest();
  }

 private:
  bool navigation_requested_ = false;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsNavigationControlTest);

class HeadlessCrashObserverTest : public HeadlessAsyncDevTooledBrowserTest,
                                  inspector::ExperimentalObserver {
 public:
  void RunDevTooledTest() override {
    devtools_client_->GetInspector()->GetExperimental()->AddObserver(this);
    devtools_client_->GetInspector()->GetExperimental()->Enable(
        inspector::EnableParams::Builder().Build());
    devtools_client_->GetPage()->Enable();
    devtools_client_->GetPage()->Navigate(content::kChromeUICrashURL);
  }

  void OnTargetCrashed(const inspector::TargetCrashedParams& params) override {
    FinishAsynchronousTest();
    render_process_exited_ = true;
  }

  // Make sure we don't fail because the renderer crashed!
  void RenderProcessExited(base::TerminationStatus status,
                           int exit_code) override {
#if defined(OS_WIN) || defined(OS_MACOSX)
    EXPECT_EQ(base::TERMINATION_STATUS_PROCESS_CRASHED, status);
#else
    EXPECT_EQ(base::TERMINATION_STATUS_ABNORMAL_TERMINATION, status);
#endif  // defined(OS_WIN) || defined(OS_MACOSX)
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessCrashObserverTest);

class HeadlessDevToolsClientAttachTest
    : public HeadlessAsyncDevTooledBrowserTest {
 public:
  void RunDevTooledTest() override {
    other_devtools_client_ = HeadlessDevToolsClient::Create();
    HeadlessDevToolsTarget* devtools_target =
        web_contents_->GetDevToolsTarget();

    EXPECT_TRUE(devtools_target->IsAttached());
    // Detach the existing client, attach the other client.
    devtools_target->DetachClient(devtools_client_.get());
    EXPECT_FALSE(devtools_target->IsAttached());
    devtools_target->AttachClient(other_devtools_client_.get());
    EXPECT_TRUE(devtools_target->IsAttached());

    // Now, let's make sure this devtools client works.
    other_devtools_client_->GetRuntime()->Evaluate(
        "24 * 7",
        base::BindOnce(&HeadlessDevToolsClientAttachTest::OnFirstResult,
                       base::Unretained(this)));
  }

  void OnFirstResult(std::unique_ptr<runtime::EvaluateResult> result) {
    EXPECT_TRUE(result->GetResult()->HasValue());
    EXPECT_EQ(24 * 7, result->GetResult()->GetValue()->GetInt());

    HeadlessDevToolsTarget* devtools_target =
        web_contents_->GetDevToolsTarget();

    EXPECT_TRUE(devtools_target->IsAttached());
    devtools_target->DetachClient(other_devtools_client_.get());
    EXPECT_FALSE(devtools_target->IsAttached());
    devtools_target->AttachClient(devtools_client_.get());
    EXPECT_TRUE(devtools_target->IsAttached());

    devtools_client_->GetRuntime()->Evaluate(
        "27 * 4",
        base::BindOnce(&HeadlessDevToolsClientAttachTest::OnSecondResult,
                       base::Unretained(this)));
  }

  void OnSecondResult(std::unique_ptr<runtime::EvaluateResult> result) {
    EXPECT_TRUE(result->GetResult()->HasValue());
    EXPECT_EQ(27 * 4, result->GetResult()->GetValue()->GetInt());

    // If everything worked, this call will not crash, since it
    // detaches devtools_client_.
    FinishAsynchronousTest();
  }

 protected:
  std::unique_ptr<HeadlessDevToolsClient> other_devtools_client_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsClientAttachTest);

class HeadlessDevToolsMethodCallErrorTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public page::Observer {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->AddObserver(this);
    devtools_client_->GetPage()->Enable(run_loop.QuitClosure());
    run_loop.Run();
    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/hello.html").spec());
  }

  void OnLoadEventFired(const page::LoadEventFiredParams& params) override {
    devtools_client_->GetPage()->GetExperimental()->RemoveObserver(this);
    devtools_client_->GetDOM()->GetDocument(
        base::BindOnce(&HeadlessDevToolsMethodCallErrorTest::OnGetDocument,
                       base::Unretained(this)));
  }

  void OnGetDocument(std::unique_ptr<dom::GetDocumentResult> result) {
    devtools_client_->GetDOM()->QuerySelector(
        dom::QuerySelectorParams::Builder()
            .SetNodeId(result->GetRoot()->GetNodeId())
            .SetSelector("<o_O>")
            .Build(),
        base::BindOnce(&HeadlessDevToolsMethodCallErrorTest::OnQuerySelector,
                       base::Unretained(this)));
  }

  void OnQuerySelector(std::unique_ptr<dom::QuerySelectorResult> result) {
    EXPECT_EQ(nullptr, result);
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsMethodCallErrorTest);

class HeadlessDevToolsNetworkBlockedUrlTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public page::Observer,
      public network::Observer {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->AddObserver(this);
    devtools_client_->GetPage()->Enable();
    devtools_client_->GetNetwork()->AddObserver(this);
    devtools_client_->GetNetwork()->Enable(run_loop.QuitClosure());
    run_loop.Run();
    std::vector<std::string> blockedUrls;
    blockedUrls.push_back("dom_tree_test.css");
    devtools_client_->GetNetwork()->GetExperimental()->SetBlockedURLs(
        network::SetBlockedURLsParams::Builder().SetUrls(blockedUrls).Build());
    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/dom_tree_test.html").spec());
  }

  std::string GetUrlPath(const std::string& url) const {
    GURL gurl(url);
    return gurl.path();
  }

  void OnRequestWillBeSent(
      const network::RequestWillBeSentParams& params) override {
    std::string path = GetUrlPath(params.GetRequest()->GetUrl());
    requests_to_be_sent_.push_back(path);
    request_id_to_path_[params.GetRequestId()] = path;
  }

  void OnResponseReceived(
      const network::ResponseReceivedParams& params) override {
    responses_received_.push_back(GetUrlPath(params.GetResponse()->GetUrl()));
  }

  void OnLoadingFailed(const network::LoadingFailedParams& failed) override {
    failures_.push_back(request_id_to_path_[failed.GetRequestId()]);
    EXPECT_EQ(network::BlockedReason::INSPECTOR, failed.GetBlockedReason());
  }

  void OnLoadEventFired(const page::LoadEventFiredParams&) override {
    EXPECT_THAT(
        requests_to_be_sent_,
        testing::UnorderedElementsAre("/dom_tree_test.html",
                                      "/dom_tree_test.css", "/iframe.html"));
    EXPECT_THAT(responses_received_,
                ElementsAre("/dom_tree_test.html", "/iframe.html"));
    EXPECT_THAT(failures_, ElementsAre("/dom_tree_test.css"));
    FinishAsynchronousTest();
  }

  std::map<std::string, std::string> request_id_to_path_;
  std::vector<std::string> requests_to_be_sent_;
  std::vector<std::string> responses_received_;
  std::vector<std::string> failures_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessDevToolsNetworkBlockedUrlTest);

namespace {
// Keep in sync with X_DevTools_Emulate_Network_Conditions_Client_Id defined in
// HTTPNames.json5.
const char kDevToolsEmulateNetworkConditionsClientId[] =
    "X-DevTools-Emulate-Network-Conditions-Client-Id";
}  // namespace

class DevToolsHeaderStrippingTest : public HeadlessAsyncDevTooledBrowserTest,
                                    public page::Observer,
                                    public network::Observer {
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->AddObserver(this);
    devtools_client_->GetPage()->Enable();
    // Enable network domain in order to get DevTools to add the header.
    devtools_client_->GetNetwork()->AddObserver(this);
    devtools_client_->GetNetwork()->Enable(run_loop.QuitClosure());
    run_loop.Run();
    devtools_client_->GetPage()->Navigate(
        "http://not-an-actual-domain.tld/hello.html");
  }

  ProtocolHandlerMap GetProtocolHandlers() override {
    const std::string kResponseBody = "<p>HTTP response body</p>";
    ProtocolHandlerMap protocol_handlers;
    protocol_handlers[url::kHttpScheme] =
        std::make_unique<TestProtocolHandler>(kResponseBody);
    test_handler_ = static_cast<TestProtocolHandler*>(
        protocol_handlers[url::kHttpScheme].get());
    return protocol_handlers;
  }

  void OnLoadEventFired(const page::LoadEventFiredParams&) override {
    EXPECT_FALSE(test_handler_->last_http_request_headers().IsEmpty());
    EXPECT_FALSE(test_handler_->last_http_request_headers().HasHeader(
        kDevToolsEmulateNetworkConditionsClientId));
    FinishAsynchronousTest();
  }

  TestProtocolHandler* test_handler_;  // NOT OWNED
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(DevToolsHeaderStrippingTest);

class DevToolsNetworkOfflineEmulationTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public page::Observer,
      public network::Observer {
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->AddObserver(this);
    devtools_client_->GetPage()->Enable();
    devtools_client_->GetNetwork()->AddObserver(this);
    devtools_client_->GetNetwork()->Enable(run_loop.QuitClosure());
    run_loop.Run();
    std::unique_ptr<network::EmulateNetworkConditionsParams> params =
        network::EmulateNetworkConditionsParams::Builder()
            .SetOffline(true)
            .SetLatency(0)
            .SetDownloadThroughput(0)
            .SetUploadThroughput(0)
            .Build();
    devtools_client_->GetNetwork()->EmulateNetworkConditions(std::move(params));
    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/hello.html").spec());
  }

  void OnLoadingFailed(const network::LoadingFailedParams& failed) override {
    EXPECT_EQ("net::ERR_INTERNET_DISCONNECTED", failed.GetErrorText());
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(DevToolsNetworkOfflineEmulationTest);

class RawDevtoolsProtocolTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public HeadlessDevToolsClient::RawProtocolListener {
 public:
  void RunDevTooledTest() override {
    devtools_client_->SetRawProtocolListener(this);

    base::DictionaryValue message;
    message.SetInteger("id", devtools_client_->GetNextRawDevToolsMessageId());
    message.SetString("method", "Runtime.evaluate");
    std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
    params->SetString("expression", "1+1");
    message.Set("params", std::move(params));
    devtools_client_->SendRawDevToolsMessage(message);
  }

  bool OnProtocolMessage(const std::string& devtools_agent_host_id,
                         const std::string& json_message,
                         const base::DictionaryValue& parsed_message) override {
    EXPECT_EQ(
        "{\"id\":1,\"result\":{\"result\":{\"type\":\"number\","
        "\"value\":2,\"description\":\"2\"}}}",
        json_message);

    FinishAsynchronousTest();
    return true;
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(RawDevtoolsProtocolTest);

class DevToolsAttachAndDetachNotifications
    : public HeadlessAsyncDevTooledBrowserTest {
 public:
  void DevToolsClientAttached() override { dev_tools_client_attached_ = true; }

  void RunDevTooledTest() override {
    EXPECT_TRUE(dev_tools_client_attached_);
    FinishAsynchronousTest();
  }

  void DevToolsClientDetached() override { dev_tools_client_detached_ = true; }

  void TearDownOnMainThread() override {
    EXPECT_TRUE(dev_tools_client_detached_);
  }

 private:
  bool dev_tools_client_attached_ = false;
  bool dev_tools_client_detached_ = false;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(DevToolsAttachAndDetachNotifications);

class DomTreeExtractionBrowserTest : public HeadlessAsyncDevTooledBrowserTest,
                                     public page::Observer {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    devtools_client_->GetPage()->AddObserver(this);
    devtools_client_->GetPage()->Enable();
    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/dom_tree_test.html").spec());
  }

  void OnLoadEventFired(const page::LoadEventFiredParams& params) override {
    devtools_client_->GetPage()->Disable();
    devtools_client_->GetPage()->RemoveObserver(this);

    std::vector<std::string> css_whitelist = {
        "color",       "display",      "font-style", "font-family",
        "margin-left", "margin-right", "margin-top", "margin-bottom"};
    devtools_client_->GetDOMSnapshot()->GetExperimental()->GetSnapshot(
        dom_snapshot::GetSnapshotParams::Builder()
            .SetComputedStyleWhitelist(std::move(css_whitelist))
            .Build(),
        base::BindOnce(&DomTreeExtractionBrowserTest::OnGetSnapshotResult,
                       base::Unretained(this)));
  }

  void OnGetSnapshotResult(
      std::unique_ptr<dom_snapshot::GetSnapshotResult> result) {
    GURL::Replacements replace_port;
    replace_port.SetPortStr("");

    std::vector<std::unique_ptr<base::DictionaryValue>> dom_nodes(
        result->GetDomNodes()->size());

    // For convenience, flatten the dom tree into an array of dicts.
    for (size_t i = 0; i < result->GetDomNodes()->size(); i++) {
      dom_snapshot::DOMNode* node = (*result->GetDomNodes())[i].get();

      dom_nodes[i].reset(
          static_cast<base::DictionaryValue*>(node->Serialize().release()));
      base::DictionaryValue* node_dict = dom_nodes[i].get();

      // Frame IDs are random.
      if (node_dict->FindKey("frameId"))
        node_dict->SetString("frameId", "?");

      // Ports are random.
      if (base::Value* base_url_value = node_dict->FindKey("baseURL")) {
        node_dict->SetString("baseURL", GURL(base_url_value->GetString())
                                            .ReplaceComponents(replace_port)
                                            .spec());
      }

      if (base::Value* document_url_value = node_dict->FindKey("documentURL")) {
        node_dict->SetString("documentURL",
                             GURL(document_url_value->GetString())
                                 .ReplaceComponents(replace_port)
                                 .spec());
      }

      // Merge LayoutTreeNode data into the dictionary.
      if (base::Value* layout_node_index_value =
          node_dict->FindKey("layoutNodeIndex")) {
        int layout_node_index = layout_node_index_value->GetInt();
        ASSERT_LE(0, layout_node_index);
        ASSERT_GT(result->GetLayoutTreeNodes()->size(),
                  static_cast<size_t>(layout_node_index));
        const std::unique_ptr<dom_snapshot::LayoutTreeNode>& layout_node =
            (*result->GetLayoutTreeNodes())[layout_node_index];

        node_dict->Set("boundingBox",
                       layout_node->GetBoundingBox()->Serialize());

        if (layout_node->HasLayoutText())
          node_dict->SetString("layoutText", layout_node->GetLayoutText());

        if (layout_node->HasStyleIndex())
          node_dict->SetInteger("styleIndex", layout_node->GetStyleIndex());

        if (layout_node->HasInlineTextNodes()) {
          std::unique_ptr<base::ListValue> inline_text_nodes(
              new base::ListValue());
          for (const std::unique_ptr<dom_snapshot::InlineTextBox>&
                   inline_text_box : *layout_node->GetInlineTextNodes()) {
            size_t index = inline_text_nodes->GetSize();
            inline_text_nodes->Set(index, inline_text_box->Serialize());
          }
          node_dict->Set("inlineTextNodes", std::move(inline_text_nodes));
        }
      }
    }

    std::vector<std::unique_ptr<base::DictionaryValue>> computed_styles(
        result->GetComputedStyles()->size());

    for (size_t i = 0; i < result->GetComputedStyles()->size(); i++) {
      std::unique_ptr<base::DictionaryValue> style(new base::DictionaryValue());
      for (const auto& style_property :
           *(*result->GetComputedStyles())[i]->GetProperties()) {
        style->SetString(style_property->GetName(), style_property->GetValue());
      }
      computed_styles[i] = std::move(style);
    }

    base::ThreadRestrictions::SetIOAllowed(true);
    base::FilePath source_root_dir;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &source_root_dir);
    base::FilePath expected_dom_nodes_path =
        source_root_dir.Append(FILE_PATH_LITERAL(
            "headless/lib/dom_tree_extraction_expected_nodes.txt"));
    std::string expected_dom_nodes;
    ASSERT_TRUE(
        base::ReadFileToString(expected_dom_nodes_path, &expected_dom_nodes));

    std::string dom_nodes_result;
    for (size_t i = 0; i < dom_nodes.size(); i++) {
      std::string result_json;
      base::JSONWriter::WriteWithOptions(
          *dom_nodes[i], base::JSONWriter::OPTIONS_PRETTY_PRINT, &result_json);

      dom_nodes_result += result_json;
    }

#if defined(OS_WIN)
    ASSERT_TRUE(base::RemoveChars(dom_nodes_result, "\r", &dom_nodes_result));
#endif

    EXPECT_EQ(expected_dom_nodes, dom_nodes_result);

    base::FilePath expected_styles_path =
        source_root_dir.Append(FILE_PATH_LITERAL(
            "headless/lib/dom_tree_extraction_expected_styles.txt"));
    std::string expected_computed_styles;
    ASSERT_TRUE(base::ReadFileToString(expected_styles_path,
                                       &expected_computed_styles));

    std::string computed_styles_result;
    for (size_t i = 0; i < computed_styles.size(); i++) {
      std::string result_json;
      base::JSONWriter::WriteWithOptions(*computed_styles[i],
                                         base::JSONWriter::OPTIONS_PRETTY_PRINT,
                                         &result_json);

      computed_styles_result += result_json;
    }

#if defined(OS_WIN)
    ASSERT_TRUE(base::RemoveChars(computed_styles_result, "\r",
                                  &computed_styles_result));
#endif

    EXPECT_EQ(expected_computed_styles, computed_styles_result);
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(DomTreeExtractionBrowserTest);

class UrlRequestFailedTest : public HeadlessAsyncDevTooledBrowserTest,
                             public HeadlessBrowserContext::Observer,
                             public network::ExperimentalObserver,
                             public page::Observer {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    devtools_client_->GetNetwork()->GetExperimental()->AddObserver(this);
    devtools_client_->GetNetwork()->Enable();

    std::unique_ptr<headless::network::RequestPattern> match_all =
        headless::network::RequestPattern::Builder().SetUrlPattern("*").Build();
    std::vector<std::unique_ptr<headless::network::RequestPattern>> patterns;
    patterns.push_back(std::move(match_all));
    devtools_client_->GetNetwork()->GetExperimental()->SetRequestInterception(
        network::SetRequestInterceptionParams::Builder()
            .SetPatterns(std::move(patterns))
            .Build());

    browser_context_->AddObserver(this);

    devtools_client_->GetPage()->AddObserver(this);

    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->Enable(run_loop.QuitClosure());
    run_loop.Run();

    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/resource_cancel_test.html").spec());
  }

  bool ShouldCancel(const std::string& url) {
    if (EndsWith(url, "/iframe.html", base::CompareCase::INSENSITIVE_ASCII))
      return true;

    if (EndsWith(url, "/test.jpg", base::CompareCase::INSENSITIVE_ASCII))
      return true;

    return false;
  }

  void OnRequestIntercepted(
      const network::RequestInterceptedParams& params) override {
    urls_seen_.push_back(GURL(params.GetRequest()->GetUrl()).ExtractFileName());
    auto continue_intercept_params =
        network::ContinueInterceptedRequestParams::Builder()
            .SetInterceptionId(params.GetInterceptionId())
            .Build();
    if (ShouldCancel(params.GetRequest()->GetUrl()))
      continue_intercept_params->SetErrorReason(GetErrorReason());

    devtools_client_->GetNetwork()
        ->GetExperimental()
        ->ContinueInterceptedRequest(std::move(continue_intercept_params));
  }

  void OnLoadEventFired(const page::LoadEventFiredParams&) override {
    browser_context_->RemoveObserver(this);

    base::AutoLock lock(lock_);
    EXPECT_THAT(urls_that_failed_to_load_,
                ElementsAre("test.jpg", "iframe.html"));
    EXPECT_THAT(
        urls_seen_,
        ElementsAre("resource_cancel_test.html", "dom_tree_test.css",
                    "test.jpg", "iframe.html", "iframe2.html", "Ahem.ttf"));
    FinishAsynchronousTest();
  }

  void UrlRequestFailed(net::URLRequest* request,
                        int net_error,
                        DevToolsStatus devtools_status) override {
    base::AutoLock lock(lock_);
    urls_that_failed_to_load_.push_back(request->url().ExtractFileName());
    EXPECT_TRUE(devtools_status != DevToolsStatus::kNotCanceled);
  }

  virtual network::ErrorReason GetErrorReason() = 0;

 private:
  base::Lock lock_;
  std::vector<std::string> urls_seen_;
  std::vector<std::string> urls_that_failed_to_load_;
};

class UrlRequestFailedTest_Failed : public UrlRequestFailedTest {
 public:
  network::ErrorReason GetErrorReason() override {
    return network::ErrorReason::FAILED;
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(UrlRequestFailedTest_Failed);

class UrlRequestFailedTest_Abort : public UrlRequestFailedTest {
 public:
  network::ErrorReason GetErrorReason() override {
    return network::ErrorReason::ABORTED;
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(UrlRequestFailedTest_Abort);

// A test for recording failed URL loading events, much like
// UrlRequestFailedTest{_Failed,Abort} above, but without overriding
// HeadlessBrowserContext::Observer. Instead, this feature uses
// network observation and works exactly and only for
// network::ErrorReason::BLOCKED_BY_CLIENT modifications that are initiated
// via network::ExperimentalObserver.
class BlockedByClient_NetworkObserver_Test
    : public HeadlessAsyncDevTooledBrowserTest,
      public network::ExperimentalObserver,
      public page::Observer {
 public:
  void RunDevTooledTest() override {
    ASSERT_TRUE(embedded_test_server()->Start());

    // Intercept all network requests.
    devtools_client_->GetNetwork()->GetExperimental()->AddObserver(this);
    devtools_client_->GetNetwork()->Enable();
    std::vector<std::unique_ptr<network::RequestPattern>> patterns;
    patterns.emplace_back(
        network::RequestPattern::Builder().SetUrlPattern("*").Build());
    devtools_client_->GetNetwork()->GetExperimental()->SetRequestInterception(
        network::SetRequestInterceptionParams::Builder()
            .SetPatterns(std::move(patterns))
            .Build());

    // For observing OnLoadEventFired.
    devtools_client_->GetPage()->AddObserver(this);
    devtools_client_->GetPage()->Enable();

    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/resource_cancel_test.html").spec());
  }

  // Overrides network::ExperimentalObserver.
  void OnRequestIntercepted(
      const network::RequestInterceptedParams& params) override {
    urls_seen_.push_back(GURL(params.GetRequest()->GetUrl()).ExtractFileName());

    auto continue_intercept_params =
        network::ContinueInterceptedRequestParams::Builder()
            .SetInterceptionId(params.GetInterceptionId())
            .Build();

    // We *abort* fetching Ahem.ttf, and *fail* for test.jpg
    // to verify that both ways result in a failed loading event,
    // which we'll observe in OnLoadingFailed below.
    // Also, we abort iframe2.html because it turns out frame interception
    // uses a very different codepath than other resources.
    if (EndsWith(params.GetRequest()->GetUrl(), "/test.jpg",
                 base::CompareCase::SENSITIVE)) {
      continue_intercept_params->SetErrorReason(
          network::ErrorReason::BLOCKED_BY_CLIENT);
    } else if (EndsWith(params.GetRequest()->GetUrl(), "/Ahem.ttf",
                        base::CompareCase::SENSITIVE)) {
      continue_intercept_params->SetErrorReason(
          network::ErrorReason::BLOCKED_BY_CLIENT);
    } else if (EndsWith(params.GetRequest()->GetUrl(), "/iframe2.html",
                        base::CompareCase::SENSITIVE)) {
      continue_intercept_params->SetErrorReason(
          network::ErrorReason::BLOCKED_BY_CLIENT);
    }

    devtools_client_->GetNetwork()
        ->GetExperimental()
        ->ContinueInterceptedRequest(std::move(continue_intercept_params));
  }

  // Overrides network::ExperimentalObserver.
  void OnRequestWillBeSent(
      const network::RequestWillBeSentParams& params) override {
    // Here, we just record the URLs (filenames) for each request ID, since
    // we won't have access to them in ::OnLoadingFailed below.
    urls_by_id_[params.GetRequestId()] =
        GURL(params.GetRequest()->GetUrl()).ExtractFileName();
  }

  // Overrides network::ExperimentalObserver.
  void OnLoadingFailed(const network::LoadingFailedParams& params) override {
    // Record the failed loading events so we can verify below that we
    // received the events.
    urls_that_failed_to_load_.push_back(urls_by_id_[params.GetRequestId()]);
    EXPECT_EQ(network::BlockedReason::INSPECTOR, params.GetBlockedReason());
  }

  // Overrides page::ExperimentalObserver.
  void OnLoadEventFired(const page::LoadEventFiredParams&) override {
    EXPECT_THAT(urls_that_failed_to_load_,
                UnorderedElementsAre("test.jpg", "Ahem.ttf", "iframe2.html"));
    EXPECT_THAT(urls_seen_, UnorderedElementsAre("resource_cancel_test.html",
                                                 "dom_tree_test.css",
                                                 "test.jpg", "iframe.html",
                                                 "iframe2.html", "Ahem.ttf"));
    FinishAsynchronousTest();
  }

 private:
  std::vector<std::string> urls_seen_;
  std::vector<std::string> urls_that_failed_to_load_;
  std::map<std::string, std::string> urls_by_id_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(BlockedByClient_NetworkObserver_Test);

class DevToolsSetCookieTest : public HeadlessAsyncDevTooledBrowserTest,
                              public network::Observer {
 public:
  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    devtools_client_->GetNetwork()->AddObserver(this);

    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetNetwork()->Enable(run_loop.QuitClosure());
    run_loop.Run();

    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/set-cookie?cookie1").spec());
  }

  void OnResponseReceived(
      const network::ResponseReceivedParams& params) override {
    EXPECT_NE(std::string::npos, params.GetResponse()->GetHeadersText().find(
                                     "Set-Cookie: cookie1"));
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(DevToolsSetCookieTest);

class DevtoolsInterceptionWithAuthProxyTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public network::ExperimentalObserver,
      public page::Observer {
 public:
  DevtoolsInterceptionWithAuthProxyTest()
      : proxy_server_(net::SpawnedTestServer::TYPE_BASIC_AUTH_PROXY,
                      base::FilePath(FILE_PATH_LITERAL("headless/test/data"))) {
  }

  void SetUp() override {
    ASSERT_TRUE(proxy_server_.Start());
    HeadlessAsyncDevTooledBrowserTest::SetUp();
  }

  void RunDevTooledTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    devtools_client_->GetNetwork()->GetExperimental()->AddObserver(this);
    devtools_client_->GetNetwork()->Enable();
    std::unique_ptr<headless::network::RequestPattern> match_all =
        headless::network::RequestPattern::Builder().SetUrlPattern("*").Build();
    std::vector<std::unique_ptr<headless::network::RequestPattern>> patterns;
    patterns.push_back(std::move(match_all));
    devtools_client_->GetNetwork()->GetExperimental()->SetRequestInterception(
        network::SetRequestInterceptionParams::Builder()
            .SetPatterns(std::move(patterns))
            .Build());

    devtools_client_->GetPage()->AddObserver(this);

    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->Enable(run_loop.QuitClosure());
    run_loop.Run();

    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/dom_tree_test.html").spec());
  }

  void OnRequestIntercepted(
      const network::RequestInterceptedParams& params) override {
    if (params.HasAuthChallenge()) {
      auth_challenge_seen_ = true;
      devtools_client_->GetNetwork()
          ->GetExperimental()
          ->ContinueInterceptedRequest(
              network::ContinueInterceptedRequestParams::Builder()
                  .SetInterceptionId(params.GetInterceptionId())
                  .SetAuthChallengeResponse(
                      network::AuthChallengeResponse::Builder()
                          .SetResponse(network::AuthChallengeResponseResponse::
                                           PROVIDE_CREDENTIALS)
                          .SetUsername("foo")  // These are tested by the proxy.
                          .SetPassword("bar")
                          .Build())
                  .Build());
    } else {
      devtools_client_->GetNetwork()
          ->GetExperimental()
          ->ContinueInterceptedRequest(
              network::ContinueInterceptedRequestParams::Builder()
                  .SetInterceptionId(params.GetInterceptionId())
                  .Build());
      GURL url(params.GetRequest()->GetUrl());
      files_loaded_.insert(url.path());
    }
  }

  void OnLoadEventFired(const page::LoadEventFiredParams&) override {
    EXPECT_TRUE(auth_challenge_seen_);
    EXPECT_THAT(files_loaded_,
                ElementsAre("/Ahem.ttf", "/dom_tree_test.css",
                            "/dom_tree_test.html", "/iframe.html"));
    FinishAsynchronousTest();
  }

  void CustomizeHeadlessBrowserContext(
      HeadlessBrowserContext::Builder& builder) override {
    std::unique_ptr<net::ProxyConfig> proxy_config(new net::ProxyConfig);
    proxy_config->proxy_rules().ParseFromString(
        proxy_server_.host_port_pair().ToString());
    builder.SetProxyConfig(std::move(proxy_config));
  }

 private:
  net::SpawnedTestServer proxy_server_;
  bool auth_challenge_seen_ = false;
  std::set<std::string> files_loaded_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(DevtoolsInterceptionWithAuthProxyTest);

class NavigatorLanguages : public HeadlessAsyncDevTooledBrowserTest {
 public:
  void RunDevTooledTest() override {
    devtools_client_->GetRuntime()->Evaluate(
        "JSON.stringify(navigator.languages)",
        base::BindOnce(&NavigatorLanguages::OnResult, base::Unretained(this)));
  }

  void OnResult(std::unique_ptr<runtime::EvaluateResult> result) {
    EXPECT_TRUE(result->GetResult()->HasValue());
    EXPECT_EQ("[\"en-UK\",\"DE\",\"FR\"]",
              result->GetResult()->GetValue()->GetString());
    FinishAsynchronousTest();
  }

  void CustomizeHeadlessBrowserContext(
      HeadlessBrowserContext::Builder& builder) override {
    builder.SetAcceptLanguage("en-UK, DE, FR");
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(NavigatorLanguages);

}  // namespace headless
