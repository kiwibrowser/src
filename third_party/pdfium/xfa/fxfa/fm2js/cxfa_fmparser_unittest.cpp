// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xfa/fxfa/fm2js/cxfa_fmparser.h"

#include <vector>

#include "core/fxcrt/cfx_widetextbuf.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_support.h"
#include "third_party/base/ptr_util.h"
#include "xfa/fxfa/fm2js/cxfa_fmtojavascriptdepth.h"

TEST(CXFA_FMParserTest, Empty) {
  auto parser = pdfium::MakeUnique<CXFA_FMParser>(L"");
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser->HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  CFX_WideTextBuf buf;
  EXPECT_TRUE(ast->ToJavaScript(&buf));
  // TODO(dsinclair): This is a little weird .....
  EXPECT_EQ(L"// comments only", buf.AsStringView());
}

TEST(CXFA_FMParserTest, CommentOnlyIsError) {
  auto parser = pdfium::MakeUnique<CXFA_FMParser>(L"; Just comment");
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast);
  // TODO(dsinclair): This isn't allowed per the spec.
  EXPECT_FALSE(parser->HasError());
  // EXPECT_TRUE(parser->HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  CFX_WideTextBuf buf;
  EXPECT_TRUE(ast->ToJavaScript(&buf));
  EXPECT_EQ(L"// comments only", buf.AsStringView());
}

TEST(CXFA_FMParserTest, CommentThenValue) {
  const wchar_t ret[] =
      L"(function() {\n"
      L"let pfm_method_runner = function(obj, cb) {\n"
      L"  if (pfm_rt.is_ary(obj)) {\n"
      L"    let pfm_method_return = null;\n"
      L"    for (var idx = obj.length -1; idx > 1; idx--) {\n"
      L"      pfm_method_return = cb(obj[idx]);\n"
      L"    }\n"
      L"    return pfm_method_return;\n"
      L"  }\n"
      L"  return cb(obj);\n"
      L"};\n"
      L"var pfm_ret = null;\n"
      L"pfm_ret = 12;\n"
      L"return pfm_rt.get_val(pfm_ret);\n"
      L"}).call(this);";

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(L"; Just comment\n12");
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser->HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  CFX_WideTextBuf buf;
  EXPECT_TRUE(ast->ToJavaScript(&buf));
  EXPECT_EQ(ret, buf.AsStringView());
}

TEST(CXFA_FMParserTest, Parse) {
  const wchar_t input[] =
      L"$ = Avg (-3, 5, -6, 12, -13);\n"
      L"$ = Avg (Table2..Row[*].Cell1);\n"
      L"\n"
      L"if ($ ne -1)then\n"
      L"  border.fill.color.value = \"255,64,64\";\n"
      L"elseif ($ ne -2) then\n"
      L"  border.fill.color.value = \"128,128,128\";\n"
      L"else\n"
      L"  border.fill.color.value = \"20,170,13\";\n"
      L"endif\n"
      L"\n"
      L"$";

  const wchar_t ret[] =
      L"(function() {\n"
      L"let pfm_method_runner = function(obj, cb) {\n"
      L"  if (pfm_rt.is_ary(obj)) {\n"
      L"    let pfm_method_return = null;\n"
      L"    for (var idx = obj.length -1; idx > 1; idx--) {\n"
      L"      pfm_method_return = cb(obj[idx]);\n"
      L"    }\n"
      L"    return pfm_method_return;\n"
      L"  }\n"
      L"  return cb(obj);\n"
      L"};\n"
      L"var pfm_ret = null;\n"
      L"if (pfm_rt.is_obj(this))\n{\n"
      L"pfm_rt.asgn_val_op(this, pfm_rt.Avg(pfm_rt.neg_op(3), 5, "
      L"pfm_rt.neg_op(6), 12, pfm_rt.neg_op(13)));\n"
      L"}\n"
      L"if (pfm_rt.is_obj(this))\n{\n"
      L"pfm_rt.asgn_val_op(this, pfm_rt.Avg(pfm_rt.dot_acc(pfm_rt.dotdot_acc("
      L"Table2, \"Table2\", \"Row\", 1), \"\", \"Cell1\", 0, 0)));\n"
      L"}\n"
      L"if (pfm_rt.get_val(pfm_rt.neq_op(this, pfm_rt.neg_op(1))))\n{\n"
      L"if (pfm_rt.is_obj(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc("
      L"border, \"border\", \"fill\", 0, 0), \"\", \"color\", 0, 0), \"\", "
      L"\"value\", 0, 0)))\n{\n"
      L"pfm_rt.asgn_val_op(pfm_rt.dot_acc(pfm_rt.dot_acc("
      L"pfm_rt.dot_acc(border, \"border\", \"fill\", 0, 0), \"\", "
      L"\"color\", 0, 0), \"\", \"value\", 0, 0), \"255,64,64\");\n"
      L"}\n"
      L"}\nelse if (pfm_rt.get_val(pfm_rt.neq_op(this, pfm_rt.neg_op(2))))\n"
      L"{\n"
      L"if (pfm_rt.is_obj(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc(border, "
      L"\"border\", \"fill\", 0, 0), \"\", \"color\", 0, 0), \"\", \"value\", "
      L"0, 0)))\n"
      L"{\n"
      L"pfm_rt.asgn_val_op(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc(border,"
      L" \"border\", \"fill\", 0, 0), \"\", \"color\", 0, 0), \"\", \"value\", "
      L"0, 0), \"128,128,128\");\n"
      L"}\n"
      L"}\n"
      L"else {\n"
      L"if (pfm_rt.is_obj(pfm_rt.dot_acc(pfm_rt.dot_acc(pfm_rt.dot_acc("
      L"border, \"border\", \"fill\", 0, 0), \"\", \"color\", 0, 0), \"\", "
      L"\"value\", 0, 0)))\n{\n"
      L"pfm_rt.asgn_val_op(pfm_rt.dot_acc(pfm_rt.dot_acc("
      L"pfm_rt.dot_acc(border, \"border\", \"fill\", 0, 0), \"\", "
      L"\"color\", 0, 0), \"\", \"value\", 0, 0), \"20,170,13\");\n"
      L"}\n"
      L"}\n"
      L"pfm_ret = this;\n"
      L"return pfm_rt.get_val(pfm_ret);\n"
      L"}).call(this);";

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser->HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  CFX_WideTextBuf buf;
  EXPECT_TRUE(ast->ToJavaScript(&buf));
  EXPECT_EQ(ret, buf.AsStringView());
}

