// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_hostpseudomodel.h"

#include <memory>
#include <vector>

#include "fxjs/cfxjse_engine.h"
#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/cxfa_ffdoc.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/parser/cscript_hostpseudomodel.h"
#include "xfa/fxfa/parser/cxfa_layoutprocessor.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/xfa_resolvenode_rs.h"

namespace {

int32_t FilterName(const WideStringView& wsExpression,
                   int32_t nStart,
                   WideString& wsFilter) {
  ASSERT(nStart > -1);
  int32_t iLength = wsExpression.GetLength();
  if (nStart >= iLength)
    return iLength;

  int32_t nCount = 0;
  {
    // Span's lifetime must end before ReleaseBuffer() below.
    pdfium::span<wchar_t> pBuf = wsFilter.GetBuffer(iLength - nStart);
    const wchar_t* pSrc = wsExpression.unterminated_c_str();
    while (nStart < iLength) {
      wchar_t wCur = pSrc[nStart++];
      if (wCur == ',')
        break;

      pBuf[nCount++] = wCur;
    }
  }
  wsFilter.ReleaseBuffer(nCount);
  wsFilter.Trim();
  return nStart;
}

}  // namespace

const CJX_MethodSpec CJX_HostPseudoModel::MethodSpecs[] = {
    {"beep", beep_static},
    {"documentCountInBatch", documentCountInBatch_static},
    {"documentInBatch", documentInBatch_static},
    {"exportData", exportData_static},
    {"getFocus", getFocus_static},
    {"gotoURL", gotoURL_static},
    {"importData", importData_static},
    {"messageBox", messageBox_static},
    {"openList", openList_static},
    {"pageDown", pageDown_static},
    {"pageUp", pageUp_static},
    {"print", print_static},
    {"resetData", resetData_static},
    {"response", response_static},
    {"setFocus", setFocus_static}};

CJX_HostPseudoModel::CJX_HostPseudoModel(CScript_HostPseudoModel* model)
    : CJX_Object(model) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_HostPseudoModel::~CJX_HostPseudoModel() {}

void CJX_HostPseudoModel::appType(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  if (bSetting) {
    ThrowInvalidPropertyException();
    return;
  }
  pValue->SetString("Exchange");
}

void CJX_HostPseudoModel::calculationsEnabled(CFXJSE_Value* pValue,
                                              bool bSetting,
                                              XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  if (bSetting) {
    hDoc->GetDocEnvironment()->SetCalculationsEnabled(hDoc,
                                                      pValue->ToBoolean());
    return;
  }
  pValue->SetBoolean(hDoc->GetDocEnvironment()->IsCalculationsEnabled(hDoc));
}

void CJX_HostPseudoModel::currentPage(CFXJSE_Value* pValue,
                                      bool bSetting,
                                      XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  if (bSetting) {
    hDoc->GetDocEnvironment()->SetCurrentPage(hDoc, pValue->ToInteger());
    return;
  }
  pValue->SetInteger(hDoc->GetDocEnvironment()->GetCurrentPage(hDoc));
}

void CJX_HostPseudoModel::language(CFXJSE_Value* pValue,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  if (bSetting) {
    ThrowException(L"Unable to set language value.");
    return;
  }
  pValue->SetString(
      pNotify->GetAppProvider()->GetLanguage().UTF8Encode().AsStringView());
}

void CJX_HostPseudoModel::numPages(CFXJSE_Value* pValue,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  if (bSetting) {
    ThrowException(L"Unable to set numPages value.");
    return;
  }
  pValue->SetInteger(hDoc->GetDocEnvironment()->CountPages(hDoc));
}

void CJX_HostPseudoModel::platform(CFXJSE_Value* pValue,
                                   bool bSetting,
                                   XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  if (bSetting) {
    ThrowException(L"Unable to set platform value.");
    return;
  }
  pValue->SetString(
      pNotify->GetAppProvider()->GetPlatform().UTF8Encode().AsStringView());
}

