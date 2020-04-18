// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/fm2js/cxfa_fmsimpleexpression.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "core/fxcrt/autorestorer.h"
#include "core/fxcrt/cfx_widetextbuf.h"
#include "core/fxcrt/fx_extension.h"
#include "third_party/base/logging.h"
#include "xfa/fxfa/fm2js/cxfa_fmtojavascriptdepth.h"

namespace {

const wchar_t* const g_BuiltInFuncs[] = {
    L"Abs",          L"Apr",       L"At",       L"Avg",
    L"Ceil",         L"Choose",    L"Concat",   L"Count",
    L"Cterm",        L"Date",      L"Date2Num", L"DateFmt",
    L"Decode",       L"Encode",    L"Eval",     L"Exists",
    L"Floor",        L"Format",    L"FV",       L"Get",
    L"HasValue",     L"If",        L"Ipmt",     L"IsoDate2Num",
    L"IsoTime2Num",  L"Left",      L"Len",      L"LocalDateFmt",
    L"LocalTimeFmt", L"Lower",     L"Ltrim",    L"Max",
    L"Min",          L"Mod",       L"NPV",      L"Num2Date",
    L"Num2GMTime",   L"Num2Time",  L"Oneof",    L"Parse",
    L"Pmt",          L"Post",      L"PPmt",     L"Put",
    L"PV",           L"Rate",      L"Ref",      L"Replace",
    L"Right",        L"Round",     L"Rtrim",    L"Space",
    L"Str",          L"Stuff",     L"Substr",   L"Sum",
    L"Term",         L"Time",      L"Time2Num", L"TimeFmt",
    L"UnitType",     L"UnitValue", L"Upper",    L"Uuid",
    L"Within",       L"WordNum",
};

const size_t g_BuiltInFuncsMaxLen = 12;

struct XFA_FMSOMMethod {
  const wchar_t* m_wsSomMethodName;
  uint32_t m_dParameters;
};

const XFA_FMSOMMethod gs_FMSomMethods[] = {
    {L"absPage", 0x01},
    {L"absPageInBatch", 0x01},
    {L"absPageSpan", 0x01},
    {L"append", 0x01},
    {L"clear", 0x01},
    {L"formNodes", 0x01},
    {L"h", 0x01},
    {L"insert", 0x03},
    {L"isRecordGroup", 0x01},
    {L"page", 0x01},
    {L"pageSpan", 0x01},
    {L"remove", 0x01},
    {L"saveFilteredXML", 0x01},
    {L"setElement", 0x01},
    {L"sheet", 0x01},
    {L"sheetInBatch", 0x01},
    {L"sign", 0x61},
    {L"verify", 0x0d},
    {L"w", 0x01},
    {L"x", 0x01},
    {L"y", 0x01},
};

}  // namespace

CXFA_FMSimpleExpression::CXFA_FMSimpleExpression(XFA_FM_TOKEN op) : m_op(op) {}

XFA_FM_TOKEN CXFA_FMSimpleExpression::GetOperatorToken() const {
  return m_op;
}

CXFA_FMNullExpression::CXFA_FMNullExpression()
    : CXFA_FMSimpleExpression(TOKnull) {}

bool CXFA_FMNullExpression::ToJavaScript(CFX_WideTextBuf* js, ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << L"null";
  return !CXFA_IsTooBig(js);
}

CXFA_FMNumberExpression::CXFA_FMNumberExpression(WideStringView wsNumber)
    : CXFA_FMSimpleExpression(TOKnumber), m_wsNumber(wsNumber) {}

CXFA_FMNumberExpression::~CXFA_FMNumberExpression() = default;

bool CXFA_FMNumberExpression::ToJavaScript(CFX_WideTextBuf* js,
                                           ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << m_wsNumber;
  return !CXFA_IsTooBig(js);
}

CXFA_FMStringExpression::CXFA_FMStringExpression(WideStringView wsString)
    : CXFA_FMSimpleExpression(TOKstring), m_wsString(wsString) {}

CXFA_FMStringExpression::~CXFA_FMStringExpression() = default;

