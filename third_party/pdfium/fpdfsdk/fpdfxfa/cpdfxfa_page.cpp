// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/fpdfxfa/cpdfxfa_page.h"

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"
#include "fpdfsdk/fpdfxfa/cxfa_fwladaptertimermgr.h"
#include "public/fpdf_formfill.h"
#include "third_party/base/compiler_specific.h"
#include "third_party/base/ptr_util.h"
#include "xfa/fxfa/cxfa_ffdocview.h"
#include "xfa/fxfa/cxfa_ffpageview.h"

CPDFXFA_Page::CPDFXFA_Page(CPDFXFA_Context* pContext, int page_index)
    : m_pXFAPageView(nullptr), m_pContext(pContext), m_iPageIndex(page_index) {}

CPDFXFA_Page::~CPDFXFA_Page() {}

bool CPDFXFA_Page::LoadPDFPage() {
  if (!m_pContext)
    return false;

  CPDF_Document* pPDFDoc = m_pContext->GetPDFDoc();
  if (!pPDFDoc)
    return false;

  CPDF_Dictionary* pDict = pPDFDoc->GetPageDictionary(m_iPageIndex);
  if (!pDict)
    return false;

  if (!m_pPDFPage || m_pPDFPage->GetFormDict() != pDict) {
    m_pPDFPage = pdfium::MakeUnique<CPDF_Page>(pPDFDoc, pDict, true);
    m_pPDFPage->ParseContent();
  }
  return true;
}

bool CPDFXFA_Page::LoadXFAPageView() {
  if (!m_pContext)
    return false;

  CXFA_FFDoc* pXFADoc = m_pContext->GetXFADoc();
  if (!pXFADoc)
    return false;

  CXFA_FFDocView* pXFADocView = m_pContext->GetXFADocView();
  if (!pXFADocView)
    return false;

  CXFA_FFPageView* pPageView = pXFADocView->GetPageView(m_iPageIndex);
  if (!pPageView)
    return false;

  m_pXFAPageView = pPageView;
  return true;
}

bool CPDFXFA_Page::LoadPage() {
  if (!m_pContext || m_iPageIndex < 0)
    return false;

  switch (m_pContext->GetFormType()) {
    case FormType::kNone:
    case FormType::kAcroForm:
    case FormType::kXFAForeground:
      return LoadPDFPage();
    case FormType::kXFAFull:
      return LoadXFAPageView();
  }
  return false;
}

bool CPDFXFA_Page::LoadPDFPage(CPDF_Dictionary* pageDict) {
  if (!m_pContext || m_iPageIndex < 0 || !pageDict)
    return false;

  m_pPDFPage =
      pdfium::MakeUnique<CPDF_Page>(m_pContext->GetPDFDoc(), pageDict, true);
  m_pPDFPage->ParseContent();
  return true;
}

CPDF_Document::Extension* CPDFXFA_Page::GetDocumentExtension() const {
  return m_pContext.Get();
}

float CPDFXFA_Page::GetPageWidth() const {
  if (!m_pPDFPage && !m_pXFAPageView)
    return 0.0f;

  switch (m_pContext->GetFormType()) {
    case FormType::kNone:
    case FormType::kAcroForm:
    case FormType::kXFAForeground:
      if (m_pPDFPage)
        return m_pPDFPage->GetPageWidth();
      FALLTHROUGH;
    case FormType::kXFAFull:
      if (m_pXFAPageView)
        return m_pXFAPageView->GetPageViewRect().width;
      break;
  }

  return 0.0f;
}

float CPDFXFA_Page::GetPageHeight() const {
  if (!m_pPDFPage && !m_pXFAPageView)
    return 0.0f;

  switch (m_pContext->GetFormType()) {
    case FormType::kNone:
    case FormType::kAcroForm:
    case FormType::kXFAForeground:
      if (m_pPDFPage)
        return m_pPDFPage->GetPageHeight();
      FALLTHROUGH;
    case FormType::kXFAFull:
      if (m_pXFAPageView)
        return m_pXFAPageView->GetPageViewRect().height;
      break;
  }

  return 0.0f;
}

Optional<CFX_PointF> CPDFXFA_Page::DeviceToPage(
    const FX_RECT& rect,
    int rotate,
    const CFX_PointF& device_point) const {
  if (!m_pPDFPage && !m_pXFAPageView)
    return {};

  CFX_PointF pos =
      GetDisplayMatrix(rect, rotate).GetInverse().Transform(device_point);
  return pos;
}

Optional<CFX_PointF> CPDFXFA_Page::PageToDevice(
    const FX_RECT& rect,
    int rotate,
    const CFX_PointF& page_point) const {
  if (!m_pPDFPage && !m_pXFAPageView)
    return {};

  CFX_Matrix page2device = GetDisplayMatrix(rect, rotate);
  return page2device.Transform(page_point);
}

CFX_Matrix CPDFXFA_Page::GetDisplayMatrix(const FX_RECT& rect,
                                          int iRotate) const {
  if (!m_pPDFPage && !m_pXFAPageView)
    return CFX_Matrix();

  switch (m_pContext->GetFormType()) {
    case FormType::kNone:
    case FormType::kAcroForm:
    case FormType::kXFAForeground:
      if (m_pPDFPage)
        return m_pPDFPage->GetDisplayMatrix(rect, iRotate);
      FALLTHROUGH;
    case FormType::kXFAFull:
      if (m_pXFAPageView)
        return m_pXFAPageView->GetDisplayMatrix(rect, iRotate);
      break;
  }

  return CFX_Matrix();
}
