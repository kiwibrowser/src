// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cjx_layoutpseudomodel.h"

#include <set>

#include "core/fxcrt/fx_coordinates.h"
#include "fxjs/cfxjse_engine.h"
#include "fxjs/cfxjse_value.h"
#include "fxjs/js_resources.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/parser/cscript_layoutpseudomodel.h"
#include "xfa/fxfa/parser/cxfa_arraynodelist.h"
#include "xfa/fxfa/parser/cxfa_containerlayoutitem.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_form.h"
#include "xfa/fxfa/parser/cxfa_layoutitem.h"
#include "xfa/fxfa/parser/cxfa_layoutprocessor.h"
#include "xfa/fxfa/parser/cxfa_measurement.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_nodeiteratortemplate.h"
#include "xfa/fxfa/parser/cxfa_traversestrategy_contentlayoutitem.h"

const CJX_MethodSpec CJX_LayoutPseudoModel::MethodSpecs[] = {
    {"absPage", absPage_static},
    {"absPageCount", absPageCount_static},
    {"absPageCountInBatch", absPageCountInBatch_static},
    {"absPageInBatch", absPageInBatch_static},
    {"absPageSpan", absPageSpan_static},
    {"h", h_static},
    {"page", page_static},
    {"pageContent", pageContent_static},
    {"pageCount", pageCount_static},
    {"pageSpan", pageSpan_static},
    {"relayout", relayout_static},
    {"relayoutPageArea", relayoutPageArea_static},
    {"sheet", sheet_static},
    {"sheetCount", sheetCount_static},
    {"sheetCountInBatch", sheetCountInBatch_static},
    {"sheetInBatch", sheetInBatch_static},
    {"w", w_static},
    {"x", x_static},
    {"y", y_static}};

