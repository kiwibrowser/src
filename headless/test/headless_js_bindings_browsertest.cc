// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/test/browser_test.h"
#include "headless/grit/headless_browsertest_resources.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_web_contents.h"
#include "headless/public/util/testing/test_in_memory_protocol_handler.h"
#include "headless/test/headless_browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"

namespace headless {

namespace {
const char kIndexHtml[] = R"(
<html>
<body>
<script src="bindings.js"></script>
</body>
</html>
)";

const char kTabSocketScript[] = R"(
window.TabSocket = {};
window.TabSocket.onmessage = () => {};
window.TabSocket.send = (json) => console.debug(json);
)";

}  // namespace

class HeadlessJsBindingsTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public HeadlessDevToolsClient::RawProtocolListener,
      public TestInMemoryProtocolHandler::RequestDeferrer,
      public headless::runtime::Observer,
      public page::ExperimentalObserver {
 public:
  using ConsoleAPICalledParams = headless::runtime::ConsoleAPICalledParams;
  using EvaluateResult = headless::runtime::EvaluateResult;
  using RemoteObject = headless::runtime::RemoteObject;

  HeadlessJsBindingsTest() : weak_factory_(this) {}

  void SetUpOnMainThread() override {
    base::ThreadRestrictions::SetIOAllowed(true);
    base::FilePath pak_path;
    ASSERT_TRUE(base::PathService::Get(base::DIR_MODULE, &pak_path));
    pak_path = pak_path.AppendASCII("headless_browser_tests.pak");
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        pak_path, ui::SCALE_FACTOR_NONE);
  }

  void CustomizeHeadlessBrowserContext(
      HeadlessBrowserContext::Builder& builder) override {
    builder.EnableUnsafeNetworkAccessWithMojoBindings(true);
  }

  void CustomizeHeadlessWebContents(
      HeadlessWebContents::Builder& builder) override {
    builder.SetWindowSize(gfx::Size(0, 0));
    builder.SetInitialURL(GURL("http://test.com/index.html"));

    http_handler_->SetHeadlessBrowserContext(browser_context_);
  }

  ProtocolHandlerMap GetProtocolHandlers() override {
    ProtocolHandlerMap protocol_handlers;
    std::unique_ptr<TestInMemoryProtocolHandler> http_handler(
        new TestInMemoryProtocolHandler(browser()->BrowserIOThread(), this));
    http_handler_ = http_handler.get();
    bindings_js_ = ui::ResourceBundle::GetSharedInstance()
                       .GetRawDataResource(DEVTOOLS_BINDINGS_TEST)
                       .as_string();
    http_handler->InsertResponse("http://test.com/index.html",
                                 {kIndexHtml, "text/html"});
    http_handler->InsertResponse(
        "http://test.com/bindings.js",
        {bindings_js_.c_str(), "application/javascript"});
    protocol_handlers[url::kHttpScheme] = std::move(http_handler);
    return protocol_handlers;
  }

  void RunDevTooledTest() override {
    devtools_client_->GetPage()->GetExperimental()->AddObserver(this);
    devtools_client_->GetPage()->Enable();
    devtools_client_->GetPage()->AddScriptToEvaluateOnNewDocument(
        kTabSocketScript);
    devtools_client_->GetRuntime()->AddObserver(this);
    devtools_client_->GetRuntime()->Enable();
    devtools_client_->GetRuntime()->Evaluate(
        kTabSocketScript,
        base::BindOnce(&HeadlessJsBindingsTest::ConnectionEstablished,
                       weak_factory_.GetWeakPtr()));
    devtools_client_->SetRawProtocolListener(this);
  }

  void OnRequest(const GURL& url,
                 base::RepeatingClosure complete_request) override {
    complete_request.Run();
  }

  void ConnectionEstablished(std::unique_ptr<EvaluateResult>) {
    connection_established_ = true;
    if (complete_request_) {
      browser()->BrowserIOThread()->PostTask(FROM_HERE, complete_request_);
      complete_request_ = base::RepeatingClosure();
    }
  }

  virtual void RunJsBindingsTest() = 0;

  void OnLoadEventFired(const page::LoadEventFiredParams& params) override {
    RunJsBindingsTest();
  }

  virtual void OnResult(const std::string& result) = 0;

  void FailOnJsEvaluateException(
      std::unique_ptr<runtime::EvaluateResult> result) {
    if (!result->HasExceptionDetails())
      return;

    FinishAsynchronousTest();

    const runtime::ExceptionDetails* exception_details =
        result->GetExceptionDetails();
    FAIL() << exception_details->GetText()
           << (exception_details->HasException()
                   ? exception_details->GetException()->GetDescription().c_str()
                   : "");
  }

  void OnConsoleAPICalled(const ConsoleAPICalledParams& params) override {
    const std::vector<std::unique_ptr<RemoteObject>>& args = *params.GetArgs();
    if (args.empty())
      return;
    if (params.GetType() != headless::runtime::ConsoleAPICalledType::DEBUG)
      return;

    RemoteObject* object = args[0].get();
    if (object->GetType() != headless::runtime::RemoteObjectType::STRING)
      return;

    OnMessageFromJS(object->GetValue()->GetString());
  }

  void SendMessageToJS(const std::string& message) {
    std::string encoded;
    base::Base64Encode(message, &encoded);
    devtools_client_->GetRuntime()->Evaluate(
        "window.TabSocket.onmessage(atob(\"" + encoded + "\"))");
  }

  void OnMessageFromJS(const std::string& json_message) {
    std::unique_ptr<base::Value> message =
        base::JSONReader::Read(json_message, base::JSON_PARSE_RFC);
    const base::Value* method_value = message->FindKey("method");
    if (!method_value) {
      FinishAsynchronousTest();
      FAIL() << "Badly formed message " << json_message;
      return;
    }

    const base::Value* params_value = message->FindKey("params");
    if (!params_value) {
      FinishAsynchronousTest();
      FAIL() << "Badly formed message " << json_message;
      return;
    }

    const base::Value* id_value = message->FindKey("id");
    if (!id_value) {
      FinishAsynchronousTest();
      FAIL() << "Badly formed message " << json_message;
      return;
    }

    if (method_value->GetString() == "__Result") {
      OnResult(params_value->FindKey("result")->GetString());
      return;
    }

    devtools_client_->SendRawDevToolsMessage(json_message);
  }

  bool OnProtocolMessage(const std::string& devtools_agent_host_id,
                         const std::string& json_message,
                         const base::DictionaryValue& parsed_message) override {
    if (!connection_established_)
      return false;

    const base::Value* id_value = parsed_message.FindKey("id");
    // If |parsed_message| contains an id we know this is a message reply.
    if (id_value) {
      int id = id_value->GetInt();
      // We are only interested in message replies (ones with an id) where the
      // id is odd. The reason is HeadlessDevToolsClientImpl uses even/oddness
      // to distinguish between commands send from the C++ bindings and those
      // via HeadlessDevToolsClientImpl::SendRawDevToolsMessage.
      if ((id % 2) == 0)
        return false;

      SendMessageToJS(json_message);
      return true;
    }

    const base::Value* method_value = parsed_message.FindKey("method");
    if (!method_value)
      return false;

    if (method_value->GetString() == "Runtime.consoleAPICalled") {
      // console.debug is used for transport.
      return false;
    }

    SendMessageToJS(json_message);

    // Check which domain the event belongs to, if it's the DOM domain then
    // assume js handled it.
    std::vector<base::StringPiece> sections =
        SplitStringPiece(method_value->GetString(), ".", base::KEEP_WHITESPACE,
                         base::SPLIT_WANT_ALL);

    return sections[0] == "DOM" || sections[0] == "Runtime";
  }

 protected:
  TestInMemoryProtocolHandler* http_handler_;  // NOT OWNED
  std::string bindings_js_;
  base::RepeatingClosure complete_request_;
  bool connection_established_ = false;
  base::WeakPtrFactory<HeadlessJsBindingsTest> weak_factory_;
};

