// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/fm2js/cxfa_fmexpression.h"

#include <utility>

#include "core/fxcrt/cfx_widetextbuf.h"
#include "xfa/fxfa/fm2js/cxfa_fmsimpleexpression.h"
#include "xfa/fxfa/fm2js/cxfa_fmtojavascriptdepth.h"

namespace {

const wchar_t kLessEqual[] = L" <= ";
const wchar_t kGreaterEqual[] = L" >= ";
const wchar_t kPlusEqual[] = L" += ";
const wchar_t kMinusEqual[] = L" -= ";

WideString IdentifierToName(WideStringView ident) {
  if (ident.IsEmpty())
    return L"";
  if (ident[0] != L'!')
    return WideString(ident);
  return L"pfm__excl__" + ident.Right(ident.GetLength() - 1);
}

}  // namespace

CXFA_FMExpression::CXFA_FMExpression() = default;

CXFA_FMFunctionDefinition::CXFA_FMFunctionDefinition(
    const WideStringView& wsName,
    std::vector<WideStringView>&& arguments,
    std::vector<std::unique_ptr<CXFA_FMExpression>>&& expressions)
    : CXFA_FMExpression(),
      m_wsName(wsName),
      m_pArguments(std::move(arguments)),
      m_pExpressions(std::move(expressions)) {
  ASSERT(!wsName.IsEmpty());
}

CXFA_FMFunctionDefinition::~CXFA_FMFunctionDefinition() = default;

bool CXFA_FMFunctionDefinition::ToJavaScript(CFX_WideTextBuf* js,
                                             ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  if (m_wsName.IsEmpty())
    return false;

  *js << L"function " << IdentifierToName(m_wsName) << L"(";
  for (const auto& identifier : m_pArguments) {
    if (identifier != m_pArguments.front())
      *js << L", ";

    *js << IdentifierToName(identifier);
  }
  *js << L") {\n";

  *js << L"var pfm_ret = null;\n";
  for (const auto& expr : m_pExpressions) {
    ReturnType ret_type = expr == m_pExpressions.back() ? ReturnType::kImplied
                                                        : ReturnType::kInfered;
    if (!expr->ToJavaScript(js, ret_type))
      return false;
  }

  *js << L"return pfm_ret;\n";
  *js << L"}\n";

  return !CXFA_IsTooBig(js);
}

CXFA_FMAST::CXFA_FMAST(
    std::vector<std::unique_ptr<CXFA_FMExpression>> expressions)
    : expressions_(std::move(expressions)) {}

CXFA_FMAST::~CXFA_FMAST() = default;

bool CXFA_FMAST::ToJavaScript(CFX_WideTextBuf* js) {
  if (expressions_.empty()) {
    *js << L"// comments only";
    return !CXFA_IsTooBig(js);
  }

  *js << L"(function() {\n";
  *js << L"let pfm_method_runner = function(obj, cb) {\n";
  *js << L"  if (pfm_rt.is_ary(obj)) {\n";
  *js << L"    let pfm_method_return = null;\n";
  *js << L"    for (var idx = obj.length -1; idx > 1; idx--) {\n";
  *js << L"      pfm_method_return = cb(obj[idx]);\n";
  *js << L"    }\n";
  *js << L"    return pfm_method_return;\n";
  *js << L"  }\n";
  *js << L"  return cb(obj);\n";
  *js << L"};\n";
  *js << L"var pfm_ret = null;\n";

  for (const auto& expr : expressions_) {
    ReturnType ret_type = expr == expressions_.back() ? ReturnType::kImplied
                                                      : ReturnType::kInfered;
    if (!expr->ToJavaScript(js, ret_type))
      return false;
  }

  *js << L"return pfm_rt.get_val(pfm_ret);\n";
  *js << L"}).call(this);";
  return !CXFA_IsTooBig(js);
}

CXFA_FMVarExpression::CXFA_FMVarExpression(
    const WideStringView& wsName,
    std::unique_ptr<CXFA_FMSimpleExpression> pInit)
    : CXFA_FMExpression(), m_wsName(wsName), m_pInit(std::move(pInit)) {}

CXFA_FMVarExpression::~CXFA_FMVarExpression() = default;

bool CXFA_FMVarExpression::ToJavaScript(CFX_WideTextBuf* js, ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  WideString tempName = IdentifierToName(m_wsName);
  *js << L"var " << tempName << L" = ";
  if (m_pInit) {
    if (!m_pInit->ToJavaScript(js, ReturnType::kInfered))
      return false;

    *js << tempName << L" = pfm_rt.var_filter(" << tempName << L");\n";
  } else {
    *js << L"\"\";\n";
  }

  if (type == ReturnType::kImplied)
    *js << L"pfm_ret = " << tempName << L";\n";

  return !CXFA_IsTooBig(js);
}

CXFA_FMExpExpression::CXFA_FMExpExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pExpression)
    : CXFA_FMExpression(), m_pExpression(std::move(pExpression)) {}

CXFA_FMExpExpression::~CXFA_FMExpExpression() = default;

