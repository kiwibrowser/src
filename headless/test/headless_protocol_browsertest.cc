// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/base64.h"
#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "content/public/test/browser_test.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_browser_context.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_devtools_target.h"
#include "headless/public/headless_web_contents.h"
#include "headless/test/headless_browser_test.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/url_util.h"

namespace headless {

namespace {
static const char kResetResults[] = "reset-results";
}  // namespace

class HeadlessProtocolBrowserTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public HeadlessDevToolsClient::RawProtocolListener,
      public runtime::Observer {
 public:
  HeadlessProtocolBrowserTest() {
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "third_party/WebKit/LayoutTests/http/tests/inspector-protocol");
    EXPECT_TRUE(embedded_test_server()->Start());
  }

 private:
  // HeadlessWebContentsObserver implementation.
  void DevToolsTargetReady() override {
    HeadlessAsyncDevTooledBrowserTest::DevToolsTargetReady();
    devtools_client_->GetRuntime()->AddObserver(this);
    devtools_client_->GetRuntime()->Enable();
    browser_devtools_client_->SetRawProtocolListener(this);
  }

  std::string EncodeQuery(const std::string& query) {
    url::RawCanonOutputT<char> buffer;
    url::EncodeURIComponent(query.data(), query.size(), &buffer);
    return std::string(buffer.data(), buffer.length());
  }

  void RunDevTooledTest() override {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath src_dir;
    CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &src_dir));
    static const base::FilePath kTestsDirectory(
        FILE_PATH_LITERAL("headless/test/data/protocol"));
    base::FilePath test_path =
        src_dir.Append(kTestsDirectory).AppendASCII(script_name_);
    std::string script;
    if (!base::ReadFileToString(test_path, &script)) {
      ADD_FAILURE() << "Unable to read test at " << test_path;
      FinishTest();
      return;
    }
    GURL test_url = embedded_test_server()->GetURL("/protocol/" + script_name_);
    GURL page_url = embedded_test_server()->GetURL(
        "harness.test", "/protocol/inspector-protocol-test.html?test=" +
                            test_url.spec() + "&script=" + EncodeQuery(script));
    devtools_client_->GetPage()->Navigate(page_url.spec());
  }

  // runtime::Observer implementation.
  void OnConsoleAPICalled(
      const runtime::ConsoleAPICalledParams& params) override {
    const std::vector<std::unique_ptr<runtime::RemoteObject>>& args =
        *params.GetArgs();
    if (args.empty())
      return;
    if (params.GetType() != runtime::ConsoleAPICalledType::DEBUG)
      return;

    runtime::RemoteObject* object = args[0].get();
    if (object->GetType() != runtime::RemoteObjectType::STRING)
      return;

    DispatchMessageFromJS(object->GetValue()->GetString());
  }

  void DispatchMessageFromJS(const std::string& json_message) {
    std::unique_ptr<base::Value> message = base::JSONReader::Read(json_message);
    const base::DictionaryValue* message_dict;
    const base::DictionaryValue* params_dict;
    std::string method;
    int id;
    if (!message || !message->GetAsDictionary(&message_dict) ||
        !message_dict->GetString("method", &method) ||
        !message_dict->GetDictionary("params", &params_dict) ||
        !message_dict->GetInteger("id", &id)) {
      LOG(ERROR) << "Poorly formed message " << json_message;
      FinishTest();
      return;
    }

    if (method != "DONE") {
      // Pass unhandled commands onto the inspector.
      browser_devtools_client_->SendRawDevToolsMessage(json_message);
      return;
    }

    std::string test_result;
    message_dict->GetString("result", &test_result);
    static const base::FilePath kTestsDirectory(
        FILE_PATH_LITERAL("headless/test/data/protocol"));

    base::ScopedAllowBlockingForTesting allow_blocking;

    base::FilePath src_dir;
    CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &src_dir));
    base::FilePath expectation_path =
        src_dir.Append(kTestsDirectory)
            .AppendASCII(script_name_.substr(0, script_name_.length() - 3) +
                         "-expected.txt");

    if (base::CommandLine::ForCurrentProcess()->HasSwitch(kResetResults)) {
      LOG(INFO) << "Updating expectations at " << expectation_path;
      int result = base::WriteFile(expectation_path, test_result.data(),
                                   static_cast<int>(test_result.size()));
      CHECK(test_result.size() == static_cast<size_t>(result));
    }

    std::string expectation;
    if (!base::ReadFileToString(expectation_path, &expectation)) {
      ADD_FAILURE() << "Unable to read expectations at " << expectation_path;
    }
    EXPECT_EQ(test_result, expectation);
    FinishTest();
  }

  // HeadlessDevToolsClient::RawProtocolListener
  bool OnProtocolMessage(const std::string& devtools_agent_host_id,
                         const std::string& json_message,
                         const base::DictionaryValue& parsed_message) override {
    SendMessageToJS(json_message);
    return true;
  }

  void SendMessageToJS(const std::string& message) {
    if (test_finished_)
      return;
    std::string encoded;
    base::Base64Encode(message, &encoded);
    devtools_client_->GetRuntime()->Evaluate("onmessage(atob(\"" + encoded +
                                             "\"))");
  }

  void FinishTest() {
    test_finished_ = true;
    FinishAsynchronousTest();
  }

  // HeadlessBrowserTest overrides.
  void CustomizeHeadlessBrowserContext(
      HeadlessBrowserContext::Builder& builder) override {
    // Make sure the navigations spawn new processes. We run test harness
    // in one process (harness.test) and tests in another.
    builder.SetSitePerProcess(true);
    builder.SetHostResolverRules("MAP *.test 127.0.0.1");
  }

 protected:
  bool test_finished_ = false;
  std::string test_folder_;
  std::string script_name_;
};

