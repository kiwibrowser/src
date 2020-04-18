// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_form.h"

#include "core/fpdfapi/page/cpdf_contentparser.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_pageobjectholder.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "third_party/base/ptr_util.h"

CPDF_Form::CPDF_Form(CPDF_Document* pDoc,
                     CPDF_Dictionary* pPageResources,
                     CPDF_Stream* pFormStream,
                     CPDF_Dictionary* pParentResources)
    : CPDF_PageObjectHolder(pDoc, pFormStream->GetDict()) {
  m_pFormStream = pFormStream;
  m_pResources = m_pFormDict->GetDictFor("Resources");
  m_pPageResources = pPageResources;
  if (!m_pResources)
    m_pResources = pParentResources;
  if (!m_pResources)
    m_pResources = pPageResources;
  m_Transparency = CPDF_Transparency();
  LoadTransInfo();
}

CPDF_Form::~CPDF_Form() = default;

void CPDF_Form::ParseContent(CPDF_AllStates* pGraphicStates,
                             const CFX_Matrix* pParentMatrix,
                             CPDF_Type3Char* pType3Char,
                             std::set<const uint8_t*>* parsedSet) {
  if (m_ParseState == CONTENT_PARSED || m_ParseState == CONTENT_PARSING)
    return;

  if (!parsedSet) {
    if (!m_ParsedSet)
      m_ParsedSet = pdfium::MakeUnique<std::set<const uint8_t*>>();
    parsedSet = m_ParsedSet.get();
  }

  m_pParser = pdfium::MakeUnique<CPDF_ContentParser>(
      this, pGraphicStates, pParentMatrix, pType3Char, parsedSet);
  m_ParseState = CONTENT_PARSING;

  ContinueParse(nullptr);
}
