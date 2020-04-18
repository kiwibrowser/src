// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CXFA_CONTENTLAYOUTITEM_H_
#define XFA_FXFA_PARSER_CXFA_CONTENTLAYOUTITEM_H_

#include "xfa/fxfa/parser/cxfa_layoutitem.h"

class CXFA_ContentLayoutItem : public CXFA_LayoutItem {
 public:
  explicit CXFA_ContentLayoutItem(CXFA_Node* pNode);
  ~CXFA_ContentLayoutItem() override;

  CXFA_ContentLayoutItem* m_pPrev;
  CXFA_ContentLayoutItem* m_pNext;
  CFX_PointF m_sPos;
  CFX_SizeF m_sSize;
  mutable uint32_t m_dwStatus;
};

inline CXFA_ContentLayoutItem* ToContentLayoutItem(CXFA_LayoutItem* pItem) {
  return pItem ? pItem->AsContentLayoutItem() : nullptr;
}

#endif  // XFA_FXFA_PARSER_CXFA_CONTENTLAYOUTITEM_H_
