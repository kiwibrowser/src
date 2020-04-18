// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_layoutitem.h"

#include "fxjs/xfa/cjx_object.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/parser/cxfa_containerlayoutitem.h"
#include "xfa/fxfa/parser/cxfa_contentlayoutitem.h"
#include "xfa/fxfa/parser/cxfa_margin.h"
#include "xfa/fxfa/parser/cxfa_measurement.h"
#include "xfa/fxfa/parser/cxfa_node.h"

void XFA_ReleaseLayoutItem(CXFA_LayoutItem* pLayoutItem) {
  CXFA_LayoutItem* pNode = pLayoutItem->m_pFirstChild;
  CXFA_FFNotify* pNotify = pLayoutItem->m_pFormNode->GetDocument()->GetNotify();
  CXFA_LayoutProcessor* pDocLayout =
      pLayoutItem->m_pFormNode->GetDocument()->GetLayoutProcessor();
  while (pNode) {
    CXFA_LayoutItem* pNext = pNode->m_pNextSibling;
    pNode->m_pParent = nullptr;
    pNotify->OnLayoutItemRemoving(pDocLayout, pNode);
    XFA_ReleaseLayoutItem(pNode);
    pNode = pNext;
  }
  pNotify->OnLayoutItemRemoving(pDocLayout, pLayoutItem);
  if (pLayoutItem->m_pFormNode->GetElementType() == XFA_Element::PageArea) {
    pNotify->OnPageEvent(static_cast<CXFA_ContainerLayoutItem*>(pLayoutItem),
                         XFA_PAGEVIEWEVENT_PostRemoved);
  }
  delete pLayoutItem;
}

CXFA_LayoutItem::CXFA_LayoutItem(CXFA_Node* pNode, bool bIsContentLayoutItem)
    : m_pFormNode(pNode),
      m_pParent(nullptr),
      m_pNextSibling(nullptr),
      m_pFirstChild(nullptr),
      m_bIsContentLayoutItem(bIsContentLayoutItem) {}

CXFA_LayoutItem::~CXFA_LayoutItem() {}

CXFA_ContainerLayoutItem* CXFA_LayoutItem::AsContainerLayoutItem() {
  return IsContainerLayoutItem() ? static_cast<CXFA_ContainerLayoutItem*>(this)
                                 : nullptr;
}

CXFA_ContentLayoutItem* CXFA_LayoutItem::AsContentLayoutItem() {
  return IsContentLayoutItem() ? static_cast<CXFA_ContentLayoutItem*>(this)
                               : nullptr;
}

CXFA_ContainerLayoutItem* CXFA_LayoutItem::GetPage() const {
  for (CXFA_LayoutItem* pCurNode = const_cast<CXFA_LayoutItem*>(this); pCurNode;
       pCurNode = pCurNode->m_pParent) {
    if (pCurNode->m_pFormNode->GetElementType() == XFA_Element::PageArea)
      return static_cast<CXFA_ContainerLayoutItem*>(pCurNode);
  }
  return nullptr;
}

CFX_RectF CXFA_LayoutItem::GetRect(bool bRelative) const {
  ASSERT(m_bIsContentLayoutItem);

  auto* pThis = static_cast<const CXFA_ContentLayoutItem*>(this);
  CFX_PointF sPos = pThis->m_sPos;
  CFX_SizeF sSize = pThis->m_sSize;
  if (bRelative)
    return CFX_RectF(sPos, sSize);

  for (CXFA_LayoutItem* pLayoutItem = pThis->m_pParent; pLayoutItem;
       pLayoutItem = pLayoutItem->m_pParent) {
    if (CXFA_ContentLayoutItem* pContent = pLayoutItem->AsContentLayoutItem()) {
      sPos += pContent->m_sPos;
      CXFA_Margin* pMarginNode =
          pLayoutItem->m_pFormNode->GetFirstChildByClass<CXFA_Margin>(
              XFA_Element::Margin);
      if (pMarginNode) {
        sPos += CFX_PointF(pMarginNode->JSObject()
                               ->GetMeasure(XFA_Attribute::LeftInset)
                               .ToUnit(XFA_Unit::Pt),
                           pMarginNode->JSObject()
                               ->GetMeasure(XFA_Attribute::TopInset)
                               .ToUnit(XFA_Unit::Pt));
      }
      continue;
    }

    if (pLayoutItem->m_pFormNode->GetElementType() ==
        XFA_Element::ContentArea) {
      sPos += CFX_PointF(pLayoutItem->m_pFormNode->JSObject()
                             ->GetMeasure(XFA_Attribute::X)
                             .ToUnit(XFA_Unit::Pt),
                         pLayoutItem->m_pFormNode->JSObject()
                             ->GetMeasure(XFA_Attribute::Y)
                             .ToUnit(XFA_Unit::Pt));
      break;
    }
    if (pLayoutItem->m_pFormNode->GetElementType() == XFA_Element::PageArea)
      break;
  }
  return CFX_RectF(sPos, sSize);
}