#define HEADLESS_PROTOCOL_TEST(TEST_NAME, SCRIPT_NAME)             \
  IN_PROC_BROWSER_TEST_F(HeadlessProtocolBrowserTest, TEST_NAME) { \
    test_folder_ = "/protocol/";                                   \
    script_name_ = SCRIPT_NAME;                                    \
    RunTest();                                                     \
  }

#define LAYOUT_PROTOCOL_TEST(TEST_NAME, SCRIPT_NAME)               \
  IN_PROC_BROWSER_TEST_F(HeadlessProtocolBrowserTest, TEST_NAME) { \
    test_folder_ = "/";                                            \
    script_name_ = SCRIPT_NAME;                                    \
    RunTest();                                                     \
  }

// Headless-specific tests
HEADLESS_PROTOCOL_TEST(VirtualTimeAdvance, "emulation/virtual-time-advance.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeBasics, "emulation/virtual-time-basics.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeCrossProcessNavigation,
                       "emulation/virtual-time-cross-process-navigation.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeDetachFrame,
                       "emulation/virtual-time-detach-frame.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeErrorLoop,
                       "emulation/virtual-time-error-loop.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeHistoryNavigation,
                       "emulation/virtual-time-history-navigation.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeNoBlock404, "emulation/virtual-time-404.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeLocalStorage,
                       "emulation/virtual-time-local-storage.js");
HEADLESS_PROTOCOL_TEST(VirtualTimePendingScript,
                       "emulation/virtual-time-pending-script.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeRedirect,
                       "emulation/virtual-time-redirect.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeSessionStorage,
                       "emulation/virtual-time-session-storage.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeStarvation,
                       "emulation/virtual-time-starvation.js");
HEADLESS_PROTOCOL_TEST(VirtualTimeVideo, "emulation/virtual-time-video.js");

// http://crbug.com/633321
#if defined(OS_ANDROID)
#define MAYBE_VirtualTimeTimerOrder DISABLED_VirtualTimeTimerOrder
#define MAYBE_VirtualTimeTimerSuspend DISABLED_VirtualTimeTimerSuspend
#else
#define MAYBE_VirtualTimeTimerOrder VirtualTimeTimerOrder
#define MAYBE_VirtualTimeTimerSuspend VirtualTimeTimerSuspend
#endif
HEADLESS_PROTOCOL_TEST(MAYBE_VirtualTimeTimerOrder,
                       "emulation/virtual-time-timer-order.js");
HEADLESS_PROTOCOL_TEST(MAYBE_VirtualTimeTimerSuspend,
                       "emulation/virtual-time-timer-suspended.js");
#undef MAYBE_VirtualTimeTimerOrder
#undef MAYBE_VirtualTimeTimerSuspend

}  // namespace headless
