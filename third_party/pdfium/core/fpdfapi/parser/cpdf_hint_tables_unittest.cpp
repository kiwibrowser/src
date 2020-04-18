// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/cpdf_hint_tables.h"

#include <memory>
#include <string>
#include <utility>

#include "core/fpdfapi/cpdf_modulemgr.h"
#include "core/fpdfapi/parser/cpdf_data_avail.h"
#include "core/fxcrt/fx_stream.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/path_service.h"
#include "third_party/base/ptr_util.h"

namespace {

std::unique_ptr<CPDF_DataAvail> MakeDataAvailFromFile(
    const std::string& file_name) {
  std::string file_path;
  if (!PathService::GetTestFilePath(file_name, &file_path))
    return nullptr;
  return pdfium::MakeUnique<CPDF_DataAvail>(
      nullptr, IFX_SeekableReadStream::CreateFromFilename(file_path.c_str()),
      true);
}

}  // namespace

class CPDF_HintTablesTest : public testing::Test {
 public:
  CPDF_HintTablesTest() {
    // Needs for encoding Hint table stream.
    CPDF_ModuleMgr::Get()->Init();
  }

  ~CPDF_HintTablesTest() override { CPDF_ModuleMgr::Destroy(); }
};

TEST_F(CPDF_HintTablesTest, Load) {
  auto data_avail = MakeDataAvailFromFile("feature_linearized_loading.pdf");
  ASSERT_EQ(CPDF_DataAvail::DocAvailStatus::DataAvailable,
            data_avail->IsDocAvail(nullptr));

  ASSERT_TRUE(data_avail->GetHintTables());

  const CPDF_HintTables* hint_tables = data_avail->GetHintTables();
  FX_FILESIZE page_start = 0;
  FX_FILESIZE page_length = 0;
  uint32_t page_obj_num = 0;

  ASSERT_TRUE(
      hint_tables->GetPagePos(0, &page_start, &page_length, &page_obj_num));
  EXPECT_EQ(777, page_start);
  EXPECT_EQ(4328, page_length);
  EXPECT_EQ(39u, page_obj_num);

  ASSERT_TRUE(
      hint_tables->GetPagePos(1, &page_start, &page_length, &page_obj_num));
  EXPECT_EQ(5105, page_start);
  EXPECT_EQ(767, page_length);
  EXPECT_EQ(1u, page_obj_num);

  ASSERT_FALSE(
      hint_tables->GetPagePos(2, &page_start, &page_length, &page_obj_num));
}
