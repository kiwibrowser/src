// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/renovations/page_renovation_loader.h"

#include <memory>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
const char kTestRenovationKey1[] = "abc_renovation";
const char kTestRenovationKey2[] = "xyz_renovation";
const char kTestRenovationScript[] =
    "\nrun_renovations([\"abc_renovation\",\"xyz_renovation\",]);";
}  // namespace

class PageRenovationLoaderTest : public testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;

 protected:
  std::unique_ptr<PageRenovationLoader> page_renovation_loader_;
};

void PageRenovationLoaderTest::SetUp() {
  page_renovation_loader_.reset(new PageRenovationLoader);
  page_renovation_loader_->SetSourceForTest(base::string16());
}

void PageRenovationLoaderTest::TearDown() {
  page_renovation_loader_.reset();
}

TEST_F(PageRenovationLoaderTest, NoRenovations) {
  std::vector<std::string> renovation_keys;
  // Pass empty list
  base::string16 script;
  EXPECT_TRUE(
      page_renovation_loader_->GetRenovationScript(renovation_keys, &script));

  EXPECT_EQ(script, base::UTF8ToUTF16("\nrun_renovations([]);"));
}

TEST_F(PageRenovationLoaderTest, TestRenovations) {
  std::vector<std::string> renovation_keys;
  renovation_keys.push_back(kTestRenovationKey1);
  renovation_keys.push_back(kTestRenovationKey2);

  base::string16 script;
  EXPECT_TRUE(
      page_renovation_loader_->GetRenovationScript(renovation_keys, &script));

  EXPECT_EQ(script, base::UTF8ToUTF16(kTestRenovationScript));
}

TEST_F(PageRenovationLoaderTest, ReturnsSameScript) {
  std::vector<std::string> renovation_keys;
  renovation_keys.push_back(kTestRenovationKey1);
  renovation_keys.push_back(kTestRenovationKey2);

  base::string16 script1, script2;
  EXPECT_TRUE(
      page_renovation_loader_->GetRenovationScript(renovation_keys, &script1));
  EXPECT_TRUE(
      page_renovation_loader_->GetRenovationScript(renovation_keys, &script2));

  EXPECT_EQ(script1, script2);
}

}  // namespace offline_pages
