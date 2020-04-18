// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "public/fpdf_transformpage.h"

#include <memory>
#include <sstream>
#include <vector>

#include "core/fpdfapi/page/cpdf_clippath.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_path.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxge/cfx_pathdata.h"
#include "fpdfsdk/cpdfsdk_helpers.h"

namespace {

void SetBoundingBox(CPDF_Page* page,
                    const ByteString& key,
                    const CFX_FloatRect& rect) {
  page->GetFormDict()->SetRectFor(key, rect);
}

bool GetBoundingBox(CPDF_Page* page,
                    const ByteString& key,
                    float* left,
                    float* bottom,
                    float* right,
                    float* top) {
  CPDF_Array* pArray = page->GetFormDict()->GetArrayFor(key);
  if (!pArray)
    return false;

  *left = pArray->GetFloatAt(0);
  *bottom = pArray->GetFloatAt(1);
  *right = pArray->GetFloatAt(2);
  *top = pArray->GetFloatAt(3);
  return true;
}

CPDF_Object* GetPageContent(CPDF_Dictionary* pPageDict) {
  return pPageDict ? pPageDict->GetDirectObjectFor("Contents") : nullptr;
}

}  // namespace

FPDF_EXPORT void FPDF_CALLCONV FPDFPage_SetMediaBox(FPDF_PAGE page,
                                                    float left,
                                                    float bottom,
                                                    float right,
                                                    float top) {
  CPDF_Page* pPage = CPDFPageFromFPDFPage(page);
  if (!pPage)
    return;

  SetBoundingBox(pPage, "MediaBox", CFX_FloatRect(left, bottom, right, top));
}

