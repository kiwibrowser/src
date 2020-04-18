// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CXFA_LAYOUTITEM_H_
#define XFA_FXFA_PARSER_CXFA_LAYOUTITEM_H_

#include "xfa/fxfa/parser/cxfa_document.h"

class CXFA_ContainerLayoutItem;
class CXFA_ContentLayoutItem;
class CXFA_LayoutProcessor;

class CXFA_LayoutItem {
 public:
  virtual ~CXFA_LayoutItem();

  bool IsContainerLayoutItem() const { return !m_bIsContentLayoutItem; }
  bool IsContentLayoutItem() const { return m_bIsContentLayoutItem; }
  CXFA_ContainerLayoutItem* AsContainerLayoutItem();
  CXFA_ContentLayoutItem* AsContentLayoutItem();

  CXFA_ContainerLayoutItem* GetPage() const;
  CXFA_Node* GetFormNode() const { return m_pFormNode; }
  CFX_RectF GetRect(bool bRelative) const;

  int32_t GetIndex() const;
  int32_t GetCount() const;

  CXFA_LayoutItem* GetParent() const { return m_pParent; }
  CXFA_LayoutItem* GetFirst();
  const CXFA_LayoutItem* GetLast() const;
  CXFA_LayoutItem* GetPrev() const;
  CXFA_LayoutItem* GetNext() const;

  void AddChild(CXFA_LayoutItem* pChildItem);
  void AddHeadChild(CXFA_LayoutItem* pChildItem);
  void RemoveChild(CXFA_LayoutItem* pChildItem);
  void InsertChild(CXFA_LayoutItem* pBeforeItem, CXFA_LayoutItem* pChildItem);

  CXFA_Node* m_pFormNode;
  CXFA_LayoutItem* m_pParent;
  CXFA_LayoutItem* m_pNextSibling;
  CXFA_LayoutItem* m_pFirstChild;

 protected:
  CXFA_LayoutItem(CXFA_Node* pNode, bool bIsContentLayoutItem);

  bool m_bIsContentLayoutItem;
};

void XFA_ReleaseLayoutItem(CXFA_LayoutItem* pLayoutItem);

#endif  // XFA_FXFA_PARSER_CXFA_LAYOUTITEM_H_
