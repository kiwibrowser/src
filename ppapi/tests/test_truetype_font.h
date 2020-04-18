// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAPPI_TESTS_TEST_TRUETYPE_FONT_H_
#define PAPPI_TESTS_TEST_TRUETYPE_FONT_H_

#include <string>

#include "ppapi/c/dev/ppb_truetype_font_dev.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/tests/test_case.h"

class TestTrueTypeFont : public TestCase {
 public:
  explicit TestTrueTypeFont(TestingInstance* instance);
  virtual ~TestTrueTypeFont();

 private:
  // TestCase implementation.
  virtual bool Init();
  virtual void RunTests(const std::string& filter);

  std::string TestGetFontFamilies();
  std::string TestGetFontsInFamily();
  std::string TestCreate();
  std::string TestDescribe();
  std::string TestGetTableTags();
  std::string TestGetTable();

  const PPB_TrueTypeFont_Dev* ppb_truetype_font_interface_;
  const PPB_Core* ppb_core_interface_;
  const PPB_Var* ppb_var_interface_;
};

#endif  // PAPPI_TESTS_TEST_TRUETYPE_FONT_H_
