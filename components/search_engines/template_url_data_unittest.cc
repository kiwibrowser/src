// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_engines/template_url_data.h"

#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(TemplateURLDataTest, Trim) {
  TemplateURLData data(
      base::ASCIIToUTF16(" shortname "), base::ASCIIToUTF16(" keyword "),
      "https://cs.chromium.org", base::StringPiece(), base::StringPiece(),
      base::StringPiece(), base::StringPiece(), base::StringPiece(),
      base::StringPiece(), base::StringPiece(), base::StringPiece(),
      base::StringPiece(), base::StringPiece(), base::StringPiece(),
      base::ListValue(), 0);

  EXPECT_EQ(base::ASCIIToUTF16("shortname"), data.short_name());
  EXPECT_EQ(base::ASCIIToUTF16("keyword"), data.keyword());

  data.SetShortName(base::ASCIIToUTF16(" othershortname "));
  data.SetKeyword(base::ASCIIToUTF16(" otherkeyword "));

  EXPECT_EQ(base::ASCIIToUTF16("othershortname"), data.short_name());
  EXPECT_EQ(base::ASCIIToUTF16("otherkeyword"), data.keyword());
}