void CJX_HostPseudoModel::title(CFXJSE_Value* pValue,
                                bool bSetting,
                                XFA_Attribute eAttribute) {
  if (!GetDocument()->GetScriptContext()->IsRunAtClient())
    return;

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  if (bSetting) {
    hDoc->GetDocEnvironment()->SetTitle(hDoc, pValue->ToWideString());
    return;
  }

  WideString wsTitle;
  hDoc->GetDocEnvironment()->GetTitle(hDoc, wsTitle);
  pValue->SetString(wsTitle.UTF8Encode().AsStringView());
}

void CJX_HostPseudoModel::validationsEnabled(CFXJSE_Value* pValue,
                                             bool bSetting,
                                             XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  if (bSetting) {
    hDoc->GetDocEnvironment()->SetValidationsEnabled(hDoc, pValue->ToBoolean());
    return;
  }

  bool bEnabled = hDoc->GetDocEnvironment()->IsValidationsEnabled(hDoc);
  pValue->SetBoolean(bEnabled);
}

void CJX_HostPseudoModel::variation(CFXJSE_Value* pValue,
                                    bool bSetting,
                                    XFA_Attribute eAttribute) {
  if (!GetDocument()->GetScriptContext()->IsRunAtClient())
    return;

  if (bSetting) {
    ThrowException(L"Unable to set variation value.");
    return;
  }
  pValue->SetString("Full");
}

void CJX_HostPseudoModel::version(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  if (bSetting) {
    ThrowException(L"Unable to set version value.");
    return;
  }
  pValue->SetString("11");
}

void CJX_HostPseudoModel::name(CFXJSE_Value* pValue,
                               bool bSetting,
                               XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;

  if (bSetting) {
    ThrowInvalidPropertyException();
    return;
  }
  pValue->SetString(
      pNotify->GetAppProvider()->GetAppName().UTF8Encode().AsStringView());
}

CJS_Return CJX_HostPseudoModel::gotoURL(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!GetDocument()->GetScriptContext()->IsRunAtClient())
    return CJS_Return(true);

  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  WideString URL = runtime->ToWideString(params[0]);
  hDoc->GetDocEnvironment()->GotoURL(hDoc, URL);
  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::openList(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!GetDocument()->GetScriptContext()->IsRunAtClient())
    return CJS_Return(true);

  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  CXFA_Node* pNode = nullptr;
  if (params[0]->IsObject()) {
    pNode =
        ToNode(static_cast<CFXJSE_Engine*>(runtime)->ToXFAObject(params[0]));
  } else if (params[0]->IsString()) {
    CFXJSE_Engine* pScriptContext = GetDocument()->GetScriptContext();
    if (!pScriptContext)
      return CJS_Return(true);

    CXFA_Object* pObject = pScriptContext->GetThisObject();
    if (!pObject)
      return CJS_Return(true);

    uint32_t dwFlag = XFA_RESOLVENODE_Children | XFA_RESOLVENODE_Parent |
                      XFA_RESOLVENODE_Siblings;
    XFA_RESOLVENODE_RS resolveNodeRS;
    bool iRet = pScriptContext->ResolveObjects(
        pObject, runtime->ToWideString(params[0]).AsStringView(),
        &resolveNodeRS, dwFlag, nullptr);
    if (!iRet || !resolveNodeRS.objects.front()->IsNode())
      return CJS_Return(true);

    pNode = resolveNodeRS.objects.front()->AsNode();
  }

  CXFA_LayoutProcessor* pDocLayout = GetDocument()->GetLayoutProcessor();
  if (!pDocLayout)
    return CJS_Return(true);

  CXFA_FFWidget* hWidget =
      XFA_GetWidgetFromLayoutItem(pDocLayout->GetLayoutItem(pNode));
  if (!hWidget)
    return CJS_Return(true);

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  hDoc->GetDocEnvironment()->SetFocusWidget(hDoc, hWidget);
  pNotify->OpenDropDownList(hWidget);
  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::response(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 4)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  WideString question;
  if (params.size() >= 1)
    question = runtime->ToWideString(params[0]);

  WideString title;
  if (params.size() >= 2)
    title = runtime->ToWideString(params[1]);

  WideString defaultAnswer;
  if (params.size() >= 3)
    defaultAnswer = runtime->ToWideString(params[2]);

  bool mark = false;
  if (params.size() >= 4)
    mark = runtime->ToInt32(params[3]) != 0;

  WideString answer =
      pNotify->GetAppProvider()->Response(question, title, defaultAnswer, mark);
  return CJS_Return(runtime->NewString(answer.UTF8Encode().AsStringView()));
}