TEST(CXFA_FMParserTest, MaxParseDepth) {
  auto parser = pdfium::MakeUnique<CXFA_FMParser>(L"foo(bar[baz(fizz[0])])");
  parser->SetMaxParseDepthForTest(5);
  EXPECT_EQ(nullptr, parser->Parse());
  EXPECT_TRUE(parser->HasError());
}

TEST(CFXA_FMParserTest, chromium752201) {
  auto parser = pdfium::MakeUnique<CXFA_FMParser>(
      L"fTep a\n"
      L".#\n"
      L"fo@ =[=l");
  EXPECT_EQ(nullptr, parser->Parse());
  EXPECT_TRUE(parser->HasError());
}

TEST(CXFA_FMParserTest, MultipleAssignmentIsNotAllowed) {
  auto parser = pdfium::MakeUnique<CXFA_FMParser>(L"(a=(b=t))=u");

  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(!ast);
  EXPECT_TRUE(parser->HasError());
}

TEST(CXFA_FMParserTest, ParseFuncWithParams) {
  const wchar_t input[] = {
      L"func MyFunction(param1, param2) do\n"
      L"  param1 * param2\n"
      L"endfunc"};

  const wchar_t ret[] = {
      L"(function() {\n"
      L"let pfm_method_runner = function(obj, cb) {\n"
      L"  if (pfm_rt.is_ary(obj)) {\n"
      L"    let pfm_method_return = null;\n"
      L"    for (var idx = obj.length -1; idx > 1; idx--) {\n"
      L"      pfm_method_return = cb(obj[idx]);\n"
      L"    }\n"
      L"    return pfm_method_return;\n"
      L"  }\n"
      L"  return cb(obj);\n"
      L"};\n"
      L"var pfm_ret = null;\n"
      L"function MyFunction(param1, param2) {\n"
      L"var pfm_ret = null;\n"
      L"pfm_ret = pfm_rt.mul_op(param1, param2);\n"
      L"return pfm_ret;\n"
      L"}\n"
      L"return pfm_rt.get_val(pfm_ret);\n"
      L"}).call(this);"};

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser->HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  CFX_WideTextBuf buf;
  EXPECT_TRUE(ast->ToJavaScript(&buf));
  EXPECT_EQ(ret, buf.AsStringView());
}

TEST(CXFA_FMParserTest, ParseFuncWithoutParams) {
  const wchar_t input[] = {
      L"func MyFunction() do\n"
      L"  42\n"
      L"endfunc"};

  const wchar_t ret[] = {
      L"(function() {\n"
      L"let pfm_method_runner = function(obj, cb) {\n"
      L"  if (pfm_rt.is_ary(obj)) {\n"
      L"    let pfm_method_return = null;\n"
      L"    for (var idx = obj.length -1; idx > 1; idx--) {\n"
      L"      pfm_method_return = cb(obj[idx]);\n"
      L"    }\n"
      L"    return pfm_method_return;\n"
      L"  }\n"
      L"  return cb(obj);\n"
      L"};\n"
      L"var pfm_ret = null;\n"
      L"function MyFunction() {\n"
      L"var pfm_ret = null;\n"
      L"pfm_ret = 42;\n"
      L"return pfm_ret;\n"
      L"}\n"
      L"return pfm_rt.get_val(pfm_ret);\n"
      L"}).call(this);"};

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast);
  EXPECT_FALSE(parser->HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  CFX_WideTextBuf buf;
  EXPECT_TRUE(ast->ToJavaScript(&buf));
  EXPECT_EQ(ret, buf.AsStringView());
}

