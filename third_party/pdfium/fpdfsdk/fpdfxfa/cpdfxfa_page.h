// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FPDFXFA_CPDFXFA_PAGE_H_
#define FPDFSDK_FPDFXFA_CPDFXFA_PAGE_H_

#include <memory>

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "third_party/base/optional.h"

class CPDF_Dictionary;
class CPDFXFA_Context;
class CXFA_FFPageView;

class CPDFXFA_Page : public CPDF_Page::Extension {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  bool LoadPage();
  bool LoadPDFPage(CPDF_Dictionary* pageDict);

  // CPDF_Page::Extension:
  CPDF_Document::Extension* GetDocumentExtension() const override;

  int GetPageIndex() const { return m_iPageIndex; }
  CPDF_Page* GetPDFPage() const { return m_pPDFPage.get(); }
  CXFA_FFPageView* GetXFAPageView() const { return m_pXFAPageView; }

  void SetXFAPageView(CXFA_FFPageView* pPageView) {
    m_pXFAPageView = pPageView;
  }

  float GetPageWidth() const;
  float GetPageHeight() const;

  Optional<CFX_PointF> DeviceToPage(const FX_RECT& rect,
                                    int rotate,
                                    const CFX_PointF& device_point) const;
  Optional<CFX_PointF> PageToDevice(const FX_RECT& rect,
                                    int rotate,
                                    const CFX_PointF& page_point) const;

  CFX_Matrix GetDisplayMatrix(const FX_RECT& rect, int iRotate) const;

 protected:
  // Refcounted class.
  CPDFXFA_Page(CPDFXFA_Context* pContext, int page_index);
  ~CPDFXFA_Page() override;

  bool LoadPDFPage();
  bool LoadXFAPageView();

 private:
  std::unique_ptr<CPDF_Page> m_pPDFPage;
  CXFA_FFPageView* m_pXFAPageView;
  UnownedPtr<CPDFXFA_Context> const m_pContext;
  const int m_iPageIndex;
};

#endif  // FPDFSDK_FPDFXFA_CPDFXFA_PAGE_H_
