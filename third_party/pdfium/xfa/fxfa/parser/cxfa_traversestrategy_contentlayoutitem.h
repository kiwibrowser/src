// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CXFA_TRAVERSESTRATEGY_CONTENTLAYOUTITEM_H_
#define XFA_FXFA_PARSER_CXFA_TRAVERSESTRATEGY_CONTENTLAYOUTITEM_H_

#include "xfa/fxfa/parser/cxfa_contentlayoutitem.h"

class CXFA_TraverseStrategy_ContentLayoutItem {
 public:
  static CXFA_ContentLayoutItem* GetFirstChild(
      CXFA_ContentLayoutItem* pLayoutItem) {
    return static_cast<CXFA_ContentLayoutItem*>(pLayoutItem->m_pFirstChild);
  }

  static CXFA_ContentLayoutItem* GetNextSibling(
      CXFA_ContentLayoutItem* pLayoutItem) {
    return static_cast<CXFA_ContentLayoutItem*>(pLayoutItem->m_pNextSibling);
  }

  static CXFA_ContentLayoutItem* GetParent(
      CXFA_ContentLayoutItem* pLayoutItem) {
    return static_cast<CXFA_ContentLayoutItem*>(pLayoutItem->m_pParent);
  }
};

#endif  // XFA_FXFA_PARSER_CXFA_TRAVERSESTRATEGY_CONTENTLAYOUTITEM_H_