FPDF_EXPORT void FPDF_CALLCONV FPDFPage_SetCropBox(FPDF_PAGE page,
                                                   float left,
                                                   float bottom,
                                                   float right,
                                                   float top) {
  CPDF_Page* pPage = CPDFPageFromFPDFPage(page);
  if (!pPage)
    return;

  SetBoundingBox(pPage, "CropBox", CFX_FloatRect(left, bottom, right, top));
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV FPDFPage_GetMediaBox(FPDF_PAGE page,
                                                         float* left,
                                                         float* bottom,
                                                         float* right,
                                                         float* top) {
  CPDF_Page* pPage = CPDFPageFromFPDFPage(page);
  return pPage && GetBoundingBox(pPage, "MediaBox", left, bottom, right, top);
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV FPDFPage_GetCropBox(FPDF_PAGE page,
                                                        float* left,
                                                        float* bottom,
                                                        float* right,
                                                        float* top) {
  CPDF_Page* pPage = CPDFPageFromFPDFPage(page);
  return pPage && GetBoundingBox(pPage, "CropBox", left, bottom, right, top);
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFPage_TransFormWithClip(FPDF_PAGE page,
                           FS_MATRIX* matrix,
                           FS_RECTF* clipRect) {
  if (!matrix && !clipRect)
    return false;

  CPDF_Page* pPage = CPDFPageFromFPDFPage(page);
  if (!pPage)
    return false;

  std::ostringstream textBuf;
  textBuf << "q ";

  if (clipRect) {
    CFX_FloatRect rect = CFXFloatRectFromFSRECTF(*clipRect);
    rect.Normalize();

    textBuf << ByteString::Format("%f %f %f %f re W* n ", rect.left,
                                  rect.bottom, rect.Width(), rect.Height());
  }
  if (matrix) {
    textBuf << ByteString::Format("%f %f %f %f %f %f cm ", matrix->a, matrix->b,
                                  matrix->c, matrix->d, matrix->e, matrix->f);
  }

  CPDF_Dictionary* pPageDict = pPage->GetFormDict();
  CPDF_Object* pContentObj = GetPageContent(pPageDict);
  if (!pContentObj)
    return false;

  CPDF_Document* pDoc = pPage->GetDocument();
  if (!pDoc)
    return false;

  CPDF_Stream* pStream = pDoc->NewIndirect<CPDF_Stream>(
      nullptr, 0,
      pdfium::MakeUnique<CPDF_Dictionary>(pDoc->GetByteStringPool()));
  pStream->SetData(&textBuf);

  CPDF_Stream* pEndStream = pDoc->NewIndirect<CPDF_Stream>(
      nullptr, 0,
      pdfium::MakeUnique<CPDF_Dictionary>(pDoc->GetByteStringPool()));
  pEndStream->SetData((const uint8_t*)" Q", 2);

  if (CPDF_Array* pContentArray = ToArray(pContentObj)) {
    pContentArray->InsertNewAt<CPDF_Reference>(0, pDoc, pStream->GetObjNum());
    pContentArray->AddNew<CPDF_Reference>(pDoc, pEndStream->GetObjNum());
  } else if (pContentObj->IsStream() && !pContentObj->IsInline()) {
    CPDF_Array* pContentArray = pDoc->NewIndirect<CPDF_Array>();
    pContentArray->AddNew<CPDF_Reference>(pDoc, pStream->GetObjNum());
    pContentArray->AddNew<CPDF_Reference>(pDoc, pContentObj->GetObjNum());
    pContentArray->AddNew<CPDF_Reference>(pDoc, pEndStream->GetObjNum());
    pPageDict->SetNewFor<CPDF_Reference>("Contents", pDoc,
                                         pContentArray->GetObjNum());
  }

  // Need to transform the patterns as well.
  CPDF_Dictionary* pRes = pPageDict->GetDictFor("Resources");
  if (pRes) {
    CPDF_Dictionary* pPattenDict = pRes->GetDictFor("Pattern");
    if (pPattenDict) {
      for (const auto& it : *pPattenDict) {
        CPDF_Object* pObj = it.second.get();
        if (pObj->IsReference())
          pObj = pObj->GetDirect();

        CPDF_Dictionary* pDict = nullptr;
        if (pObj->IsDictionary())
          pDict = pObj->AsDictionary();
        else if (CPDF_Stream* pObjStream = pObj->AsStream())
          pDict = pObjStream->GetDict();
        else
          continue;

        CFX_Matrix m = pDict->GetMatrixFor("Matrix");
        CFX_Matrix t = *(CFX_Matrix*)matrix;
        m.Concat(t);
        pDict->SetMatrixFor("Matrix", m);
      }
    }
  }

  return true;
}

FPDF_EXPORT void FPDF_CALLCONV
FPDFPageObj_TransformClipPath(FPDF_PAGEOBJECT page_object,
                              double a,
                              double b,
                              double c,
                              double d,
                              double e,
                              double f) {
  CPDF_PageObject* pPageObj = CPDFPageObjectFromFPDFPageObject(page_object);
  if (!pPageObj)
    return;

  CFX_Matrix matrix((float)a, (float)b, (float)c, (float)d, (float)e, (float)f);

  // Special treatment to shading object, because the ClipPath for shading
  // object is already transformed.
  if (!pPageObj->IsShading())
    pPageObj->TransformClipPath(matrix);
  pPageObj->TransformGeneralState(matrix);
}

FPDF_EXPORT FPDF_CLIPPATH FPDF_CALLCONV FPDF_CreateClipPath(float left,
                                                            float bottom,
                                                            float right,
                                                            float top) {
  CPDF_Path Path;
  Path.AppendRect(left, bottom, right, top);

  auto pNewClipPath = pdfium::MakeUnique<CPDF_ClipPath>();
  pNewClipPath->AppendPath(Path, FXFILL_ALTERNATE, false);

  // Caller takes ownership.
  return FPDFClipPathFromCPDFClipPath(pNewClipPath.release());
}

FPDF_EXPORT void FPDF_CALLCONV FPDF_DestroyClipPath(FPDF_CLIPPATH clipPath) {
  // Take ownership back from caller and destroy.
  std::unique_ptr<CPDF_ClipPath>(CPDFClipPathFromFPDFClipPath(clipPath));
}

void OutputPath(std::ostringstream& buf, CPDF_Path path) {
  const CFX_PathData* pPathData = path.GetObject();
  if (!pPathData)
    return;

  const std::vector<FX_PATHPOINT>& pPoints = pPathData->GetPoints();
  if (path.IsRect()) {
    CFX_PointF diff = pPoints[2].m_Point - pPoints[0].m_Point;
    buf << pPoints[0].m_Point.x << " " << pPoints[0].m_Point.y << " " << diff.x
        << " " << diff.y << " re\n";
    return;
  }

  ByteString temp;
  for (size_t i = 0; i < pPoints.size(); i++) {
    buf << pPoints[i].m_Point.x << " " << pPoints[i].m_Point.y;
    FXPT_TYPE point_type = pPoints[i].m_Type;
    if (point_type == FXPT_TYPE::MoveTo) {
      buf << " m\n";
    } else if (point_type == FXPT_TYPE::BezierTo) {
      buf << " " << pPoints[i + 1].m_Point.x << " " << pPoints[i + 1].m_Point.y
          << " " << pPoints[i + 2].m_Point.x << " " << pPoints[i + 2].m_Point.y;
      buf << " c";
      if (pPoints[i + 2].m_CloseFigure)
        buf << " h";
      buf << "\n";

      i += 2;
    } else if (point_type == FXPT_TYPE::LineTo) {
      buf << " l";
      if (pPoints[i].m_CloseFigure)
        buf << " h";
      buf << "\n";
    }
  }
}

FPDF_EXPORT void FPDF_CALLCONV FPDFPage_InsertClipPath(FPDF_PAGE page,
                                                       FPDF_CLIPPATH clipPath) {
  CPDF_Page* pPage = CPDFPageFromFPDFPage(page);
  if (!pPage)
    return;

  CPDF_Dictionary* pPageDict = pPage->GetFormDict();
  CPDF_Object* pContentObj = GetPageContent(pPageDict);
  if (!pContentObj)
    return;

  std::ostringstream strClip;
  CPDF_ClipPath* pClipPath = CPDFClipPathFromFPDFClipPath(clipPath);
  for (size_t i = 0; i < pClipPath->GetPathCount(); ++i) {
    CPDF_Path path = pClipPath->GetPath(i);
    if (path.GetPoints().empty()) {
      // Empty clipping (totally clipped out)
      strClip << "0 0 m W n ";
    } else {
      OutputPath(strClip, path);
      if (pClipPath->GetClipType(i) == FXFILL_WINDING)
        strClip << "W n\n";
      else
        strClip << "W* n\n";
    }
  }
  CPDF_Document* pDoc = pPage->GetDocument();
  if (!pDoc)
    return;

  CPDF_Stream* pStream = pDoc->NewIndirect<CPDF_Stream>(
      nullptr, 0,
      pdfium::MakeUnique<CPDF_Dictionary>(pDoc->GetByteStringPool()));
  pStream->SetData(&strClip);

  if (CPDF_Array* pArray = ToArray(pContentObj)) {
    pArray->InsertNewAt<CPDF_Reference>(0, pDoc, pStream->GetObjNum());
  } else if (pContentObj->IsStream() && !pContentObj->IsInline()) {
    CPDF_Array* pContentArray = pDoc->NewIndirect<CPDF_Array>();
    pContentArray->AddNew<CPDF_Reference>(pDoc, pStream->GetObjNum());
    pContentArray->AddNew<CPDF_Reference>(pDoc, pContentObj->GetObjNum());
    pPageDict->SetNewFor<CPDF_Reference>("Contents", pDoc,
                                         pContentArray->GetObjNum());
  }
}
