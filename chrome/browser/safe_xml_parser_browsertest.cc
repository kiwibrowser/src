// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/values.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/test_service_manager_listener.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/test/test_utils.h"
#include "services/data_decoder/public/cpp/safe_xml_parser.h"
#include "services/data_decoder/public/mojom/constants.mojom.h"
#include "services/data_decoder/public/mojom/xml_parser.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

constexpr char kTestXml[] = "<hello>bonjour</hello>";
constexpr char kTestJson[] = R"(
    {"type": "element",
     "tag": "hello",
     "children": [{"type": "text", "text": "bonjour"}]
     } )";

class SafeXmlParserTest : public InProcessBrowserTest {
 public:
  SafeXmlParserTest() = default;
  ~SafeXmlParserTest() override = default;

 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    listener_.Init();
  }

  uint32_t GetServiceStartCount(const std::string& service_name) const {
    return listener_.GetServiceStartCount(service_name);
  }

  // Parses |xml| and compares its parsed representation with |expected_json|.
  // If a |batch_id| is provided, it is passed to the ParseXml call (to group
  // parsing of multiple XML documents in the same utility process).
  // If |expected_json| is empty, the XML parsing is expected to fail.
  void TestParse(base::StringPiece xml,
                 const std::string& expected_json,
                 const std::string& batch_id = std::string()) {
    SCOPED_TRACE(xml);

    base::RunLoop run_loop;
    std::unique_ptr<base::Value> expected_value;
    if (!expected_json.empty()) {
      expected_value = base::JSONReader::Read(expected_json);
      DCHECK(expected_value) << "Bad test, incorrect JSON: " << expected_json;
    }

    data_decoder::ParseXml(
        content::ServiceManagerConnection::GetForProcess()->GetConnector(),
        xml.as_string(),
        base::BindOnce(&SafeXmlParserTest::XmlParsingDone,
                       base::Unretained(this), run_loop.QuitClosure(),
                       std::move(expected_value)),
        batch_id);
    run_loop.Run();
  }

 private:
  void XmlParsingDone(base::Closure quit_loop_closure,
                      std::unique_ptr<base::Value> expected_value,
                      std::unique_ptr<base::Value> actual_value,
                      const base::Optional<std::string>& error) {
    base::ScopedClosureRunner runner(std::move(quit_loop_closure));
    if (!expected_value) {
      EXPECT_FALSE(actual_value);
      EXPECT_TRUE(error);
      return;
    }
    EXPECT_FALSE(error);
    ASSERT_TRUE(actual_value);
    EXPECT_EQ(*expected_value, *actual_value);
  }

  data_decoder::mojom::XmlParserPtr xml_parser_ptr_;
  TestServiceManagerListener listener_;

  DISALLOW_COPY_AND_ASSIGN(SafeXmlParserTest);
};

}  // namespace

// Tests that SafeXmlParser does parse. (actual XML parsing is tested in the
// service unit-tests).
IN_PROC_BROWSER_TEST_F(SafeXmlParserTest, Parse) {
  TestParse("[\"this is JSON not XML\"]", "");
  TestParse(kTestXml, kTestJson);
}

// Tests that a new service is created for each SafeXmlParser::Parse() call.
IN_PROC_BROWSER_TEST_F(SafeXmlParserTest, Isolation) {
  constexpr size_t kParseCount = 5;
  for (size_t i = 0; i < kParseCount; i++)
    TestParse(kTestXml, kTestJson);
  EXPECT_EQ(kParseCount,
            GetServiceStartCount(data_decoder::mojom::kServiceName));
}

// Tests that using a batch ID allows service reuse.
IN_PROC_BROWSER_TEST_F(SafeXmlParserTest, IsolationWithBatchId) {
  constexpr char kBatchId1[] = "batch1";
  constexpr char kBatchId2[] = "batch2";
  for (int i = 0; i < 5; i++) {
    TestParse(kTestXml, kTestJson, kBatchId1);
    TestParse(kTestXml, kTestJson, kBatchId2);
  }
  EXPECT_EQ(2U, GetServiceStartCount(data_decoder::mojom::kServiceName));
}
