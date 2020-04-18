// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "content/public/test/browser_test.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/lib/browser/headless_web_contents_impl.h"
#include "headless/public/devtools/domains/network.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/util/testing/test_in_memory_protocol_handler.h"
#include "headless/test/headless_browser_test.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_job_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::ContainerEq;

namespace headless {

namespace {
const char* kIndexHtml = R"(
<html>
<head><link rel="stylesheet" type="text/css" href="style1.css"></head>
<body>
<div class="red">Main frame</div>
<iframe src="iframe1.html"></iframe>
<iframe src="iframe2.html"></iframe>
</body>
</html>)";

const char* kIFrame1 = R"(
<html>
<head><link rel="stylesheet" type="text/css" href="style2.css"></head>
<body>
<div class="green">IFrame 1</div>
</body>
</html>)";

const char* kIFrame2 = R"(
<html>
<head><link rel="stylesheet" type="text/css" href="style3.css"></head>
<body>
<div class="blue">IFrame 1</div>
</body>
</html>)";

const char* kStyle1Css = R"(
.red {
  color: #f00
} )";

const char* kStyle2Css = R"(
.green {
  color: #0f0
} )";

const char* kStyle3Css = R"(
.blue {
  color: #00f
} )";

}  // namespace

class FrameIdTest : public HeadlessAsyncDevTooledBrowserTest,
                    public network::ExperimentalObserver,
                    public page::Observer,
                    public runtime::Observer {
 public:
  void RunDevTooledTest() override {
    http_handler_->SetHeadlessBrowserContext(browser_context_);

    EXPECT_TRUE(embedded_test_server()->Start());
    devtools_client_->GetNetwork()->GetExperimental()->AddObserver(this);
    devtools_client_->GetNetwork()->Enable();
    devtools_client_->GetRuntime()->GetExperimental()->AddObserver(this);
    devtools_client_->GetRuntime()->Enable();

    // Enabling the runtime domain will send us the current context.
    run_loop_ = std::make_unique<base::RunLoop>(
        base::RunLoop::Type::kNestableTasksAllowed);
    run_loop_->Run();
    run_loop_ = nullptr;

    EXPECT_EQ(1u, execution_context_frame_ids_.size());
    execution_context_frame_ids_.clear();

    if (EnableInterception()) {
      std::unique_ptr<headless::network::RequestPattern> match_all =
          headless::network::RequestPattern::Builder()
              .SetUrlPattern("*")
              .Build();
      std::vector<std::unique_ptr<headless::network::RequestPattern>> patterns;
      patterns.push_back(std::move(match_all));
      devtools_client_->GetNetwork()->GetExperimental()->SetRequestInterception(
          network::SetRequestInterceptionParams::Builder()
              .SetPatterns(std::move(patterns))
              .Build());
    }

    devtools_client_->GetPage()->AddObserver(this);

    run_loop_ = std::make_unique<base::RunLoop>(
        base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetPage()->Enable(run_loop_->QuitClosure());
    run_loop_->Run();
    run_loop_ = nullptr;

    devtools_client_->GetPage()->Navigate("http://foo.com/index.html");
  }

  ProtocolHandlerMap GetProtocolHandlers() override {
    ProtocolHandlerMap protocol_handlers;
    std::unique_ptr<TestInMemoryProtocolHandler> http_handler(
        new TestInMemoryProtocolHandler(browser()->BrowserIOThread(),
                                        /* request_deferrer */ nullptr));
    http_handler_ = http_handler.get();
    http_handler_->InsertResponse("http://foo.com/index.html",
                                  {kIndexHtml, "text/html"});
    http_handler_->InsertResponse("http://foo.com/iframe1.html",
                                  {kIFrame1, "text/html"});
    http_handler_->InsertResponse("http://foo.com/iframe2.html",
                                  {kIFrame2, "text/html"});
    http_handler_->InsertResponse("http://foo.com/style1.css",
                                  {kStyle1Css, "text/css"});
    http_handler_->InsertResponse("http://foo.com/style2.css",
                                  {kStyle2Css, "text/css"});
    http_handler_->InsertResponse("http://foo.com/style3.css",
                                  {kStyle3Css, "text/css"});
    protocol_handlers[url::kHttpScheme] = std::move(http_handler);
    return protocol_handlers;
  }

  // network::Observer implementation:
  void OnRequestWillBeSent(
      const network::RequestWillBeSentParams& params) override {
    url_to_frame_id_[params.GetRequest()->GetUrl()] = params.GetFrameId();
    frame_ids_.insert(params.GetFrameId());
  }

  // page::Observer implementation:
  void OnLoadEventFired(const page::LoadEventFiredParams& params) override {
    EXPECT_THAT(url_to_frame_id_,
                ContainerEq(http_handler_->url_to_devtools_frame_id()));
    EXPECT_THAT(execution_context_frame_ids_, ContainerEq(frame_ids_));
    FinishAsynchronousTest();
  }

  virtual bool EnableInterception() const { return false; }

  void OnExecutionContextCreated(
      const runtime::ExecutionContextCreatedParams& params) override {
    const base::Value* frameId =
        params.GetContext()->GetAuxData()->FindKey("frameId");
    if (frameId && frameId->is_string())
      execution_context_frame_ids_.insert(frameId->GetString());

    // If we're nested then exit.
    if (run_loop_)
      run_loop_->Quit();
  }

 private:
  std::set<std::string> frame_ids_;
  std::set<std::string> execution_context_frame_ids_;
  std::map<std::string, std::string> url_to_frame_id_;
  TestInMemoryProtocolHandler* http_handler_;  // NOT OWNED
  std::unique_ptr<base::RunLoop> run_loop_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(FrameIdTest);

// Frame IDs should still be available with network request interception enabled
class FrameIdWithDevtoolsRequestInterceptionTest : public FrameIdTest {
 public:
  void OnRequestIntercepted(
      const network::RequestInterceptedParams& params) override {
    // Allow the request to continue.
    devtools_client_->GetNetwork()
        ->GetExperimental()
        ->ContinueInterceptedRequest(
            network::ContinueInterceptedRequestParams::Builder()
                .SetInterceptionId(params.GetInterceptionId())
                .Build());
  }

  bool EnableInterception() const override { return true; }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(FrameIdWithDevtoolsRequestInterceptionTest);

}  // namespace headless
