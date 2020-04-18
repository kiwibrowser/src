// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_EDIT_CPDF_PAGECONTENTGENERATOR_H_
#define CORE_FPDFAPI_EDIT_CPDF_PAGECONTENTGENERATOR_H_

#include <sstream>
#include <vector>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/unowned_ptr.h"

class CPDF_Document;
class CPDF_ImageObject;
class CPDF_PageObject;
class CPDF_PageObjectHolder;
class CPDF_PathObject;
class CPDF_TextObject;

class CPDF_PageContentGenerator {
 public:
  explicit CPDF_PageContentGenerator(CPDF_PageObjectHolder* pObjHolder);
  ~CPDF_PageContentGenerator();

  void GenerateContent();
  bool ProcessPageObjects(std::ostringstream* buf);

 private:
  friend class CPDF_PageContentGeneratorTest;

  void ProcessPath(std::ostringstream* buf, CPDF_PathObject* pPathObj);
  void ProcessImage(std::ostringstream* buf, CPDF_ImageObject* pImageObj);
  void ProcessGraphics(std::ostringstream* buf, CPDF_PageObject* pPageObj);
  void ProcessDefaultGraphics(std::ostringstream* buf);
  void ProcessText(std::ostringstream* buf, CPDF_TextObject* pTextObj);
  ByteString RealizeResource(uint32_t dwResourceObjNum,
                             const ByteString& bsType);

  UnownedPtr<CPDF_PageObjectHolder> const m_pObjHolder;
  UnownedPtr<CPDF_Document> const m_pDocument;
  std::vector<UnownedPtr<CPDF_PageObject>> m_pageObjects;
};

#endif  // CORE_FPDFAPI_EDIT_CPDF_PAGECONTENTGENERATOR_H_
