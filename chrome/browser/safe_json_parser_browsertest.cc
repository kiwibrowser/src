// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/test_service_manager_listener.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"
#include "services/data_decoder/public/mojom/constants.mojom.h"
#include "services/data_decoder/public/mojom/json_parser.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/service_manager.mojom.h"

namespace {

using data_decoder::SafeJsonParser;

std::string MaybeToJson(const base::Value* value) {
  if (!value)
    return "(null)";

  std::string json;
  if (!base::JSONWriter::Write(*value, &json))
    return "(invalid value)";

  return json;
}

class ParseCallback {
 public:
  explicit ParseCallback(base::Closure callback) : callback_(callback) {}

  void OnSuccess(std::unique_ptr<base::Value> value) {
    success_ = true;
    callback_.Run();
  }

  void OnError(const std::string& error) {
    success_ = false;
    callback_.Run();
  }

  bool success() const { return success_; }

 private:
  bool success_ = false;
  base::Closure callback_;

  DISALLOW_COPY_AND_ASSIGN(ParseCallback);
};

class SafeJsonParserTest : public InProcessBrowserTest {
 protected:
  void TestParse(const std::string& json) {
    SCOPED_TRACE(json);
    DCHECK(!message_loop_runner_);
    message_loop_runner_ = new content::MessageLoopRunner;

    std::string error;
    std::unique_ptr<base::Value> value = base::JSONReader::ReadAndReturnError(
        json, base::JSON_PARSE_RFC, nullptr, &error);

    SafeJsonParser::SuccessCallback success_callback;
    SafeJsonParser::ErrorCallback error_callback;
    if (value) {
      success_callback =
          base::Bind(&SafeJsonParserTest::ExpectValue, base::Unretained(this),
                     base::Passed(&value));
      error_callback = base::Bind(&SafeJsonParserTest::FailWithError,
                                  base::Unretained(this));
    } else {
      success_callback = base::Bind(&SafeJsonParserTest::FailWithValue,
                                    base::Unretained(this));
      error_callback = base::Bind(&SafeJsonParserTest::ExpectError,
                                  base::Unretained(this), error);
    }
    SafeJsonParser::Parse(
        content::ServiceManagerConnection::GetForProcess()->GetConnector(),
        json, success_callback, error_callback);

    message_loop_runner_->Run();
    message_loop_runner_ = nullptr;
  }

 private:
  void ExpectValue(std::unique_ptr<base::Value> expected_value,
                   std::unique_ptr<base::Value> actual_value) {
    EXPECT_EQ(*expected_value, *actual_value)
        << "Expected: " << MaybeToJson(expected_value.get())
        << " Actual: " << MaybeToJson(actual_value.get());
    message_loop_runner_->Quit();
  }

  void ExpectError(const std::string& expected_error,
                   const std::string& actual_error) {
    EXPECT_EQ(expected_error, actual_error);
    message_loop_runner_->Quit();
  }

  void FailWithValue(std::unique_ptr<base::Value> value) {
    ADD_FAILURE() << MaybeToJson(value.get());
    message_loop_runner_->Quit();
  }

  void FailWithError(const std::string& error) {
    ADD_FAILURE() << error;
    message_loop_runner_->Quit();
  }

  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;
};

class SafeJsonParserImplTest : public InProcessBrowserTest {
 public:
  SafeJsonParserImplTest() = default;

 protected:
  // InProcessBrowserTest implementation:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Initialize the TestServiceManagerListener so it starts listening for
    // service activity.
    listener_.Init();

    // The data_decoder service will stop if no connection is bound to it after
    // 5 seconds. We bind a connection to it for the duration of the test so it
    // is guaranteed the service is always running.
    connector()->BindInterface(data_decoder::mojom::kServiceName,
                               &json_parser_ptr_);
    listener_.WaitUntilServiceStarted(data_decoder::mojom::kServiceName);
    EXPECT_EQ(
        1U, listener_.GetServiceStartCount(data_decoder::mojom::kServiceName));
  }

  service_manager::Connector* connector() const {
    return content::ServiceManagerConnection::GetForProcess()->GetConnector();
  }

  uint32_t GetServiceStartCount(const std::string& service_name) const {
    return listener_.GetServiceStartCount(service_name);
  }

 private:
  data_decoder::mojom::JsonParserPtr json_parser_ptr_;
  TestServiceManagerListener listener_;

  DISALLOW_COPY_AND_ASSIGN(SafeJsonParserImplTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(SafeJsonParserTest, Parse) {
  TestParse("{}");
  TestParse("choke");
  TestParse("{\"awesome\": true}");
  TestParse("\"laser\"");
  TestParse("false");
  TestParse("null");
  TestParse("3.14");
  TestParse("[");
  TestParse("\"");
  TestParse(std::string());
  TestParse("☃");
  TestParse("\"☃\"");
  TestParse("\"\\ufdd0\"");
  TestParse("\"\\ufffe\"");
  TestParse("\"\\ud83f\\udffe\"");
}

// Tests that when calling SafeJsonParser::Parse() a new service is started
// every time.
IN_PROC_BROWSER_TEST_F(SafeJsonParserImplTest, Isolation) {
  for (int i = 0; i < 5; i++) {
    base::RunLoop run_loop;
    ParseCallback parse_callback(run_loop.QuitClosure());
    SafeJsonParser::Parse(
        connector(), "[\"awesome\", \"possum\"]",
        base::Bind(&ParseCallback::OnSuccess,
                   base::Unretained(&parse_callback)),
        base::Bind(&ParseCallback::OnError, base::Unretained(&parse_callback)));
    run_loop.Run();
    EXPECT_TRUE(parse_callback.success());
    // 2 + i below because the data_decoder is already running and the index
    // starts at 0.
    EXPECT_EQ(2U + i, GetServiceStartCount(data_decoder::mojom::kServiceName));
  }
}
