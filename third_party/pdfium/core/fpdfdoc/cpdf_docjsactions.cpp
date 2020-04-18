// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfdoc/cpdf_docjsactions.h"

#include "core/fpdfdoc/cpdf_nametree.h"

CPDF_DocJSActions::CPDF_DocJSActions(CPDF_Document* pDoc) : m_pDocument(pDoc) {}

CPDF_DocJSActions::~CPDF_DocJSActions() {}

int CPDF_DocJSActions::CountJSActions() const {
  ASSERT(m_pDocument);
  CPDF_NameTree name_tree(m_pDocument.Get(), "JavaScript");
  return name_tree.GetCount();
}

CPDF_Action CPDF_DocJSActions::GetJSActionAndName(int index,
                                                  WideString* csName) const {
  ASSERT(m_pDocument);
  CPDF_NameTree name_tree(m_pDocument.Get(), "JavaScript");
  return CPDF_Action(ToDictionary(name_tree.LookupValueAndName(index, csName)));
}

CPDF_Action CPDF_DocJSActions::GetJSAction(const WideString& csName) const {
  ASSERT(m_pDocument);
  CPDF_NameTree name_tree(m_pDocument.Get(), "JavaScript");
  return CPDF_Action(ToDictionary(name_tree.LookupValue(csName)));
}

int CPDF_DocJSActions::FindJSAction(const WideString& csName) const {
  ASSERT(m_pDocument);
  CPDF_NameTree name_tree(m_pDocument.Get(), "JavaScript");
  return name_tree.GetIndex(csName);
}