CJX_LayoutPseudoModel::CJX_LayoutPseudoModel(CScript_LayoutPseudoModel* model)
    : CJX_Object(model) {
  DefineMethods(MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJX_LayoutPseudoModel::~CJX_LayoutPseudoModel() {}

void CJX_LayoutPseudoModel::ready(CFXJSE_Value* pValue,
                                  bool bSetting,
                                  XFA_Attribute eAttribute) {
  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return;
  if (bSetting) {
    ThrowException(L"Unable to set ready value.");
    return;
  }

  int32_t iStatus = pNotify->GetLayoutStatus();
  pValue->SetBoolean(iStatus >= 2);
}

CJS_Return CJX_LayoutPseudoModel::HWXY(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params,
    XFA_LAYOUTMODEL_HWXY layoutModel) {
  if (params.empty() || params.size() > 3)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* pNode =
      ToNode(static_cast<CFXJSE_Engine*>(runtime)->ToXFAObject(params[0]));
  if (!pNode)
    return CJS_Return(true);

  WideString unit(L"pt");
  if (params.size() >= 2) {
    WideString tmp_unit = runtime->ToWideString(params[1]);
    if (!tmp_unit.IsEmpty())
      unit = tmp_unit;
  }
  int32_t iIndex = params.size() >= 3 ? runtime->ToInt32(params[2]) : 0;

  CXFA_LayoutProcessor* pDocLayout = GetDocument()->GetLayoutProcessor();
  if (!pDocLayout)
    return CJS_Return(true);

  CXFA_LayoutItem* pLayoutItem = pDocLayout->GetLayoutItem(pNode);
  if (!pLayoutItem)
    return CJS_Return(true);

  while (iIndex > 0 && pLayoutItem) {
    pLayoutItem = pLayoutItem->GetNext();
    --iIndex;
  }

  if (!pLayoutItem)
    return CJS_Return(runtime->NewNumber(0.0));

  CXFA_Measurement measure;
  CFX_RectF rtRect = pLayoutItem->GetRect(true);
  switch (layoutModel) {
    case XFA_LAYOUTMODEL_H:
      measure.Set(rtRect.height, XFA_Unit::Pt);
      break;
    case XFA_LAYOUTMODEL_W:
      measure.Set(rtRect.width, XFA_Unit::Pt);
      break;
    case XFA_LAYOUTMODEL_X:
      measure.Set(rtRect.left, XFA_Unit::Pt);
      break;
    case XFA_LAYOUTMODEL_Y:
      measure.Set(rtRect.top, XFA_Unit::Pt);
      break;
  }

  float fValue =
      measure.ToUnit(CXFA_Measurement::GetUnitFromString(unit.AsStringView()));
  return CJS_Return(runtime->NewNumber(FXSYS_round(fValue * 1000) / 1000.0f));
}

CJS_Return CJX_LayoutPseudoModel::h(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return HWXY(runtime, params, XFA_LAYOUTMODEL_H);
}

CJS_Return CJX_LayoutPseudoModel::w(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return HWXY(runtime, params, XFA_LAYOUTMODEL_W);
}

CJS_Return CJX_LayoutPseudoModel::x(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return HWXY(runtime, params, XFA_LAYOUTMODEL_X);
}

CJS_Return CJX_LayoutPseudoModel::y(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return HWXY(runtime, params, XFA_LAYOUTMODEL_Y);
}

CJS_Return CJX_LayoutPseudoModel::NumberedPageCount(CFX_V8* runtime,
                                                    bool bNumbered) {
  CXFA_LayoutProcessor* pDocLayout = GetDocument()->GetLayoutProcessor();
  if (!pDocLayout)
    return CJS_Return(true);

  int32_t iPageCount = 0;
  int32_t iPageNum = pDocLayout->CountPages();
  if (bNumbered) {
    for (int32_t i = 0; i < iPageNum; i++) {
      CXFA_ContainerLayoutItem* pLayoutPage = pDocLayout->GetPage(i);
      if (!pLayoutPage)
        continue;

      CXFA_Node* pMasterPage = pLayoutPage->GetMasterPage();
      if (pMasterPage->JSObject()->GetInteger(XFA_Attribute::Numbered))
        iPageCount++;
    }
  } else {
    iPageCount = iPageNum;
  }
  return CJS_Return(runtime->NewNumber(iPageCount));
}

CJS_Return CJX_LayoutPseudoModel::pageCount(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return NumberedPageCount(runtime, true);
}

CJS_Return CJX_LayoutPseudoModel::pageSpan(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* pNode =
      ToNode(static_cast<CFXJSE_Engine*>(runtime)->ToXFAObject(params[0]));
  if (!pNode)
    return CJS_Return(true);

  CXFA_LayoutProcessor* pDocLayout = GetDocument()->GetLayoutProcessor();
  if (!pDocLayout)
    return CJS_Return(true);

  CXFA_LayoutItem* pLayoutItem = pDocLayout->GetLayoutItem(pNode);
  if (!pLayoutItem)
    return CJS_Return(runtime->NewNumber(-1));

  int32_t iLast = pLayoutItem->GetLast()->GetPage()->GetPageIndex();
  int32_t iFirst = pLayoutItem->GetFirst()->GetPage()->GetPageIndex();
  int32_t iPageSpan = iLast - iFirst + 1;
  return CJS_Return(runtime->NewNumber(iPageSpan));
}

CJS_Return CJX_LayoutPseudoModel::page(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return PageInternals(runtime, params, false);
}

std::vector<CXFA_Node*> CJX_LayoutPseudoModel::GetObjArray(
    CXFA_LayoutProcessor* pDocLayout,
    int32_t iPageNo,
    const WideString& wsType,
    bool bOnPageArea) {
  CXFA_ContainerLayoutItem* pLayoutPage = pDocLayout->GetPage(iPageNo);
  if (!pLayoutPage)
    return std::vector<CXFA_Node*>();

  std::vector<CXFA_Node*> retArray;
  if (wsType == L"pageArea") {
    if (pLayoutPage->m_pFormNode)
      retArray.push_back(pLayoutPage->m_pFormNode);
    return retArray;
  }
  if (wsType == L"contentArea") {
    for (CXFA_LayoutItem* pItem = pLayoutPage->m_pFirstChild; pItem;
         pItem = pItem->m_pNextSibling) {
      if (pItem->m_pFormNode->GetElementType() == XFA_Element::ContentArea)
        retArray.push_back(pItem->m_pFormNode);
    }
    return retArray;
  }
  std::set<CXFA_Node*> formItems;
  if (wsType.IsEmpty()) {
    if (pLayoutPage->m_pFormNode)
      retArray.push_back(pLayoutPage->m_pFormNode);

    for (CXFA_LayoutItem* pItem = pLayoutPage->m_pFirstChild; pItem;
         pItem = pItem->m_pNextSibling) {
      if (pItem->m_pFormNode->GetElementType() == XFA_Element::ContentArea) {
        retArray.push_back(pItem->m_pFormNode);
        if (!bOnPageArea) {
          CXFA_NodeIteratorTemplate<CXFA_ContentLayoutItem,
                                    CXFA_TraverseStrategy_ContentLayoutItem>
          iterator(static_cast<CXFA_ContentLayoutItem*>(pItem->m_pFirstChild));
          for (CXFA_ContentLayoutItem* pItemChild = iterator.GetCurrent();
               pItemChild; pItemChild = iterator.MoveToNext()) {
            if (!pItemChild->IsContentLayoutItem()) {
              continue;
            }
            XFA_Element eType = pItemChild->m_pFormNode->GetElementType();
            if (eType != XFA_Element::Field && eType != XFA_Element::Draw &&
                eType != XFA_Element::Subform && eType != XFA_Element::Area) {
              continue;
            }
            if (pdfium::ContainsValue(formItems, pItemChild->m_pFormNode))
              continue;

            formItems.insert(pItemChild->m_pFormNode);
            retArray.push_back(pItemChild->m_pFormNode);
          }
        }
      } else {
        if (bOnPageArea) {
          CXFA_NodeIteratorTemplate<CXFA_ContentLayoutItem,
                                    CXFA_TraverseStrategy_ContentLayoutItem>
          iterator(static_cast<CXFA_ContentLayoutItem*>(pItem));
          for (CXFA_ContentLayoutItem* pItemChild = iterator.GetCurrent();
               pItemChild; pItemChild = iterator.MoveToNext()) {
            if (!pItemChild->IsContentLayoutItem()) {
              continue;
            }
            XFA_Element eType = pItemChild->m_pFormNode->GetElementType();
            if (eType != XFA_Element::Field && eType != XFA_Element::Draw &&
                eType != XFA_Element::Subform && eType != XFA_Element::Area) {
              continue;
            }
            if (pdfium::ContainsValue(formItems, pItemChild->m_pFormNode))
              continue;
            formItems.insert(pItemChild->m_pFormNode);
            retArray.push_back(pItemChild->m_pFormNode);
          }
        }
      }
    }
    return retArray;
  }

  XFA_Element eType = XFA_Element::Unknown;
  if (wsType == L"field")
    eType = XFA_Element::Field;
  else if (wsType == L"draw")
    eType = XFA_Element::Draw;
  else if (wsType == L"subform")
    eType = XFA_Element::Subform;
  else if (wsType == L"area")
    eType = XFA_Element::Area;

  if (eType != XFA_Element::Unknown) {
    for (CXFA_LayoutItem* pItem = pLayoutPage->m_pFirstChild; pItem;
         pItem = pItem->m_pNextSibling) {
      if (pItem->m_pFormNode->GetElementType() == XFA_Element::ContentArea) {
        if (!bOnPageArea) {
          CXFA_NodeIteratorTemplate<CXFA_ContentLayoutItem,
                                    CXFA_TraverseStrategy_ContentLayoutItem>
          iterator(static_cast<CXFA_ContentLayoutItem*>(pItem->m_pFirstChild));
          for (CXFA_ContentLayoutItem* pItemChild = iterator.GetCurrent();
               pItemChild; pItemChild = iterator.MoveToNext()) {
            if (!pItemChild->IsContentLayoutItem())
              continue;
            if (pItemChild->m_pFormNode->GetElementType() != eType)
              continue;
            if (pdfium::ContainsValue(formItems, pItemChild->m_pFormNode))
              continue;

            formItems.insert(pItemChild->m_pFormNode);
            retArray.push_back(pItemChild->m_pFormNode);
          }
        }
      } else {
        if (bOnPageArea) {
          CXFA_NodeIteratorTemplate<CXFA_ContentLayoutItem,
                                    CXFA_TraverseStrategy_ContentLayoutItem>
          iterator(static_cast<CXFA_ContentLayoutItem*>(pItem));
          for (CXFA_ContentLayoutItem* pItemChild = iterator.GetCurrent();
               pItemChild; pItemChild = iterator.MoveToNext()) {
            if (!pItemChild->IsContentLayoutItem())
              continue;
            if (pItemChild->m_pFormNode->GetElementType() != eType)
              continue;
            if (pdfium::ContainsValue(formItems, pItemChild->m_pFormNode))
              continue;

            formItems.insert(pItemChild->m_pFormNode);
            retArray.push_back(pItemChild->m_pFormNode);
          }
        }
      }
    }
  }
  return retArray;
}

CJS_Return CJX_LayoutPseudoModel::pageContent(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty() || params.size() > 3)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  int32_t iIndex = 0;
  if (params.size() >= 1)
    iIndex = runtime->ToInt32(params[0]);

  WideString wsType;
  if (params.size() >= 2)
    wsType = runtime->ToWideString(params[1]);

  bool bOnPageArea = false;
  if (params.size() >= 3)
    bOnPageArea = runtime->ToBoolean(params[2]);

  CXFA_FFNotify* pNotify = GetDocument()->GetNotify();
  if (!pNotify)
    return CJS_Return(true);

  CXFA_LayoutProcessor* pDocLayout = GetDocument()->GetLayoutProcessor();
  if (!pDocLayout)
    return CJS_Return(true);

  auto pArrayNodeList = pdfium::MakeUnique<CXFA_ArrayNodeList>(GetDocument());
  pArrayNodeList->SetArrayNodeList(
      GetObjArray(pDocLayout, iIndex, wsType, bOnPageArea));

  // TODO(dsinclair): Who owns the array once we release it? Won't this leak?
  return CJS_Return(static_cast<CFXJSE_Engine*>(runtime)->NewXFAObject(
      pArrayNodeList.release(),
      GetDocument()->GetScriptContext()->GetJseNormalClass()->GetTemplate()));
}

CJS_Return CJX_LayoutPseudoModel::absPageCount(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return NumberedPageCount(runtime, false);
}

CJS_Return CJX_LayoutPseudoModel::absPageCountInBatch(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(runtime->NewNumber(0));
}

CJS_Return CJX_LayoutPseudoModel::sheetCountInBatch(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(runtime->NewNumber(0));
}

CJS_Return CJX_LayoutPseudoModel::relayout(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  CXFA_Node* pRootNode = GetDocument()->GetRoot();
  CXFA_Form* pFormRoot =
      pRootNode->GetFirstChildByClass<CXFA_Form>(XFA_Element::Form);
  CXFA_Node* pContentRootNode = pFormRoot->GetFirstChild();
  CXFA_LayoutProcessor* pLayoutProcessor = GetDocument()->GetLayoutProcessor();
  if (pContentRootNode)
    pLayoutProcessor->AddChangedContainer(pContentRootNode);

  pLayoutProcessor->SetForceReLayout(true);
  return CJS_Return(true);
}

CJS_Return CJX_LayoutPseudoModel::absPageSpan(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return pageSpan(runtime, params);
}

CJS_Return CJX_LayoutPseudoModel::absPageInBatch(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewNumber(0));
}