bool CXFA_FMStringExpression::ToJavaScript(CFX_WideTextBuf* js,
                                           ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  WideString tempStr(m_wsString);
  if (tempStr.GetLength() <= 2) {
    *js << tempStr;
    return !CXFA_IsTooBig(js);
  }

  *js << L"\"";
  for (size_t i = 1; i < tempStr.GetLength() - 1; i++) {
    wchar_t oneChar = tempStr[i];
    switch (oneChar) {
      case L'\"':
        ++i;
        *js << L"\\\"";
        break;
      case 0x0d:
        break;
      case 0x0a:
        *js << L"\\n";
        break;
      default:
        js->AppendChar(oneChar);
        break;
    }
  }
  *js << L"\"";
  return !CXFA_IsTooBig(js);
}

CXFA_FMIdentifierExpression::CXFA_FMIdentifierExpression(
    WideStringView wsIdentifier)
    : CXFA_FMSimpleExpression(TOKidentifier), m_wsIdentifier(wsIdentifier) {}

CXFA_FMIdentifierExpression::~CXFA_FMIdentifierExpression() = default;

bool CXFA_FMIdentifierExpression::ToJavaScript(CFX_WideTextBuf* js,
                                               ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  if (m_wsIdentifier == L"$")
    *js << L"this";
  else if (m_wsIdentifier == L"!")
    *js << L"xfa.datasets";
  else if (m_wsIdentifier == L"$data")
    *js << L"xfa.datasets.data";
  else if (m_wsIdentifier == L"$event")
    *js << L"xfa.event";
  else if (m_wsIdentifier == L"$form")
    *js << L"xfa.form";
  else if (m_wsIdentifier == L"$host")
    *js << L"xfa.host";
  else if (m_wsIdentifier == L"$layout")
    *js << L"xfa.layout";
  else if (m_wsIdentifier == L"$template")
    *js << L"xfa.template";
  else if (m_wsIdentifier[0] == L'!')
    *js << L"pfm__excl__" +
               m_wsIdentifier.Right(m_wsIdentifier.GetLength() - 1);
  else
    *js << m_wsIdentifier;

  return !CXFA_IsTooBig(js);
}

CXFA_FMAssignExpression::CXFA_FMAssignExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMSimpleExpression(op),
      m_pExp1(std::move(pExp1)),
      m_pExp2(std::move(pExp2)) {}

CXFA_FMAssignExpression::~CXFA_FMAssignExpression() = default;

bool CXFA_FMAssignExpression::ToJavaScript(CFX_WideTextBuf* js,
                                           ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  CFX_WideTextBuf tempExp1;
  if (!m_pExp1->ToJavaScript(&tempExp1, ReturnType::kInfered))
    return false;

  *js << L"if (pfm_rt.is_obj(" << tempExp1 << L"))\n{\n";
  if (type == ReturnType::kImplied)
    *js << L"pfm_ret = ";

  CFX_WideTextBuf tempExp2;
  if (!m_pExp2->ToJavaScript(&tempExp2, ReturnType::kInfered))
    return false;

  *js << L"pfm_rt.asgn_val_op(" << tempExp1 << L", " << tempExp2 << L");\n}\n";

  if (m_pExp1->GetOperatorToken() == TOKidentifier &&
      tempExp1.AsStringView() != L"this") {
    *js << L"else\n{\n";
    if (type == ReturnType::kImplied)
      *js << L"pfm_ret = ";

    *js << tempExp1 << L" = pfm_rt.asgn_val_op";
    *js << L"(" << tempExp1 << L", " << tempExp2 << L");\n";
    *js << L"}\n";
  }
  return !CXFA_IsTooBig(js);
}