bool CXFA_FMExpExpression::ToJavaScript(CFX_WideTextBuf* js, ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  if (type == ReturnType::kInfered) {
    bool ret = m_pExpression->ToJavaScript(js, ReturnType::kInfered);
    if (m_pExpression->GetOperatorToken() != TOKassign)
      *js << L";\n";

    return ret;
  }

  if (m_pExpression->GetOperatorToken() == TOKassign)
    return m_pExpression->ToJavaScript(js, ReturnType::kImplied);

  if (m_pExpression->GetOperatorToken() == TOKstar ||
      m_pExpression->GetOperatorToken() == TOKdotstar ||
      m_pExpression->GetOperatorToken() == TOKdotscream ||
      m_pExpression->GetOperatorToken() == TOKdotdot ||
      m_pExpression->GetOperatorToken() == TOKdot) {
    *js << L"pfm_ret = pfm_rt.get_val(";
    if (!m_pExpression->ToJavaScript(js, ReturnType::kInfered))
      return false;

    *js << L");\n";
    return !CXFA_IsTooBig(js);
  }

  *js << L"pfm_ret = ";
  if (!m_pExpression->ToJavaScript(js, ReturnType::kInfered))
    return false;

  *js << L";\n";
  return !CXFA_IsTooBig(js);
}

CXFA_FMBlockExpression::CXFA_FMBlockExpression(
    std::vector<std::unique_ptr<CXFA_FMExpression>>&& pExpressionList)
    : CXFA_FMExpression(), m_ExpressionList(std::move(pExpressionList)) {}

CXFA_FMBlockExpression::~CXFA_FMBlockExpression() = default;

bool CXFA_FMBlockExpression::ToJavaScript(CFX_WideTextBuf* js,
                                          ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << L"{\n";
  for (const auto& expr : m_ExpressionList) {
    if (type == ReturnType::kInfered) {
      if (!expr->ToJavaScript(js, ReturnType::kInfered))
        return false;
    } else {
      ReturnType ret_type = expr == m_ExpressionList.back()
                                ? ReturnType::kImplied
                                : ReturnType::kInfered;
      if (!expr->ToJavaScript(js, ret_type))
        return false;
    }
  }
  *js << L"}\n";

  return !CXFA_IsTooBig(js);
}

CXFA_FMDoExpression::CXFA_FMDoExpression(
    std::unique_ptr<CXFA_FMExpression> pList)
    : CXFA_FMExpression(), m_pList(std::move(pList)) {}

CXFA_FMDoExpression::~CXFA_FMDoExpression() = default;

bool CXFA_FMDoExpression::ToJavaScript(CFX_WideTextBuf* js, ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  return m_pList->ToJavaScript(js, type);
}

CXFA_FMIfExpression::CXFA_FMIfExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pExpression,
    std::unique_ptr<CXFA_FMExpression> pIfExpression,
    std::vector<std::unique_ptr<CXFA_FMIfExpression>> pElseIfExpressions,
    std::unique_ptr<CXFA_FMExpression> pElseExpression)
    : CXFA_FMExpression(),
      m_pExpression(std::move(pExpression)),
      m_pIfExpression(std::move(pIfExpression)),
      m_pElseIfExpressions(std::move(pElseIfExpressions)),
      m_pElseExpression(std::move(pElseExpression)) {
  ASSERT(m_pExpression);
}

CXFA_FMIfExpression::~CXFA_FMIfExpression() = default;