class SimpleCommandJsBindingsTest : public HeadlessJsBindingsTest {
 public:
  void RunJsBindingsTest() override {
    devtools_client_->GetRuntime()->Evaluate(
        "new chromium.BindingsTest().evalOneAddOne();",
        base::BindOnce(&HeadlessJsBindingsTest::FailOnJsEvaluateException,
                       base::Unretained(this)));
  }

  void OnResult(const std::string& result) override {
    EXPECT_EQ("2", result);
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(SimpleCommandJsBindingsTest);

class ExperimentalCommandJsBindingsTest : public HeadlessJsBindingsTest {
 public:
  void RunJsBindingsTest() override {
    devtools_client_->GetRuntime()->Evaluate(
        "new chromium.BindingsTest().getIsolatedWorldName();",
        base::BindOnce(&HeadlessJsBindingsTest::FailOnJsEvaluateException,
                       base::Unretained(this)));
  }

  void OnResult(const std::string& result) override {
    EXPECT_EQ("Created Test Isolated World", result);
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(ExperimentalCommandJsBindingsTest);

class SimpleEventJsBindingsTest : public HeadlessJsBindingsTest {
 public:
  void RunJsBindingsTest() override {
    devtools_client_->GetRuntime()->Evaluate(
        "new chromium.BindingsTest().listenForChildNodeCountUpdated();",
        base::BindOnce(&HeadlessJsBindingsTest::FailOnJsEvaluateException,
                       base::Unretained(this)));
  }

  void OnResult(const std::string& result) override {
    EXPECT_EQ("{\"nodeId\":4,\"childNodeCount\":2}", result);
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(SimpleEventJsBindingsTest);

/*
 * Like SimpleCommandJsBindingsTest except it's run twice. On the first run
 * metadata is produced by v8 for http://test.com/bindings.js. On the second run
 * the metadata is used used leading to substantially faster execution time.
 */
class CachedJsBindingsTest : public HeadlessJsBindingsTest,
                             public HeadlessBrowserContext::Observer {
 public:
  void CustomizeHeadlessBrowserContext(
      HeadlessBrowserContext::Builder& builder) override {
    builder.SetCaptureResourceMetadata(true);
    builder.SetOverrideWebPreferencesCallback(base::BindRepeating(
        &CachedJsBindingsTest::OverrideWebPreferences, base::Unretained(this)));
    HeadlessJsBindingsTest::CustomizeHeadlessBrowserContext(builder);
  }

  void OverrideWebPreferences(WebPreferences* preferences) {
    // Request eager code compilation.
    preferences->v8_cache_options =
        content::V8_CACHE_OPTIONS_FULLCODE_WITHOUT_HEAT_CHECK;
  }

  void RunDevTooledTest() override {
    browser_context_->AddObserver(this);
    HeadlessJsBindingsTest::RunDevTooledTest();
  }

  void RunJsBindingsTest() override {
    devtools_client_->GetRuntime()->Evaluate(
        "new chromium.BindingsTest().evalOneAddOne();",
        base::BindRepeating(&HeadlessJsBindingsTest::FailOnJsEvaluateException,
                            base::Unretained(this)));
  }

  void OnResult(const std::string& result) override {
    EXPECT_EQ("2", result);

    if (first_result) {
      devtools_client_->GetPage()->Reload();
    } else {
      EXPECT_TRUE(metadata_received_);
      FinishAsynchronousTest();
    }
    first_result = false;
  }

  void OnMetadataForResource(const GURL& url,
                             net::IOBuffer* buf,
                             int buf_len) override {
    ASSERT_FALSE(metadata_received_);
    metadata_received_ = true;

    scoped_refptr<net::IOBufferWithSize> metadata(
        new net::IOBufferWithSize(buf_len));
    memcpy(metadata->data(), buf->data(), buf_len);
    http_handler_->SetResponseMetadata(url.spec(), metadata);
  }

  bool metadata_received_ = false;
  bool first_result = true;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(CachedJsBindingsTest);

}  // namespace headless