CXFA_FMBinExpression::CXFA_FMBinExpression(
    const WideString& opName,
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMSimpleExpression(op),
      m_OpName(opName),
      m_pExp1(std::move(pExp1)),
      m_pExp2(std::move(pExp2)) {}

CXFA_FMBinExpression::~CXFA_FMBinExpression() = default;

bool CXFA_FMBinExpression::ToJavaScript(CFX_WideTextBuf* js, ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << L"pfm_rt." << m_OpName << L"(";
  if (!m_pExp1->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L", ";
  if (!m_pExp2->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L")";
  return !CXFA_IsTooBig(js);
}

CXFA_FMLogicalOrExpression::CXFA_FMLogicalOrExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"log_or_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMLogicalAndExpression::CXFA_FMLogicalAndExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"log_and_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMEqualExpression::CXFA_FMEqualExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"eq_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMNotEqualExpression::CXFA_FMNotEqualExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"neq_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMGtExpression::CXFA_FMGtExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"gt_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMGeExpression::CXFA_FMGeExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"ge_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMLtExpression::CXFA_FMLtExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"lt_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMLeExpression::CXFA_FMLeExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"le_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMPlusExpression::CXFA_FMPlusExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"plus_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMMinusExpression::CXFA_FMMinusExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"minus_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMMulExpression::CXFA_FMMulExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"mul_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMDivExpression::CXFA_FMDivExpression(
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp2)
    : CXFA_FMBinExpression(L"div_op",
                           op,
                           std::move(pExp1),
                           std::move(pExp2)) {}

CXFA_FMUnaryExpression::CXFA_FMUnaryExpression(
    const WideString& opName,
    XFA_FM_TOKEN op,
    std::unique_ptr<CXFA_FMSimpleExpression> pExp)
    : CXFA_FMSimpleExpression(op), m_OpName(opName), m_pExp(std::move(pExp)) {}

CXFA_FMUnaryExpression::~CXFA_FMUnaryExpression() = default;

bool CXFA_FMUnaryExpression::ToJavaScript(CFX_WideTextBuf* js,
                                          ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << L"pfm_rt." << m_OpName.c_str() << L"(";
  if (!m_pExp->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L")";
  return !CXFA_IsTooBig(js);
}

CXFA_FMPosExpression::CXFA_FMPosExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pExp)
    : CXFA_FMUnaryExpression(L"pos_op", TOKplus, std::move(pExp)) {}

CXFA_FMNegExpression::CXFA_FMNegExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pExp)
    : CXFA_FMUnaryExpression(L"neg_op", TOKminus, std::move(pExp)) {}

CXFA_FMNotExpression::CXFA_FMNotExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pExp)
    : CXFA_FMUnaryExpression(L"log_not_op", TOKksnot, std::move(pExp)) {}

CXFA_FMCallExpression::CXFA_FMCallExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pExp,
    std::vector<std::unique_ptr<CXFA_FMSimpleExpression>>&& pArguments,
    bool bIsSomMethod)
    : CXFA_FMSimpleExpression(TOKcall),
      m_pExp(std::move(pExp)),
      m_bIsSomMethod(bIsSomMethod),
      m_Arguments(std::move(pArguments)) {}

CXFA_FMCallExpression::~CXFA_FMCallExpression() {}

bool CXFA_FMCallExpression::IsBuiltInFunc(CFX_WideTextBuf* funcName) {
  if (funcName->GetLength() > g_BuiltInFuncsMaxLen)
    return false;

  WideString str = funcName->MakeString();
  const wchar_t* const* pMatchResult = std::lower_bound(
      std::begin(g_BuiltInFuncs), std::end(g_BuiltInFuncs), str,
      [](const wchar_t* iter, const WideString& val) -> bool {
        return val.CompareNoCase(iter) > 0;
      });
  if (pMatchResult != std::end(g_BuiltInFuncs) &&
      !str.CompareNoCase(*pMatchResult)) {
    funcName->Clear();
    *funcName << *pMatchResult;
    return true;
  }
  return false;
}

uint32_t CXFA_FMCallExpression::IsMethodWithObjParam(
    const WideString& methodName) {
  const XFA_FMSOMMethod* result = std::lower_bound(
      std::begin(gs_FMSomMethods), std::end(gs_FMSomMethods), methodName,
      [](const XFA_FMSOMMethod iter, const WideString& val) {
        return val.Compare(iter.m_wsSomMethodName) > 0;
      });
  if (result != std::end(gs_FMSomMethods) &&
      !methodName.Compare(result->m_wsSomMethodName)) {
    return result->m_dParameters;
  }
  return 0;
}

