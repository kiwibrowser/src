// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CXFA_LAYOUTCONTEXT_H_
#define XFA_FXFA_PARSER_CXFA_LAYOUTCONTEXT_H_

#include <vector>

class CXFA_ItemLayoutProcess;
class CXFA_Node;

class CXFA_LayoutContext {
 public:
  CXFA_LayoutContext()
      : m_prgSpecifiedColumnWidths(nullptr),
        m_fCurColumnWidth(0),
        m_bCurColumnWidthAvaiable(false),
        m_pOverflowProcessor(nullptr),
        m_pOverflowNode(nullptr) {}
  ~CXFA_LayoutContext() {}

  std::vector<float>* m_prgSpecifiedColumnWidths;
  float m_fCurColumnWidth;
  bool m_bCurColumnWidthAvaiable;
  CXFA_ItemLayoutProcessor* m_pOverflowProcessor;
  CXFA_Node* m_pOverflowNode;
};

#endif  // XFA_FXFA_PARSER_CXFA_LAYOUTCONTEXT_H_