TEST(CXFA_FMParserTest, ParseFuncWithBadParamsList) {
  const wchar_t input[] = {
      L"func MyFunction(param1,) do\n"
      L"  param1 * param2\n"
      L"endfunc"};

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser->HasError());
}

TEST(CXFA_FMParserTest, ParseBadIfExpression) {
  const wchar_t input[] = {L"if ( then"};

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser->HasError());
}

TEST(CXFA_FMParserTest, ParseBadElseIfExpression) {
  const wchar_t input[] = {
      L"if ($ ne -1) then\n"
      L"elseif( then"};

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  ASSERT_TRUE(ast == nullptr);
  EXPECT_TRUE(parser->HasError());
}

TEST(CXFA_FMParserTest, ParseDepthWithWideTree) {
  const wchar_t input[] = {L"a <> b <> c <> d <> e <> f <> g <> h <> i <> j"};

  {
    auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
    std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
    ASSERT_TRUE(ast);
    EXPECT_TRUE(!parser->HasError());
  }

  {
    auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
    parser->SetMaxParseDepthForTest(5);
    std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
    ASSERT_TRUE(ast == nullptr);
    EXPECT_TRUE(parser->HasError());
  }
}

TEST(CXFA_FMParserTest, ParseCallSmall) {
  const wchar_t input[] = {L"i.f(O)"};
  const wchar_t ret[] = {
      L"(function() {\n"
      L"let pfm_method_runner = function(obj, cb) {\n"
      L"  if (pfm_rt.is_ary(obj)) {\n"
      L"    let pfm_method_return = null;\n"
      L"    for (var idx = obj.length -1; idx > 1; idx--) {\n"
      L"      pfm_method_return = cb(obj[idx]);\n"
      L"    }\n"
      L"    return pfm_method_return;\n"
      L"  }\n"
      L"  return cb(obj);\n"
      L"};\n"
      L"var pfm_ret = null;\n"
      L"pfm_ret = pfm_rt.get_val((function() {\n"
      L"  return pfm_method_runner(i, function(obj) {\n"
      L"    return obj.f(pfm_rt.get_val(O));\n"
      L"  });\n"
      L"}).call(this));\n"
      L"return pfm_rt.get_val(pfm_ret);\n"
      L"}).call(this);"};

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  EXPECT_FALSE(parser->HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  CFX_WideTextBuf buf;
  EXPECT_TRUE(ast->ToJavaScript(&buf));
  EXPECT_EQ(ret, buf.AsStringView());
}

TEST(CXFA_FMParserTest, ParseCallBig) {
  const wchar_t input[] = {L"i.f(O.e(O.e(O)))"};
  const wchar_t ret[] = {
      L"(function() {\n"
      L"let pfm_method_runner = function(obj, cb) {\n"
      L"  if (pfm_rt.is_ary(obj)) {\n"
      L"    let pfm_method_return = null;\n"
      L"    for (var idx = obj.length -1; idx > 1; idx--) {\n"
      L"      pfm_method_return = cb(obj[idx]);\n"
      L"    }\n"
      L"    return pfm_method_return;\n"
      L"  }\n"
      L"  return cb(obj);\n"
      L"};\n"
      L"var pfm_ret = null;\n"
      L"pfm_ret = pfm_rt.get_val((function() {\n"
      L"  return pfm_method_runner(i, function(obj) {\n"
      L"    return obj.f(pfm_rt.get_val((function() {\n"
      L"  return pfm_method_runner(O, function(obj) {\n"
      L"    return obj.e(pfm_rt.get_val((function() {\n"
      L"  return pfm_method_runner(O, function(obj) {\n"
      L"    return obj.e(pfm_rt.get_val(O));\n"
      L"  });\n"
      L"}).call(this)));\n"
      L"  });\n"
      L"}).call(this)));\n"
      L"  });\n"
      L"}).call(this));\n"
      L"return pfm_rt.get_val(pfm_ret);\n"
      L"}).call(this);"};

  auto parser = pdfium::MakeUnique<CXFA_FMParser>(input);
  std::unique_ptr<CXFA_FMAST> ast = parser->Parse();
  EXPECT_FALSE(parser->HasError());

  CXFA_FMToJavaScriptDepth::Reset();
  CFX_WideTextBuf buf;
  EXPECT_TRUE(ast->ToJavaScript(&buf));
  EXPECT_EQ(ret, buf.AsStringView());
}