bool CXFA_FMCallExpression::ToJavaScript(CFX_WideTextBuf* js, ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  CFX_WideTextBuf funcName;
  if (!m_pExp->ToJavaScript(&funcName, ReturnType::kInfered))
    return false;

  if (m_bIsSomMethod) {
    *js << funcName << L"(";
    uint32_t methodPara = IsMethodWithObjParam(funcName.MakeString());
    if (methodPara > 0) {
      for (size_t i = 0; i < m_Arguments.size(); ++i) {
        // Currently none of our expressions use objects for a parameter over
        // the 6th. Make sure we don't overflow the shift when doing this
        // check. If we ever need more the 32 object params we can revisit.
        *js << L"pfm_rt.get_";
        if (i < 32 && (methodPara & (0x01 << i)) > 0)
          *js << L"jsobj";
        else
          *js << L"val";

        *js << L"(";
        if (!m_Arguments[i]->ToJavaScript(js, ReturnType::kInfered))
          return false;
        *js << L")";
        if (i + 1 < m_Arguments.size())
          *js << L", ";
      }
    } else {
      for (const auto& expr : m_Arguments) {
        *js << L"pfm_rt.get_val(";
        if (!expr->ToJavaScript(js, ReturnType::kInfered))
          return false;
        *js << L")";
        if (expr != m_Arguments.back())
          *js << L", ";
      }
    }
    *js << L")";
    return !CXFA_IsTooBig(js);
  }

  bool isEvalFunc = false;
  bool isExistsFunc = false;
  if (!IsBuiltInFunc(&funcName)) {
    // If a function is not a SomMethod or a built-in then the input was
    // invalid, so failing. The scanner/lexer should catch this, but currently
    // doesn't. This failure will bubble up to the top-level and cause the
    // transpile to fail.
    return false;
  }

  if (funcName.AsStringView() == L"Eval") {
    isEvalFunc = true;
    *js << L"eval.call(this, pfm_rt.Translate";
  } else {
    if (funcName.AsStringView() == L"Exists")
      isExistsFunc = true;

    *js << L"pfm_rt." << funcName;
  }

  *js << L"(";
  if (isExistsFunc) {
    *js << L"\n(\nfunction ()\n{\ntry\n{\n";
    if (!m_Arguments.empty()) {
      *js << L"return ";
      if (!m_Arguments[0]->ToJavaScript(js, ReturnType::kInfered))
        return false;
      *js << L";\n}\n";
    } else {
      *js << L"return 0;\n}\n";
    }
    *js << L"catch(accessExceptions)\n";
    *js << L"{\nreturn 0;\n}\n}\n).call(this)\n";
  } else {
    for (const auto& expr : m_Arguments) {
      if (!expr->ToJavaScript(js, ReturnType::kInfered))
        return false;
      if (expr != m_Arguments.back())
        *js << L", ";
    }
  }
  *js << L")";
  if (isEvalFunc)
    *js << L")";

  return !CXFA_IsTooBig(js);
}

CXFA_FMDotAccessorExpression::CXFA_FMDotAccessorExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pAccessor,
    XFA_FM_TOKEN op,
    WideStringView wsIdentifier,
    std::unique_ptr<CXFA_FMSimpleExpression> pIndexExp)
    : CXFA_FMSimpleExpression(op),
      m_wsIdentifier(wsIdentifier),
      m_pExp1(std::move(pAccessor)),
      m_pExp2(std::move(pIndexExp)) {}

CXFA_FMDotAccessorExpression::~CXFA_FMDotAccessorExpression() = default;

bool CXFA_FMDotAccessorExpression::ToJavaScript(CFX_WideTextBuf* js,
                                                ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << L"pfm_rt.dot_acc(";

  CFX_WideTextBuf tempExp1;
  if (m_pExp1) {
    if (!m_pExp1->ToJavaScript(&tempExp1, ReturnType::kInfered))
      return false;

    *js << tempExp1;
  } else {
    *js << L"null";
  }
  *js << L", \"";

  if (m_pExp1 && m_pExp1->GetOperatorToken() == TOKidentifier)
    *js << tempExp1;

  *js << L"\", ";
  if (m_op == TOKdotscream)
    *js << L"\"#" << m_wsIdentifier << L"\", ";
  else if (m_op == TOKdotstar)
    *js << L"\"*\", ";
  else if (m_op == TOKcall)
    *js << L"\"\", ";
  else
    *js << L"\"" << m_wsIdentifier << L"\", ";

  if (!m_pExp2->ToJavaScript(js, ReturnType::kInfered))
    return false;

  *js << L")";
  return !CXFA_IsTooBig(js);
}

