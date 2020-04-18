// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PAGE_CPDF_PAGE_H_
#define CORE_FPDFAPI_PAGE_CPDF_PAGE_H_

#include <memory>

#include "core/fpdfapi/page/cpdf_pageobjectholder.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "third_party/base/optional.h"

class CPDF_Dictionary;
class CPDF_Document;
class CPDF_Object;
class CPDF_PageRenderCache;
class CPDF_PageRenderContext;

class CPDF_Page : public CPDF_PageObjectHolder {
 public:
  class View {};  // Caller implements as desired, empty here due to layering.

  // XFA page parent class, layering.
  class Extension : public Retainable {
    virtual CPDF_Document::Extension* GetDocumentExtension() const = 0;
  };

  CPDF_Page(CPDF_Document* pDocument,
            CPDF_Dictionary* pPageDict,
            bool bPageCache);
  ~CPDF_Page() override;

  // CPDF_PageObjectHolder:
  bool IsPage() const override;

  void ParseContent();

  Optional<CFX_PointF> DeviceToPage(const FX_RECT& rect,
                                    int rotate,
                                    const CFX_PointF& device_point) const;
  Optional<CFX_PointF> PageToDevice(const FX_RECT& rect,
                                    int rotate,
                                    const CFX_PointF& page_point) const;

  CFX_Matrix GetDisplayMatrix(const FX_RECT& rect, int iRotate) const;

  float GetPageWidth() const { return m_PageSize.width; }
  float GetPageHeight() const { return m_PageSize.height; }
  const CFX_SizeF& GetPageSize() const { return m_PageSize; }

  const CFX_FloatRect& GetPageBBox() const { return m_BBox; }
  int GetPageRotation() const;
  CPDF_PageRenderCache* GetRenderCache() const { return m_pPageRender.get(); }

  CPDF_PageRenderContext* GetRenderContext() const {
    return m_pRenderContext.get();
  }
  void SetRenderContext(std::unique_ptr<CPDF_PageRenderContext> pContext);

  CPDF_Document* GetPDFDocument() const { return m_pPDFDocument.Get(); }
  View* GetView() const { return m_pView.Get(); }
  void SetView(View* pView) { m_pView = pView; }
  Extension* GetPageExtension() const { return m_pPageExtension.Get(); }
  void SetPageExtension(Extension* pExt) { m_pPageExtension = pExt; }

 private:
  void StartParse();

  CPDF_Object* GetPageAttr(const ByteString& name) const;
  CFX_FloatRect GetBox(const ByteString& name) const;

  CFX_SizeF m_PageSize;
  CFX_Matrix m_PageMatrix;
  UnownedPtr<CPDF_Document> m_pPDFDocument;
  UnownedPtr<Extension> m_pPageExtension;
  std::unique_ptr<CPDF_PageRenderCache> m_pPageRender;
  std::unique_ptr<CPDF_PageRenderContext> m_pRenderContext;
  UnownedPtr<View> m_pView;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_PAGE_H_