CXFA_LayoutItem* CXFA_LayoutItem::GetFirst() {
  ASSERT(m_bIsContentLayoutItem);
  CXFA_ContentLayoutItem* pCurNode = static_cast<CXFA_ContentLayoutItem*>(this);
  while (pCurNode->m_pPrev)
    pCurNode = pCurNode->m_pPrev;

  return pCurNode;
}

const CXFA_LayoutItem* CXFA_LayoutItem::GetLast() const {
  ASSERT(m_bIsContentLayoutItem);
  const CXFA_ContentLayoutItem* pCurNode =
      static_cast<const CXFA_ContentLayoutItem*>(this);
  while (pCurNode->m_pNext)
    pCurNode = pCurNode->m_pNext;

  return pCurNode;
}

CXFA_LayoutItem* CXFA_LayoutItem::GetPrev() const {
  ASSERT(m_bIsContentLayoutItem);

  return static_cast<const CXFA_ContentLayoutItem*>(this)->m_pPrev;
}

CXFA_LayoutItem* CXFA_LayoutItem::GetNext() const {
  ASSERT(m_bIsContentLayoutItem);
  return static_cast<const CXFA_ContentLayoutItem*>(this)->m_pNext;
}

int32_t CXFA_LayoutItem::GetIndex() const {
  ASSERT(m_bIsContentLayoutItem);
  int32_t iIndex = 0;
  const CXFA_ContentLayoutItem* pCurNode =
      static_cast<const CXFA_ContentLayoutItem*>(this);
  while (pCurNode->m_pPrev) {
    pCurNode = pCurNode->m_pPrev;
    ++iIndex;
  }
  return iIndex;
}

int32_t CXFA_LayoutItem::GetCount() const {
  ASSERT(m_bIsContentLayoutItem);

  int32_t iCount = GetIndex() + 1;
  const CXFA_ContentLayoutItem* pCurNode =
      static_cast<const CXFA_ContentLayoutItem*>(this);
  while (pCurNode->m_pNext) {
    pCurNode = pCurNode->m_pNext;
    iCount++;
  }
  return iCount;
}

void CXFA_LayoutItem::AddChild(CXFA_LayoutItem* pChildItem) {
  if (pChildItem->m_pParent)
    pChildItem->m_pParent->RemoveChild(pChildItem);

  pChildItem->m_pParent = this;
  if (!m_pFirstChild) {
    m_pFirstChild = pChildItem;
    return;
  }

  CXFA_LayoutItem* pExistingChildItem = m_pFirstChild;
  while (pExistingChildItem->m_pNextSibling)
    pExistingChildItem = pExistingChildItem->m_pNextSibling;

  pExistingChildItem->m_pNextSibling = pChildItem;
}

void CXFA_LayoutItem::AddHeadChild(CXFA_LayoutItem* pChildItem) {
  if (pChildItem->m_pParent)
    pChildItem->m_pParent->RemoveChild(pChildItem);

  pChildItem->m_pParent = this;
  if (!m_pFirstChild) {
    m_pFirstChild = pChildItem;
    return;
  }

  CXFA_LayoutItem* pExistingChildItem = m_pFirstChild;
  m_pFirstChild = pChildItem;
  m_pFirstChild->m_pNextSibling = pExistingChildItem;
}

void CXFA_LayoutItem::InsertChild(CXFA_LayoutItem* pBeforeItem,
                                  CXFA_LayoutItem* pChildItem) {
  if (pBeforeItem->m_pParent != this)
    return;
  if (pChildItem->m_pParent)
    pChildItem->m_pParent = nullptr;

  pChildItem->m_pParent = this;

  CXFA_LayoutItem* pExistingChildItem = pBeforeItem->m_pNextSibling;
  pBeforeItem->m_pNextSibling = pChildItem;
  pChildItem->m_pNextSibling = pExistingChildItem;
}

void CXFA_LayoutItem::RemoveChild(CXFA_LayoutItem* pChildItem) {
  if (pChildItem->m_pParent != this)
    return;

  if (m_pFirstChild == pChildItem) {
    m_pFirstChild = pChildItem->m_pNextSibling;
  } else {
    CXFA_LayoutItem* pExistingChildItem = m_pFirstChild;
    while (pExistingChildItem &&
           pExistingChildItem->m_pNextSibling != pChildItem) {
      pExistingChildItem = pExistingChildItem->m_pNextSibling;
    }
    if (pExistingChildItem)
      pExistingChildItem->m_pNextSibling = pChildItem->m_pNextSibling;
  }
  pChildItem->m_pNextSibling = nullptr;
  pChildItem->m_pParent = nullptr;
}
