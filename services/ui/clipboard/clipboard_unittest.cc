// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/ui/public/interfaces/clipboard.mojom.h"
#include "services/ui/public/interfaces/constants.mojom.h"

using ui::mojom::Clipboard;

namespace ui {
namespace clipboard {
namespace {

const char* kUninitialized = "Uninitialized data";
const char* kPlainTextData = "Some plain data";
const char* kHtmlData = "<html>data</html>";

}  // namespace

class ClipboardAppTest : public service_manager::test::ServiceTest {
 public:
  ClipboardAppTest() : ServiceTest("clipboard_unittests") {}
  ~ClipboardAppTest() override {}

  // Overridden from service_manager::test::ServiceTest:
  void SetUp() override {
    ServiceTest::SetUp();

    connector()->BindInterface(ui::mojom::kServiceName, &clipboard_);
    ASSERT_TRUE(clipboard_);
  }

  uint64_t GetSequenceNumber() {
    uint64_t sequence_num = 999999;
    EXPECT_TRUE(clipboard_->GetSequenceNumber(
        Clipboard::Type::COPY_PASTE, &sequence_num));
    return sequence_num;
  }

  std::vector<std::string> GetAvailableFormatMimeTypes() {
    uint64_t sequence_num = 999999;
    std::vector<std::string> types;
    types.push_back(kUninitialized);
    clipboard_->GetAvailableMimeTypes(
        Clipboard::Type::COPY_PASTE,
        &sequence_num, &types);
    return types;
  }

  bool GetDataOfType(const std::string& mime_type, std::string* data) {
    bool valid = false;
    base::Optional<std::vector<uint8_t>> raw_data;
    uint64_t sequence_number = 0;
    clipboard_->ReadClipboardData(Clipboard::Type::COPY_PASTE, mime_type,
                                  &sequence_number, &raw_data);
    valid = raw_data.has_value();
    if (valid)
      data->assign(raw_data->begin(), raw_data->end());
    else
      data->clear();
    return valid;
  }

  void SetStringText(const std::string& data) {
    uint64_t sequence_number;
    base::flat_map<std::string, std::vector<uint8_t>> mime_data;
    mime_data[mojom::kMimeTypeText] =
        std::vector<uint8_t>(data.begin(), data.end());
    clipboard_->WriteClipboardData(Clipboard::Type::COPY_PASTE,
                                   std::move(mime_data),
                                   &sequence_number);
  }

 protected:
  mojom::ClipboardPtr clipboard_;

  DISALLOW_COPY_AND_ASSIGN(ClipboardAppTest);
};

TEST_F(ClipboardAppTest, EmptyClipboardOK) {
  EXPECT_EQ(0ul, GetSequenceNumber());
  EXPECT_TRUE(GetAvailableFormatMimeTypes().empty());
  std::string data;
  EXPECT_FALSE(GetDataOfType(mojom::kMimeTypeText, &data));
}

TEST_F(ClipboardAppTest, CanReadBackText) {
  std::string data;
  EXPECT_EQ(0ul, GetSequenceNumber());
  EXPECT_FALSE(GetDataOfType(mojom::kMimeTypeText, &data));

  SetStringText(kPlainTextData);
  EXPECT_EQ(1ul, GetSequenceNumber());

  EXPECT_TRUE(GetDataOfType(mojom::kMimeTypeText, &data));
  EXPECT_EQ(kPlainTextData, data);
}

TEST_F(ClipboardAppTest, CanSetMultipleDataTypesAtOnce) {
  base::flat_map<std::string, std::vector<uint8_t>> mime_data;
  mime_data[mojom::kMimeTypeText] = std::vector<uint8_t>(
      kPlainTextData, kPlainTextData + strlen(kPlainTextData));
  mime_data[mojom::kMimeTypeHTML] =
      std::vector<uint8_t>(kHtmlData, kHtmlData + strlen(kHtmlData));

  uint64_t sequence_num = 0;
  clipboard_->WriteClipboardData(Clipboard::Type::COPY_PASTE,
                                 std::move(mime_data),
                                 &sequence_num);
  EXPECT_EQ(1ul, sequence_num);

  std::string data;
  EXPECT_TRUE(GetDataOfType(mojom::kMimeTypeText, &data));
  EXPECT_EQ(kPlainTextData, data);
  EXPECT_TRUE(GetDataOfType(mojom::kMimeTypeHTML, &data));
  EXPECT_EQ(kHtmlData, data);
}

TEST_F(ClipboardAppTest, CanClearClipboardWithZeroArray) {
  std::string data;
  SetStringText(kPlainTextData);
  EXPECT_EQ(1ul, GetSequenceNumber());

  EXPECT_TRUE(GetDataOfType(mojom::kMimeTypeText, &data));
  EXPECT_EQ(kPlainTextData, data);

  base::flat_map<std::string, std::vector<uint8_t>> mime_data;
  uint64_t sequence_num = 0;
  clipboard_->WriteClipboardData(Clipboard::Type::COPY_PASTE,
                                 std::move(mime_data),
                                 &sequence_num);

  EXPECT_EQ(2ul, sequence_num);
  EXPECT_FALSE(GetDataOfType(mojom::kMimeTypeText, &data));
}

}  // namespace clipboard
}  // namespace ui