CXFA_FMIndexExpression::CXFA_FMIndexExpression(
    XFA_FM_AccessorIndex accessorIndex,
    std::unique_ptr<CXFA_FMSimpleExpression> pIndexExp,
    bool bIsStarIndex)
    : CXFA_FMSimpleExpression(TOKlbracket),
      m_pExp(std::move(pIndexExp)),
      m_accessorIndex(accessorIndex),
      m_bIsStarIndex(bIsStarIndex) {}

CXFA_FMIndexExpression::~CXFA_FMIndexExpression() = default;

bool CXFA_FMIndexExpression::ToJavaScript(CFX_WideTextBuf* js,
                                          ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  switch (m_accessorIndex) {
    case ACCESSOR_NO_INDEX:
      *js << L"0";
      break;
    case ACCESSOR_NO_RELATIVEINDEX:
      *js << L"1";
      break;
    case ACCESSOR_POSITIVE_INDEX:
      *js << L"2";
      break;
    case ACCESSOR_NEGATIVE_INDEX:
      *js << L"3";
      break;
    default:
      *js << L"0";
  }
  if (m_bIsStarIndex)
    return !CXFA_IsTooBig(js);

  *js << L", ";
  if (m_pExp) {
    if (!m_pExp->ToJavaScript(js, ReturnType::kInfered))
      return false;
  } else {
    *js << L"0";
  }
  return !CXFA_IsTooBig(js);
}

CXFA_FMDotDotAccessorExpression::CXFA_FMDotDotAccessorExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pAccessor,
    XFA_FM_TOKEN op,
    WideStringView wsIdentifier,
    std::unique_ptr<CXFA_FMSimpleExpression> pIndexExp)
    : CXFA_FMSimpleExpression(op),
      m_wsIdentifier(wsIdentifier),
      m_pExp1(std::move(pAccessor)),
      m_pExp2(std::move(pIndexExp)) {}

CXFA_FMDotDotAccessorExpression::~CXFA_FMDotDotAccessorExpression() = default;

bool CXFA_FMDotDotAccessorExpression::ToJavaScript(CFX_WideTextBuf* js,
                                                   ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << L"pfm_rt.dotdot_acc(";
  if (!m_pExp1->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L", " << L"\"";
  if (m_pExp1->GetOperatorToken() == TOKidentifier) {
    if (!m_pExp1->ToJavaScript(js, ReturnType::kInfered))
      return false;
  }

  *js << L"\", \"" << m_wsIdentifier << L"\", ";
  if (!m_pExp2->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L")";
  return !CXFA_IsTooBig(js);
}

CXFA_FMMethodCallExpression::CXFA_FMMethodCallExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pAccessorExp1,
    std::unique_ptr<CXFA_FMSimpleExpression> pCallExp)
    : CXFA_FMSimpleExpression(TOKdot),
      m_pExp1(std::move(pAccessorExp1)),
      m_pExp2(std::move(pCallExp)) {}

CXFA_FMMethodCallExpression::~CXFA_FMMethodCallExpression() = default;

bool CXFA_FMMethodCallExpression::ToJavaScript(CFX_WideTextBuf* js,
                                               ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  CFX_WideTextBuf buf;
  if (!m_pExp1->ToJavaScript(&buf, ReturnType::kInfered))
    return false;

  *js << L"(function() {\n";
  *js << L"  return pfm_method_runner(" << buf << L", function(obj) {\n";
  *js << L"    return obj.";
  if (!m_pExp2->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L";\n";
  *js << L"  });\n";
  *js << L"}).call(this)";
  return !CXFA_IsTooBig(js);
}

bool CXFA_IsTooBig(const CFX_WideTextBuf* js) {
  return js->GetSize() >= 256 * 1024 * 1024;
}