CJS_Return CJX_HostPseudoModel::documentInBatch(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(runtime->NewNumber(0));
}

CJS_Return CJX_HostPseudoModel::resetData(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() > 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  WideString expression;
  if (params.size() >= 1)
    expression = runtime->ToWideString(params[0]);

  if (expression.IsEmpty()) {
    pNotify->ResetData(nullptr);
    return CJS_Return(true);
  }

  int32_t iStart = 0;
  WideString wsName;
  CXFA_Node* pNode = nullptr;
  int32_t iExpLength = expression.GetLength();
  while (iStart < iExpLength) {
    iStart = FilterName(expression.AsStringView(), iStart, wsName);
    CFXJSE_Engine* pScriptContext = GetDocument()->GetScriptContext();
    if (!pScriptContext)
      return CJS_Return(true);

    CXFA_Object* pObject = pScriptContext->GetThisObject();
    if (!pObject)
      return CJS_Return(true);

    uint32_t dwFlag = XFA_RESOLVENODE_Children | XFA_RESOLVENODE_Parent |
                      XFA_RESOLVENODE_Siblings;
    XFA_RESOLVENODE_RS resolveNodeRS;
    bool iRet = pScriptContext->ResolveObjects(pObject, wsName.AsStringView(),
                                               &resolveNodeRS, dwFlag, nullptr);
    if (!iRet || !resolveNodeRS.objects.front()->IsNode())
      continue;

    pNode = resolveNodeRS.objects.front()->AsNode();
    pNotify->ResetData(pNode->IsWidgetReady() ? pNode : nullptr);
  }
  if (!pNode)
    pNotify->ResetData(nullptr);

  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::beep(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!GetDocument()->GetScriptContext()->IsRunAtClient())
    return CJS_Return(true);

  if (params.size() > 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  uint32_t dwType = 4;
  if (params.size() >= 1)
    dwType = runtime->ToInt32(params[0]);

  pNotify->GetAppProvider()->Beep(dwType);
  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::setFocus(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!GetDocument()->GetScriptContext()->IsRunAtClient())
    return CJS_Return(true);

  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  CXFA_Node* pNode = nullptr;
  if (params.size() >= 1) {
    if (params[0]->IsObject()) {
      pNode =
          ToNode(static_cast<CFXJSE_Engine*>(runtime)->ToXFAObject(params[0]));
    } else if (params[0]->IsString()) {
      CFXJSE_Engine* pScriptContext = GetDocument()->GetScriptContext();
      if (!pScriptContext)
        return CJS_Return(true);

      CXFA_Object* pObject = pScriptContext->GetThisObject();
      if (!pObject)
        return CJS_Return(true);

      uint32_t dwFlag = XFA_RESOLVENODE_Children | XFA_RESOLVENODE_Parent |
                        XFA_RESOLVENODE_Siblings;
      XFA_RESOLVENODE_RS resolveNodeRS;
      bool iRet = pScriptContext->ResolveObjects(
          pObject, runtime->ToWideString(params[0]).AsStringView(),
          &resolveNodeRS, dwFlag, nullptr);
      if (!iRet || !resolveNodeRS.objects.front()->IsNode())
        return CJS_Return(true);

      pNode = resolveNodeRS.objects.front()->AsNode();
    }
  }
  pNotify->SetFocusWidgetNode(pNode);
  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::getFocus(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  CXFA_Node* pNode = pNotify->GetFocusWidgetNode();
  if (!pNode)
    return CJS_Return(true);

  CFXJSE_Value* value =
      GetDocument()->GetScriptContext()->GetJSValueFromMap(pNode);
  if (!value)
    return CJS_Return(runtime->NewNull());

  return CJS_Return(value->DirectGetValue().Get(runtime->GetIsolate()));
}

CJS_Return CJX_HostPseudoModel::messageBox(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!GetDocument()->GetScriptContext()->IsRunAtClient())
    return CJS_Return(true);

  if (params.empty() || params.size() > 4)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  WideString message;
  if (params.size() >= 1)
    message = runtime->ToWideString(params[0]);

  WideString title;
  if (params.size() >= 2)
    title = runtime->ToWideString(params[1]);

  uint32_t messageType = XFA_MBICON_Error;
  if (params.size() >= 3) {
    messageType = runtime->ToInt32(params[2]);
    if (messageType > XFA_MBICON_Status)
      messageType = XFA_MBICON_Error;
  }

  uint32_t buttonType = XFA_MB_OK;
  if (params.size() >= 4) {
    buttonType = runtime->ToInt32(params[3]);
    if (buttonType > XFA_MB_YesNoCancel)
      buttonType = XFA_MB_OK;
  }

  int32_t iValue = pNotify->GetAppProvider()->MsgBox(message, title,
                                                     messageType, buttonType);
  return CJS_Return(runtime->NewNumber(iValue));
}

CJS_Return CJX_HostPseudoModel::documentCountInBatch(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(runtime->NewNumber(0));
}

CJS_Return CJX_HostPseudoModel::print(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!GetDocument()->GetScriptContext()->IsRunAtClient())
    return CJS_Return(true);

  if (params.size() != 8)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  uint32_t dwOptions = 0;
  if (runtime->ToBoolean(params[0]))
    dwOptions |= XFA_PRINTOPT_ShowDialog;
  if (runtime->ToBoolean(params[3]))
    dwOptions |= XFA_PRINTOPT_CanCancel;
  if (runtime->ToBoolean(params[4]))
    dwOptions |= XFA_PRINTOPT_ShrinkPage;
  if (runtime->ToBoolean(params[5]))
    dwOptions |= XFA_PRINTOPT_AsImage;
  if (runtime->ToBoolean(params[6]))
    dwOptions |= XFA_PRINTOPT_ReverseOrder;
  if (runtime->ToBoolean(params[7]))
    dwOptions |= XFA_PRINTOPT_PrintAnnot;

  int32_t nStartPage = runtime->ToInt32(params[1]);
  int32_t nEndPage = runtime->ToInt32(params[2]);

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  hDoc->GetDocEnvironment()->Print(hDoc, nStartPage, nEndPage, dwOptions);
  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::importData(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::exportData(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  WideString filePath;
  if (params.size() >= 1)
    filePath = runtime->ToWideString(params[0]);

  bool XDP = true;
  if (params.size() >= 2)
    XDP = runtime->ToBoolean(params[1]);

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  hDoc->GetDocEnvironment()->ExportData(hDoc, filePath, XDP);
  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::pageUp(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  int32_t nCurPage = hDoc->GetDocEnvironment()->GetCurrentPage(hDoc);
  int32_t nNewPage = 0;
  if (nCurPage <= 1)
    return CJS_Return(true);

  nNewPage = nCurPage - 1;
  hDoc->GetDocEnvironment()->SetCurrentPage(hDoc, nNewPage);
  return CJS_Return(true);
}

CJS_Return CJX_HostPseudoModel::pageDown(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  CXFA_FFDoc* hDoc = pNotify->GetHDOC();
  int32_t nCurPage = hDoc->GetDocEnvironment()->GetCurrentPage(hDoc);
  int32_t nPageCount = hDoc->GetDocEnvironment()->CountPages(hDoc);
  if (!nPageCount || nCurPage == nPageCount)
    return CJS_Return(true);

  int32_t nNewPage = 0;
  if (nCurPage >= nPageCount)
    nNewPage = nPageCount - 1;
  else
    nNewPage = nCurPage + 1;

  hDoc->GetDocEnvironment()->SetCurrentPage(hDoc, nNewPage);
  return CJS_Return(true);
}
