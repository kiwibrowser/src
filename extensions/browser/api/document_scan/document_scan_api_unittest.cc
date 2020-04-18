// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/document_scan/document_scan_api.h"

#include <string>
#include <tuple>
#include <vector>

#include "extensions/browser/api/document_scan/mock_document_scan_interface.h"
#include "extensions/browser/api_test_utils.h"
#include "extensions/browser/api_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace extensions {

namespace api {

// Tests of networking_private_crypto support for Networking Private API.
class DocumentScanScanFunctionTest : public ApiUnitTest {
 public:
  DocumentScanScanFunctionTest()
      : function_(new DocumentScanScanFunction()),
        document_scan_interface_(new MockDocumentScanInterface()) {}
  ~DocumentScanScanFunctionTest() override {}

  void SetUp() override {
    ApiUnitTest::SetUp();
    // Passes ownership.
    function_->document_scan_interface_.reset(document_scan_interface_);
  }

 protected:
  std::string RunFunctionAndReturnError(const std::string& args) {
    function_->set_extension(extension());
    std::string error = api_test_utils::RunFunctionAndReturnError(
        function_, args, browser_context(), api_test_utils::NONE);
    return error;
  }

  DocumentScanScanFunction* function_;
  MockDocumentScanInterface* document_scan_interface_;  // Owned by function_.
};

ACTION_P2(InvokeListScannersCallback, scanner_list, error) {
  std::get<0>(args).Run(scanner_list, error);
}

ACTION_P3(InvokeScanCallback, data, mime_type, error) {
  std::get<3>(args).Run(data, mime_type, error);
}

TEST_F(DocumentScanScanFunctionTest, GestureRequired) {
  EXPECT_EQ("User gesture required to perform scan",
            RunFunctionAndReturnError("[{}]"));
}

TEST_F(DocumentScanScanFunctionTest, NoScanners) {
  function_->set_user_gesture(true);
  EXPECT_CALL(*document_scan_interface_, ListScanners(_))
      .WillOnce(InvokeListScannersCallback(
          std::vector<DocumentScanInterface::ScannerDescription>(), ""));
  EXPECT_EQ("Scanner not available", RunFunctionAndReturnError("[{}]"));
}

TEST_F(DocumentScanScanFunctionTest, NoMatchingScanners) {
  function_->set_user_gesture(true);
  std::vector<DocumentScanInterface::ScannerDescription> scanner_list;
  DocumentScanInterface::ScannerDescription scanner;
  scanner.image_mime_type = "img/fresco";
  scanner_list.push_back(scanner);
  EXPECT_CALL(*document_scan_interface_, ListScanners(_))
      .WillOnce(InvokeListScannersCallback(scanner_list, ""));
  EXPECT_EQ(
      "Scanner not available",
      RunFunctionAndReturnError("[{\"mimeTypes\": [\"img/silverpoint\"]}]"));
}

TEST_F(DocumentScanScanFunctionTest, ScanFailure) {
  function_->set_user_gesture(true);
  std::vector<DocumentScanInterface::ScannerDescription> scanner_list;
  DocumentScanInterface::ScannerDescription scanner;
  const char kMimeType[] = "img/tempera";
  const char kScannerName[] = "Michelangelo";
  scanner.name = kScannerName;
  scanner.image_mime_type = kMimeType;
  scanner_list.push_back(scanner);
  EXPECT_CALL(*document_scan_interface_, ListScanners(_))
      .WillOnce(InvokeListScannersCallback(scanner_list, ""));
  const char kScanError[] = "Someone ate all the eggs";
  EXPECT_CALL(*document_scan_interface_, Scan(kScannerName, _, _, _))
      .WillOnce(InvokeScanCallback("", "", kScanError));
  EXPECT_EQ(kScanError,
            RunFunctionAndReturnError("[{\"mimeTypes\": [\"img/tempera\"]}]"));
}

TEST_F(DocumentScanScanFunctionTest, Success) {
  std::vector<DocumentScanInterface::ScannerDescription> scanner_list;
  scanner_list.push_back(DocumentScanInterface::ScannerDescription());
  EXPECT_CALL(*document_scan_interface_, ListScanners(_))
      .WillOnce(InvokeListScannersCallback(scanner_list, ""));
  const char kScanData[] = "A beautiful picture";
  const char kMimeType[] = "img/encaustic";
  EXPECT_CALL(*document_scan_interface_, Scan(_, _, _, _))
      .WillOnce(InvokeScanCallback(kScanData, kMimeType, ""));
  function_->set_user_gesture(true);
  std::unique_ptr<base::DictionaryValue> result(
      RunFunctionAndReturnDictionary(function_, "[{}]"));
  ASSERT_NE(nullptr, result.get());
  document_scan::ScanResults scan_results;
  EXPECT_TRUE(document_scan::ScanResults::Populate(*result, &scan_results));
  EXPECT_THAT(scan_results.data_urls, testing::ElementsAre(kScanData));
  EXPECT_EQ(kMimeType, scan_results.mime_type);
}

}  // namespace api

}  // namespace extensions
