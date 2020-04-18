// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_LOADERCONTEXT_H_
#define XFA_FXFA_CXFA_LOADERCONTEXT_H_

#include <vector>

#include "core/fxcrt/css/cfx_csscomputedstyle.h"
#include "core/fxcrt/fx_system.h"

class CFX_XMLNode;
class CXFA_Node;

class CXFA_LoaderContext {
 public:
  CXFA_LoaderContext();
  ~CXFA_LoaderContext();

  bool m_bSaveLineHeight;
  float m_fWidth;
  float m_fHeight;
  float m_fLastPos;
  float m_fStartLineOffset;
  int32_t m_iChar;
  int32_t m_iLines;
  int32_t m_iTotalLines;
  uint32_t m_dwFlags;
  CFX_XMLNode* m_pXMLNode;
  CXFA_Node* m_pNode;
  RetainPtr<CFX_CSSComputedStyle> m_pParentStyle;
  std::vector<float> m_lineHeights;
  std::vector<float> m_BlocksHeight;
};

#endif  // XFA_FXFA_CXFA_LOADERCONTEXT_H_