CJS_Return CJX_LayoutPseudoModel::sheetInBatch(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  return CJS_Return(runtime->NewNumber(0));
}

CJS_Return CJX_LayoutPseudoModel::sheet(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return PageInternals(runtime, params, true);
}

CJS_Return CJX_LayoutPseudoModel::relayoutPageArea(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJX_LayoutPseudoModel::sheetCount(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return NumberedPageCount(runtime, false);
}

CJS_Return CJX_LayoutPseudoModel::absPage(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return PageInternals(runtime, params, true);
}

CJS_Return CJX_LayoutPseudoModel::PageInternals(
    CFX_V8* runtime,
    const std::vector<v8::Local<v8::Value>>& params,
    bool bAbsPage) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  CXFA_Node* pNode =
      ToNode(static_cast<CFXJSE_Engine*>(runtime)->ToXFAObject(params[0]));
  if (!pNode)
    return CJS_Return(runtime->NewNumber(0));

  CXFA_LayoutProcessor* pDocLayout = GetDocument()->GetLayoutProcessor();
  if (!pDocLayout)
    return CJS_Return(true);

  CXFA_LayoutItem* pLayoutItem = pDocLayout->GetLayoutItem(pNode);
  if (!pLayoutItem)
    return CJS_Return(runtime->NewNumber(-1));

  int32_t iPage = pLayoutItem->GetFirst()->GetPage()->GetPageIndex();
  return CJS_Return(runtime->NewNumber(bAbsPage ? iPage : iPage + 1));
}