bool CXFA_FMIfExpression::ToJavaScript(CFX_WideTextBuf* js, ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  if (type == ReturnType::kImplied)
    *js << L"pfm_ret = 0;\n";

  *js << L"if (pfm_rt.get_val(";
  if (!m_pExpression->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L"))\n";

  if (CXFA_IsTooBig(js))
    return false;

  if (m_pIfExpression) {
    if (!m_pIfExpression->ToJavaScript(js, type))
      return false;
    if (CXFA_IsTooBig(js))
      return false;
  }

  for (auto& expr : m_pElseIfExpressions) {
    *js << L"else ";
    if (!expr->ToJavaScript(js, ReturnType::kInfered))
      return false;
  }

  if (m_pElseExpression) {
    *js << L"else ";
    if (!m_pElseExpression->ToJavaScript(js, type))
      return false;
  }
  return !CXFA_IsTooBig(js);
}

CXFA_FMWhileExpression::CXFA_FMWhileExpression(
    std::unique_ptr<CXFA_FMSimpleExpression> pCondition,
    std::unique_ptr<CXFA_FMExpression> pExpression)
    : CXFA_FMExpression(),
      m_pCondition(std::move(pCondition)),
      m_pExpression(std::move(pExpression)) {}

CXFA_FMWhileExpression::~CXFA_FMWhileExpression() = default;

bool CXFA_FMWhileExpression::ToJavaScript(CFX_WideTextBuf* js,
                                          ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  if (type == ReturnType::kImplied)
    *js << L"pfm_ret = 0;\n";

  *js << L"while (";
  if (!m_pCondition->ToJavaScript(js, ReturnType::kInfered))
    return false;

  *js << L")\n";
  if (CXFA_IsTooBig(js))
    return false;

  if (!m_pExpression->ToJavaScript(js, type))
    return false;

  return !CXFA_IsTooBig(js);
}

CXFA_FMBreakExpression::CXFA_FMBreakExpression() : CXFA_FMExpression() {}

CXFA_FMBreakExpression::~CXFA_FMBreakExpression() = default;

bool CXFA_FMBreakExpression::ToJavaScript(CFX_WideTextBuf* js,
                                          ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << L"pfm_ret = 0;\nbreak;\n";
  return !CXFA_IsTooBig(js);
}

CXFA_FMContinueExpression::CXFA_FMContinueExpression() : CXFA_FMExpression() {}

CXFA_FMContinueExpression::~CXFA_FMContinueExpression() = default;

bool CXFA_FMContinueExpression::ToJavaScript(CFX_WideTextBuf* js,
                                             ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  *js << L"pfm_ret = 0;\ncontinue;\n";
  return !CXFA_IsTooBig(js);
}

CXFA_FMForExpression::CXFA_FMForExpression(
    const WideStringView& wsVariant,
    std::unique_ptr<CXFA_FMSimpleExpression> pAssignment,
    std::unique_ptr<CXFA_FMSimpleExpression> pAccessor,
    int32_t iDirection,
    std::unique_ptr<CXFA_FMSimpleExpression> pStep,
    std::unique_ptr<CXFA_FMExpression> pList)
    : CXFA_FMExpression(),
      m_wsVariant(wsVariant),
      m_pAssignment(std::move(pAssignment)),
      m_pAccessor(std::move(pAccessor)),
      m_bDirection(iDirection == 1),
      m_pStep(std::move(pStep)),
      m_pList(std::move(pList)) {}

CXFA_FMForExpression::~CXFA_FMForExpression() = default;

bool CXFA_FMForExpression::ToJavaScript(CFX_WideTextBuf* js, ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  if (type == ReturnType::kImplied)
    *js << L"pfm_ret = 0;\n";

  *js << L"{\n";

  WideString tmpName = IdentifierToName(m_wsVariant);
  *js << L"var " << tmpName << L" = null;\n";

  *js << L"for (" << tmpName << L" = pfm_rt.get_val(";
  if (!m_pAssignment->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L"); ";

  *js << tmpName << (m_bDirection ? kLessEqual : kGreaterEqual);
  *js << L"pfm_rt.get_val(";
  if (!m_pAccessor->ToJavaScript(js, ReturnType::kInfered))
    return false;
  *js << L"); ";

  *js << tmpName << (m_bDirection ? kPlusEqual : kMinusEqual);
  if (m_pStep) {
    *js << L"pfm_rt.get_val(";
    if (!m_pStep->ToJavaScript(js, ReturnType::kInfered))
      return false;
    *js << L")";
  } else {
    *js << L"1";
  }
  *js << L")\n";
  if (CXFA_IsTooBig(js))
    return false;

  if (!m_pList->ToJavaScript(js, type))
    return false;

  *js << L"}\n";
  return !CXFA_IsTooBig(js);
}

CXFA_FMForeachExpression::CXFA_FMForeachExpression(
    const WideStringView& wsIdentifier,
    std::vector<std::unique_ptr<CXFA_FMSimpleExpression>>&& pAccessors,
    std::unique_ptr<CXFA_FMExpression> pList)
    : CXFA_FMExpression(),
      m_wsIdentifier(wsIdentifier),
      m_pAccessors(std::move(pAccessors)),
      m_pList(std::move(pList)) {}

CXFA_FMForeachExpression::~CXFA_FMForeachExpression() = default;

bool CXFA_FMForeachExpression::ToJavaScript(CFX_WideTextBuf* js,
                                            ReturnType type) {
  CXFA_FMToJavaScriptDepth depthManager;
  if (CXFA_IsTooBig(js) || !depthManager.IsWithinMaxDepth())
    return false;

  if (type == ReturnType::kImplied)
    *js << L"pfm_ret = 0;\n";

  *js << L"{\n";

  WideString tmpName = IdentifierToName(m_wsIdentifier);
  *js << L"var " << tmpName << L" = null;\n";
  *js << L"var pfm_ary = pfm_rt.concat_obj(";
  for (const auto& expr : m_pAccessors) {
    if (!expr->ToJavaScript(js, ReturnType::kInfered))
      return false;
    if (expr != m_pAccessors.back())
      *js << L", ";
  }
  *js << L");\n";

  *js << L"var pfm_ary_idx = 0;\n";
  *js << L"while(pfm_ary_idx < pfm_ary.length)\n{\n";
  *js << tmpName << L" = pfm_ary[pfm_ary_idx++];\n";
  if (!m_pList->ToJavaScript(js, type))
    return false;
  *js << L"}\n";  // while

  *js << L"}\n";  // block
  return !CXFA_IsTooBig(js);
}
