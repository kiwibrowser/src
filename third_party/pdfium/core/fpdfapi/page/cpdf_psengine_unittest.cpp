// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/page/cpdf_psengine.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CPDF_PSProc, AddOperator) {
  static const struct {
    const char* name;
    PDF_PSOP op;
  } kTestData[] = {
      {"add", PSOP_ADD},         {"sub", PSOP_SUB},
      {"mul", PSOP_MUL},         {"div", PSOP_DIV},
      {"idiv", PSOP_IDIV},       {"mod", PSOP_MOD},
      {"neg", PSOP_NEG},         {"abs", PSOP_ABS},
      {"ceiling", PSOP_CEILING}, {"floor", PSOP_FLOOR},
      {"round", PSOP_ROUND},     {"truncate", PSOP_TRUNCATE},
      {"sqrt", PSOP_SQRT},       {"sin", PSOP_SIN},
      {"cos", PSOP_COS},         {"atan", PSOP_ATAN},
      {"exp", PSOP_EXP},         {"ln", PSOP_LN},
      {"log", PSOP_LOG},         {"cvi", PSOP_CVI},
      {"cvr", PSOP_CVR},         {"eq", PSOP_EQ},
      {"ne", PSOP_NE},           {"gt", PSOP_GT},
      {"ge", PSOP_GE},           {"lt", PSOP_LT},
      {"le", PSOP_LE},           {"and", PSOP_AND},
      {"or", PSOP_OR},           {"xor", PSOP_XOR},
      {"not", PSOP_NOT},         {"bitshift", PSOP_BITSHIFT},
      {"true", PSOP_TRUE},       {"false", PSOP_FALSE},
      {"if", PSOP_IF},           {"ifelse", PSOP_IFELSE},
      {"pop", PSOP_POP},         {"exch", PSOP_EXCH},
      {"dup", PSOP_DUP},         {"copy", PSOP_COPY},
      {"index", PSOP_INDEX},     {"roll", PSOP_ROLL},
      {"55", PSOP_CONST},        {"123.4", PSOP_CONST},
      {"-5", PSOP_CONST},        {"invalid", PSOP_CONST},
  };

  CPDF_PSProc proc;
  EXPECT_EQ(0U, proc.num_operators());
  for (size_t i = 0; i < FX_ArraySize(kTestData); ++i) {
    ByteStringView word(kTestData[i].name);
    proc.AddOperatorForTesting(word);
    ASSERT_EQ(i + 1, proc.num_operators());
    const std::unique_ptr<CPDF_PSOP>& new_psop = proc.last_operator();
    ASSERT_TRUE(new_psop);
    PDF_PSOP new_op = new_psop->GetOp();
    EXPECT_EQ(kTestData[i].op, new_op);
    if (new_op == PSOP_CONST) {
      float fv = new_psop->GetFloatValue();
      if (word == "invalid")
        EXPECT_FLOAT_EQ(0, fv);
      else
        EXPECT_EQ(word, ByteString::FormatFloat(fv));
    }
  }
}
