// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "public/fpdf_flatten.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfdoc/cpdf_annot.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "third_party/base/stl_util.h"

enum FPDF_TYPE { MAX, MIN };
enum FPDF_VALUE { TOP, LEFT, RIGHT, BOTTOM };

namespace {

bool IsValidRect(const CFX_FloatRect& rect, const CFX_FloatRect& rcPage) {
  constexpr float kMinSize = 0.000001f;
  if (rect.IsEmpty() || rect.Width() < kMinSize || rect.Height() < kMinSize)
    return false;

  if (rcPage.IsEmpty())
    return true;

  constexpr float kMinBorderSize = 10.000001f;
  return rect.left - rcPage.left >= -kMinBorderSize &&
         rect.right - rcPage.right <= kMinBorderSize &&
         rect.top - rcPage.top <= kMinBorderSize &&
         rect.bottom - rcPage.bottom >= -kMinBorderSize;
}

void GetContentsRect(CPDF_Document* pDoc,
                     CPDF_Dictionary* pDict,
                     std::vector<CFX_FloatRect>* pRectArray) {
  auto pPDFPage = pdfium::MakeUnique<CPDF_Page>(pDoc, pDict, false);
  pPDFPage->ParseContent();

  for (const auto& pPageObject : *pPDFPage->GetPageObjectList()) {
    CFX_FloatRect rc;
    rc.left = pPageObject->m_Left;
    rc.right = pPageObject->m_Right;
    rc.bottom = pPageObject->m_Bottom;
    rc.top = pPageObject->m_Top;
    if (IsValidRect(rc, pDict->GetRectFor("MediaBox")))
      pRectArray->push_back(rc);
  }
}

void ParserStream(CPDF_Dictionary* pPageDic,
                  CPDF_Dictionary* pStream,
                  std::vector<CFX_FloatRect>* pRectArray,
                  std::vector<CPDF_Dictionary*>* pObjectArray) {
  if (!pStream)
    return;
  CFX_FloatRect rect;
  if (pStream->KeyExist("Rect"))
    rect = pStream->GetRectFor("Rect");
  else if (pStream->KeyExist("BBox"))
    rect = pStream->GetRectFor("BBox");

  if (IsValidRect(rect, pPageDic->GetRectFor("MediaBox")))
    pRectArray->push_back(rect);

  pObjectArray->push_back(pStream);
}

int ParserAnnots(CPDF_Document* pSourceDoc,
                 CPDF_Dictionary* pPageDic,
                 std::vector<CFX_FloatRect>* pRectArray,
                 std::vector<CPDF_Dictionary*>* pObjectArray,
                 int nUsage) {
  if (!pSourceDoc || !pPageDic)
    return FLATTEN_FAIL;

  GetContentsRect(pSourceDoc, pPageDic, pRectArray);
  CPDF_Array* pAnnots = pPageDic->GetArrayFor("Annots");
  if (!pAnnots)
    return FLATTEN_NOTHINGTODO;

  for (const auto& pAnnot : *pAnnots) {
    CPDF_Dictionary* pAnnotDic = ToDictionary(pAnnot->GetDirect());
    if (!pAnnotDic)
      continue;

    ByteString sSubtype = pAnnotDic->GetStringFor("Subtype");
    if (sSubtype == "Popup")
      continue;

    int nAnnotFlag = pAnnotDic->GetIntegerFor("F");
    if (nAnnotFlag & ANNOTFLAG_HIDDEN)
      continue;

    bool bParseStream;
    if (nUsage == FLAT_NORMALDISPLAY)
      bParseStream = !(nAnnotFlag & ANNOTFLAG_INVISIBLE);
    else
      bParseStream = !!(nAnnotFlag & ANNOTFLAG_PRINT);
    if (bParseStream)
      ParserStream(pPageDic, pAnnotDic, pRectArray, pObjectArray);
  }
  return FLATTEN_SUCCESS;
}

float GetMinMaxValue(const std::vector<CFX_FloatRect>& array,
                     FPDF_TYPE type,
                     FPDF_VALUE value) {
  if (array.empty())
    return 0.0f;

  size_t nRects = array.size();
  std::vector<float> pArray(nRects);
  switch (value) {
    case LEFT:
      for (size_t i = 0; i < nRects; i++)
        pArray[i] = array[i].left;
      break;
    case TOP:
      for (size_t i = 0; i < nRects; i++)
        pArray[i] = array[i].top;
      break;
    case RIGHT:
      for (size_t i = 0; i < nRects; i++)
        pArray[i] = array[i].right;
      break;
    case BOTTOM:
      for (size_t i = 0; i < nRects; i++)
        pArray[i] = array[i].bottom;
      break;
    default:
      NOTREACHED();
      return 0.0f;
  }

  float fRet = pArray[0];
  if (type == MAX) {
    for (size_t i = 1; i < nRects; i++)
      fRet = std::max(fRet, pArray[i]);
  } else {
    for (size_t i = 1; i < nRects; i++)
      fRet = std::min(fRet, pArray[i]);
  }
  return fRet;
}

CFX_FloatRect CalculateRect(std::vector<CFX_FloatRect>* pRectArray) {
  CFX_FloatRect rcRet;

  rcRet.left = GetMinMaxValue(*pRectArray, MIN, LEFT);
  rcRet.top = GetMinMaxValue(*pRectArray, MAX, TOP);
  rcRet.right = GetMinMaxValue(*pRectArray, MAX, RIGHT);
  rcRet.bottom = GetMinMaxValue(*pRectArray, MIN, BOTTOM);

  return rcRet;
}

uint32_t NewIndirectContentsStream(const ByteString& key,
                                   CPDF_Document* pDocument) {
  CPDF_Stream* pNewContents = pDocument->NewIndirect<CPDF_Stream>(
      nullptr, 0,
      pdfium::MakeUnique<CPDF_Dictionary>(pDocument->GetByteStringPool()));
  ByteString sStream =
      ByteString::Format("q 1 0 0 1 0 0 cm /%s Do Q", key.c_str());
  pNewContents->SetData(sStream.raw_str(), sStream.GetLength());
  return pNewContents->GetObjNum();
}

void SetPageContents(const ByteString& key,
                     CPDF_Dictionary* pPage,
                     CPDF_Document* pDocument) {
  CPDF_Array* pContentsArray = nullptr;
  CPDF_Stream* pContentsStream = pPage->GetStreamFor("Contents");
  if (!pContentsStream) {
    pContentsArray = pPage->GetArrayFor("Contents");
    if (!pContentsArray) {
      if (!key.IsEmpty()) {
        pPage->SetNewFor<CPDF_Reference>(
            "Contents", pDocument, NewIndirectContentsStream(key, pDocument));
      }
      return;
    }
  }
  pPage->ConvertToIndirectObjectFor("Contents", pDocument);
  if (!pContentsArray) {
    pContentsArray = pDocument->NewIndirect<CPDF_Array>();
    auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pContentsStream);
    pAcc->LoadAllDataFiltered();
    ByteString sStream = "q\n";
    ByteString sBody = ByteString(pAcc->GetData(), pAcc->GetSize());
    sStream = sStream + sBody + "\nQ";
    pContentsStream->SetDataAndRemoveFilter(sStream.raw_str(),
                                            sStream.GetLength());
    pContentsArray->AddNew<CPDF_Reference>(pDocument,
                                           pContentsStream->GetObjNum());
    pPage->SetNewFor<CPDF_Reference>("Contents", pDocument,
                                     pContentsArray->GetObjNum());
  }
  if (!key.IsEmpty()) {
    pContentsArray->AddNew<CPDF_Reference>(
        pDocument, NewIndirectContentsStream(key, pDocument));
  }
}

