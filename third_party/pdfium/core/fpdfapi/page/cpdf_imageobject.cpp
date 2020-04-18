// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_imageobject.h"

#include <memory>

#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/parser/cpdf_document.h"

CPDF_ImageObject::CPDF_ImageObject() {}

CPDF_ImageObject::~CPDF_ImageObject() {
  MaybePurgeCache();
}

CPDF_PageObject::Type CPDF_ImageObject::GetType() const {
  return IMAGE;
}

void CPDF_ImageObject::Transform(const CFX_Matrix& matrix) {
  m_Matrix.Concat(matrix);
  CalcBoundingBox();
  SetDirty(true);
}

bool CPDF_ImageObject::IsImage() const {
  return true;
}

CPDF_ImageObject* CPDF_ImageObject::AsImage() {
  return this;
}

const CPDF_ImageObject* CPDF_ImageObject::AsImage() const {
  return this;
}

void CPDF_ImageObject::CalcBoundingBox() {
  std::tie(m_Left, m_Right, m_Top, m_Bottom) =
      m_Matrix.TransformRect(0.f, 1.f, 1.f, 0.f);
}

void CPDF_ImageObject::SetImage(const RetainPtr<CPDF_Image>& pImage) {
  MaybePurgeCache();
  m_pImage = pImage;
}

void CPDF_ImageObject::MaybePurgeCache() {
  if (!m_pImage)
    return;

  CPDF_Document* pDocument = m_pImage->GetDocument();
  if (!pDocument)
    return;

  CPDF_DocPageData* pPageData = pDocument->GetPageData();
  if (!pPageData)
    return;

  CPDF_Stream* pStream = m_pImage->GetStream();
  if (!pStream)
    return;

  uint32_t objnum = pStream->GetObjNum();
  if (!objnum)
    return;

  m_pImage.Reset();  // Clear my reference before asking the cache.
  pPageData->MaybePurgeImage(objnum);
}
