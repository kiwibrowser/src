// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <utility>

#include "base/files/file_path.h"
#include "base/values.h"
#include "extensions/browser/api/web_request/upload_data_presenter.h"
#include "extensions/browser/api/web_request/web_request_api_constants.h"
#include "net/base/upload_bytes_element_reader.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace keys = extension_web_request_api_constants;

namespace extensions {

// This only tests the handling of dots in keys. Other functionality is covered
// by ExtensionWebRequestTest.AccessRequestBodyData and
// WebRequestFormDataParserTest.
TEST(WebRequestUploadDataPresenterTest, ParsedData) {
  // Input.
  const char block[] = "key.with.dots=value";
  net::UploadBytesElementReader element(block, sizeof(block) - 1);

  // Expected output.
  std::unique_ptr<base::ListValue> values(new base::ListValue);
  values->AppendString("value");
  base::DictionaryValue expected_form;
  expected_form.SetWithoutPathExpansion("key.with.dots", std::move(values));

  // Real output.
  std::unique_ptr<ParsedDataPresenter> parsed_data_presenter(
      ParsedDataPresenter::CreateForTests());
  ASSERT_TRUE(parsed_data_presenter.get() != NULL);
  parsed_data_presenter->FeedBytes(
      base::StringPiece(element.bytes(), element.length()));
  EXPECT_TRUE(parsed_data_presenter->Succeeded());
  std::unique_ptr<base::Value> result = parsed_data_presenter->Result();
  ASSERT_TRUE(result.get() != NULL);

  EXPECT_TRUE(result->Equals(&expected_form));
}

TEST(WebRequestUploadDataPresenterTest, RawData) {
  // Input.
  const char block1[] = "test";
  const size_t block1_size = sizeof(block1) - 1;
  const char kFilename[] = "path/test_filename.ext";
  const char block2[] = "another test";
  const size_t block2_size = sizeof(block2) - 1;

  // Expected output.
  std::unique_ptr<base::Value> expected_a(
      base::Value::CreateWithCopiedBuffer(block1, block1_size));
  ASSERT_TRUE(expected_a.get() != NULL);
  std::unique_ptr<base::Value> expected_b(new base::Value(kFilename));
  ASSERT_TRUE(expected_b.get() != NULL);
  std::unique_ptr<base::Value> expected_c(
      base::Value::CreateWithCopiedBuffer(block2, block2_size));
  ASSERT_TRUE(expected_c.get() != NULL);

  base::ListValue expected_list;
  subtle::AppendKeyValuePair(keys::kRequestBodyRawBytesKey,
                             std::move(expected_a), &expected_list);
  subtle::AppendKeyValuePair(keys::kRequestBodyRawFileKey,
                             std::move(expected_b), &expected_list);
  subtle::AppendKeyValuePair(keys::kRequestBodyRawBytesKey,
                             std::move(expected_c), &expected_list);

  // Real output.
  RawDataPresenter raw_presenter;
  raw_presenter.FeedNextBytes(block1, block1_size);
  raw_presenter.FeedNextFile(kFilename);
  raw_presenter.FeedNextBytes(block2, block2_size);
  EXPECT_TRUE(raw_presenter.Succeeded());
  std::unique_ptr<base::Value> result = raw_presenter.Result();
  ASSERT_TRUE(result.get() != NULL);

  EXPECT_TRUE(result->Equals(&expected_list));
}

TEST(WebRequestUploadDataPresenterTest, ParsedDataSegmented) {
  // Input.
  static constexpr char block1[] = "v1=FOO";
  static constexpr char block2[] = "BAR&v2=BAZ";
  static constexpr size_t block1_size = sizeof(block1) - 1;
  static constexpr size_t block2_size = sizeof(block2) - 1;

  // Expected output.
  auto v1 = std::make_unique<base::ListValue>();
  v1->AppendString("FOOBAR");
  auto v2 = std::make_unique<base::ListValue>();
  v2->AppendString("BAZ");

  base::DictionaryValue expected_form;
  expected_form.SetWithoutPathExpansion("v1", std::move(v1));
  expected_form.SetWithoutPathExpansion("v2", std::move(v2));

  {
    // Consecutive data segments should be consolidated and parsed successfuly.
    auto parsed_data_presenter = ParsedDataPresenter::CreateForTests();
    ASSERT_TRUE(parsed_data_presenter.get());
    parsed_data_presenter->FeedBytes(base::StringPiece(block1, block1_size));
    parsed_data_presenter->FeedBytes(base::StringPiece(block2, block2_size));

    EXPECT_TRUE(parsed_data_presenter->Succeeded());
    auto result = parsed_data_presenter->Result();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, expected_form);
  }

  {
    // Data segments separate by file inputs should not be consolidated.
    auto parsed_data_presenter = ParsedDataPresenter::CreateForTests();
    ASSERT_TRUE(parsed_data_presenter.get());
    parsed_data_presenter->FeedBytes(base::StringPiece(block1, block1_size));
    parsed_data_presenter->FeedFile(base::FilePath());
    parsed_data_presenter->FeedBytes(base::StringPiece(block2, block2_size));

    EXPECT_FALSE(parsed_data_presenter->Succeeded());
  }
}

}  // namespace extensions