CFX_Matrix GetMatrix(CFX_FloatRect rcAnnot,
                     CFX_FloatRect rcStream,
                     const CFX_Matrix& matrix) {
  if (rcStream.IsEmpty())
    return CFX_Matrix();

  rcStream = matrix.TransformRect(rcStream);
  rcStream.Normalize();

  float a = rcAnnot.Width() / rcStream.Width();
  float d = rcAnnot.Height() / rcStream.Height();

  float e = rcAnnot.left - rcStream.left * a;
  float f = rcAnnot.bottom - rcStream.bottom * d;
  return CFX_Matrix(a, 0, 0, d, e, f);
}

}  // namespace

FPDF_EXPORT int FPDF_CALLCONV FPDFPage_Flatten(FPDF_PAGE page, int nFlag) {
  CPDF_Page* pPage = CPDFPageFromFPDFPage(page);
  if (!page)
    return FLATTEN_FAIL;

  CPDF_Document* pDocument = pPage->GetDocument();
  CPDF_Dictionary* pPageDict = pPage->GetFormDict();
  if (!pDocument || !pPageDict)
    return FLATTEN_FAIL;

  std::vector<CPDF_Dictionary*> ObjectArray;
  std::vector<CFX_FloatRect> RectArray;
  int iRet =
      ParserAnnots(pDocument, pPageDict, &RectArray, &ObjectArray, nFlag);
  if (iRet == FLATTEN_NOTHINGTODO || iRet == FLATTEN_FAIL)
    return iRet;

  CFX_FloatRect rcOriginalCB;
  CFX_FloatRect rcMerger = CalculateRect(&RectArray);
  CFX_FloatRect rcOriginalMB = pPageDict->GetRectFor("MediaBox");
  if (pPageDict->KeyExist("CropBox"))
    rcOriginalMB = pPageDict->GetRectFor("CropBox");

  if (rcOriginalMB.IsEmpty())
    rcOriginalMB = CFX_FloatRect(0.0f, 0.0f, 612.0f, 792.0f);

  rcMerger.left = std::max(rcMerger.left, rcOriginalMB.left);
  rcMerger.right = std::min(rcMerger.right, rcOriginalMB.right);
  rcMerger.bottom = std::max(rcMerger.bottom, rcOriginalMB.bottom);
  rcMerger.top = std::min(rcMerger.top, rcOriginalMB.top);
  if (pPageDict->KeyExist("ArtBox"))
    rcOriginalCB = pPageDict->GetRectFor("ArtBox");
  else
    rcOriginalCB = rcOriginalMB;

  if (!rcOriginalMB.IsEmpty())
    pPageDict->SetRectFor("MediaBox", rcOriginalMB);

  if (!rcOriginalCB.IsEmpty())
    pPageDict->SetRectFor("ArtBox", rcOriginalCB);

  CPDF_Dictionary* pRes = pPageDict->GetDictFor("Resources");
  if (!pRes)
    pRes = pPageDict->SetNewFor<CPDF_Dictionary>("Resources");

  CPDF_Stream* pNewXObject = pDocument->NewIndirect<CPDF_Stream>(
      nullptr, 0,
      pdfium::MakeUnique<CPDF_Dictionary>(pDocument->GetByteStringPool()));

  uint32_t dwObjNum = pNewXObject->GetObjNum();
  CPDF_Dictionary* pPageXObject = pRes->GetDictFor("XObject");
  if (!pPageXObject)
    pPageXObject = pRes->SetNewFor<CPDF_Dictionary>("XObject");

  ByteString key;
  if (!ObjectArray.empty()) {
    int i = 0;
    while (i < INT_MAX) {
      ByteString sKey = ByteString::Format("FFT%d", i);
      if (!pPageXObject->KeyExist(sKey)) {
        key = sKey;
        break;
      }
      ++i;
    }
  }

  SetPageContents(key, pPageDict, pDocument);

  CPDF_Dictionary* pNewXORes = nullptr;
  if (!key.IsEmpty()) {
    pPageXObject->SetNewFor<CPDF_Reference>(key, pDocument, dwObjNum);
    CPDF_Dictionary* pNewOXbjectDic = pNewXObject->GetDict();
    pNewXORes = pNewOXbjectDic->SetNewFor<CPDF_Dictionary>("Resources");
    pNewOXbjectDic->SetNewFor<CPDF_Name>("Type", "XObject");
    pNewOXbjectDic->SetNewFor<CPDF_Name>("Subtype", "Form");
    pNewOXbjectDic->SetNewFor<CPDF_Number>("FormType", 1);
    CFX_FloatRect rcBBox = pPageDict->GetRectFor("ArtBox");
    pNewOXbjectDic->SetRectFor("BBox", rcBBox);
  }

  for (size_t i = 0; i < ObjectArray.size(); ++i) {
    CPDF_Dictionary* pAnnotDic = ObjectArray[i];
    if (!pAnnotDic)
      continue;

    CFX_FloatRect rcAnnot = pAnnotDic->GetRectFor("Rect");
    rcAnnot.Normalize();

    ByteString sAnnotState = pAnnotDic->GetStringFor("AS");
    CPDF_Dictionary* pAnnotAP = pAnnotDic->GetDictFor("AP");
    if (!pAnnotAP)
      continue;

    CPDF_Stream* pAPStream = pAnnotAP->GetStreamFor("N");
    if (!pAPStream) {
      CPDF_Dictionary* pAPDic = pAnnotAP->GetDictFor("N");
      if (!pAPDic)
        continue;

      if (!sAnnotState.IsEmpty()) {
        pAPStream = pAPDic->GetStreamFor(sAnnotState);
      } else {
        if (pAPDic->GetCount() > 0) {
          CPDF_Object* pFirstObj = pAPDic->begin()->second.get();
          if (pFirstObj) {
            if (pFirstObj->IsReference())
              pFirstObj = pFirstObj->GetDirect();
            if (!pFirstObj->IsStream())
              continue;
            pAPStream = pFirstObj->AsStream();
          }
        }
      }
    }
    if (!pAPStream)
      continue;

    CPDF_Dictionary* pAPDic = pAPStream->GetDict();
    CFX_FloatRect rcStream;
    if (pAPDic->KeyExist("Rect"))
      rcStream = pAPDic->GetRectFor("Rect");
    else if (pAPDic->KeyExist("BBox"))
      rcStream = pAPDic->GetRectFor("BBox");

    if (rcStream.IsEmpty())
      continue;

    CPDF_Object* pObj = pAPStream;
    if (pObj->IsInline()) {
      std::unique_ptr<CPDF_Object> pNew = pObj->Clone();
      pObj = pNew.get();
      pDocument->AddIndirectObject(std::move(pNew));
    }

    CPDF_Dictionary* pObjDic = pObj->GetDict();
    if (pObjDic) {
      pObjDic->SetNewFor<CPDF_Name>("Type", "XObject");
      pObjDic->SetNewFor<CPDF_Name>("Subtype", "Form");
    }

    CPDF_Dictionary* pXObject = pNewXORes->GetDictFor("XObject");
    if (!pXObject)
      pXObject = pNewXORes->SetNewFor<CPDF_Dictionary>("XObject");

    ByteString sFormName = ByteString::Format("F%d", i);
    pXObject->SetNewFor<CPDF_Reference>(sFormName, pDocument,
                                        pObj->GetObjNum());

    auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(pNewXObject);
    pAcc->LoadAllDataFiltered();
    ByteString sStream(pAcc->GetData(), pAcc->GetSize());
    CFX_Matrix matrix = pAPDic->GetMatrixFor("Matrix");
    CFX_Matrix m = GetMatrix(rcAnnot, rcStream, matrix);
    sStream += ByteString::Format("q %f 0 0 %f %f %f cm /%s Do Q\n", m.a, m.d,
                                  m.e, m.f, sFormName.c_str());
    pNewXObject->SetDataAndRemoveFilter(sStream.raw_str(), sStream.GetLength());
  }
  pPageDict->RemoveFor("Annots");
  return FLATTEN_SUCCESS;
}
