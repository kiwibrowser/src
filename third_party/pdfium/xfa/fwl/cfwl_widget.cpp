// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/cfwl_widget.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "third_party/base/stl_util.h"
#include "xfa/fde/cfde_textout.h"
#include "xfa/fwl/cfwl_app.h"
#include "xfa/fwl/cfwl_combobox.h"
#include "xfa/fwl/cfwl_event.h"
#include "xfa/fwl/cfwl_eventmouse.h"
#include "xfa/fwl/cfwl_form.h"
#include "xfa/fwl/cfwl_messagekey.h"
#include "xfa/fwl/cfwl_messagekillfocus.h"
#include "xfa/fwl/cfwl_messagemouse.h"
#include "xfa/fwl/cfwl_messagemousewheel.h"
#include "xfa/fwl/cfwl_messagesetfocus.h"
#include "xfa/fwl/cfwl_notedriver.h"
#include "xfa/fwl/cfwl_themebackground.h"
#include "xfa/fwl/cfwl_themepart.h"
#include "xfa/fwl/cfwl_themetext.h"
#include "xfa/fwl/cfwl_widgetmgr.h"
#include "xfa/fwl/ifwl_themeprovider.h"
#include "xfa/fxfa/cxfa_ffapp.h"

#define FWL_WGT_CalcHeight 2048
#define FWL_WGT_CalcWidth 2048
#define FWL_WGT_CalcMultiLineDefWidth 120.0f

CFWL_Widget::CFWL_Widget(const CFWL_App* app,
                         std::unique_ptr<CFWL_WidgetProperties> properties,
                         CFWL_Widget* pOuter)
    : m_pOwnerApp(app),
      m_pWidgetMgr(app->GetWidgetMgr()),
      m_pProperties(std::move(properties)),
      m_pOuter(pOuter),
      m_iLock(0),
      m_pLayoutItem(nullptr),
      m_nEventKey(0),
      m_pDelegate(nullptr) {
  ASSERT(m_pWidgetMgr);

  CFWL_Widget* pParent = m_pProperties->m_pParent;
  m_pWidgetMgr->InsertWidget(pParent, this);
  if (IsChild())
    return;

  CFWL_Widget* pOwner = m_pProperties->m_pOwner;
  if (pOwner)
    m_pWidgetMgr->SetOwner(pOwner, this);
}

CFWL_Widget::~CFWL_Widget() {
  NotifyDriver();
  m_pWidgetMgr->RemoveWidget(this);
}

bool CFWL_Widget::IsInstance(const WideStringView& wsClass) const {
  return false;
}

CFX_RectF CFWL_Widget::GetAutosizedWidgetRect() {
  return CFX_RectF();
}

CFX_RectF CFWL_Widget::GetWidgetRect() {
  return m_pProperties->m_rtWidget;
}

void CFWL_Widget::InflateWidgetRect(CFX_RectF& rect) {
  if (HasBorder()) {
    float fBorder = GetBorderSize(true);
    rect.Inflate(fBorder, fBorder);
  }
}

void CFWL_Widget::SetWidgetRect(const CFX_RectF& rect) {
  m_pProperties->m_rtWidget = rect;
}

CFX_RectF CFWL_Widget::GetClientRect() {
  return GetEdgeRect();
}

void CFWL_Widget::SetParent(CFWL_Widget* pParent) {
  m_pProperties->m_pParent = pParent;
  m_pWidgetMgr->SetParent(pParent, this);
}

uint32_t CFWL_Widget::GetStyles() const {
  return m_pProperties->m_dwStyles;
}

void CFWL_Widget::ModifyStyles(uint32_t dwStylesAdded,
                               uint32_t dwStylesRemoved) {
  m_pProperties->m_dwStyles =
      (m_pProperties->m_dwStyles & ~dwStylesRemoved) | dwStylesAdded;
}

uint32_t CFWL_Widget::GetStylesEx() const {
  return m_pProperties->m_dwStyleExes;
}
uint32_t CFWL_Widget::GetStates() const {
  return m_pProperties->m_dwStates;
}

void CFWL_Widget::ModifyStylesEx(uint32_t dwStylesExAdded,
                                 uint32_t dwStylesExRemoved) {
  m_pProperties->m_dwStyleExes =
      (m_pProperties->m_dwStyleExes & ~dwStylesExRemoved) | dwStylesExAdded;
}

static void NotifyHideChildWidget(CFWL_WidgetMgr* widgetMgr,
                                  CFWL_Widget* widget,
                                  CFWL_NoteDriver* noteDriver) {
  CFWL_Widget* child = widgetMgr->GetFirstChildWidget(widget);
  while (child) {
    noteDriver->NotifyTargetHide(child);
    NotifyHideChildWidget(widgetMgr, child, noteDriver);
    child = widgetMgr->GetNextSiblingWidget(child);
  }
}

void CFWL_Widget::SetStates(uint32_t dwStates) {
  m_pProperties->m_dwStates |= dwStates;
  if (!(dwStates & FWL_WGTSTATE_Invisible))
    return;

  CFWL_NoteDriver* noteDriver =
      static_cast<CFWL_NoteDriver*>(GetOwnerApp()->GetNoteDriver());
  CFWL_WidgetMgr* widgetMgr = GetOwnerApp()->GetWidgetMgr();
  noteDriver->NotifyTargetHide(this);
  CFWL_Widget* child = widgetMgr->GetFirstChildWidget(this);
  while (child) {
    noteDriver->NotifyTargetHide(child);
    NotifyHideChildWidget(widgetMgr, child, noteDriver);
    child = widgetMgr->GetNextSiblingWidget(child);
  }
  return;
}

void CFWL_Widget::RemoveStates(uint32_t dwStates) {
  m_pProperties->m_dwStates &= ~dwStates;
}

FWL_WidgetHit CFWL_Widget::HitTest(const CFX_PointF& point) {
  if (GetClientRect().Contains(point))
    return FWL_WidgetHit::Client;
  if (HasBorder() && GetRelativeRect().Contains(point))
    return FWL_WidgetHit::Border;
  return FWL_WidgetHit::Unknown;
}

CFX_PointF CFWL_Widget::TransformTo(CFWL_Widget* pWidget,
                                    const CFX_PointF& point) {
  CFX_SizeF szOffset;
  if (IsParent(pWidget)) {
    szOffset = GetOffsetFromParent(pWidget);
  } else {
    szOffset = pWidget->GetOffsetFromParent(this);
    szOffset.width = -szOffset.width;
    szOffset.height = -szOffset.height;
  }
  return point + CFX_PointF(szOffset.width, szOffset.height);
}

CFX_Matrix CFWL_Widget::GetMatrix() {
  if (!m_pProperties)
    return CFX_Matrix();

  CFWL_Widget* parent = GetParent();
  std::vector<CFWL_Widget*> parents;
  while (parent) {
    parents.push_back(parent);
    parent = parent->GetParent();
  }

  CFX_Matrix matrix;
  CFX_Matrix ctmOnParent;
  CFX_RectF rect;
  int32_t count = pdfium::CollectionSize<int32_t>(parents);
  for (int32_t i = count - 2; i >= 0; i--) {
    parent = parents[i];
    if (parent->m_pProperties)
      ctmOnParent.SetIdentity();
    rect = parent->GetWidgetRect();
    matrix.Concat(ctmOnParent, true);
    matrix.Translate(rect.left, rect.top, true);
  }
  CFX_Matrix m;
  m.SetIdentity();
  matrix.Concat(m, true);
  parents.clear();
  return matrix;
}

IFWL_ThemeProvider* CFWL_Widget::GetThemeProvider() const {
  return m_pProperties->m_pThemeProvider;
}

void CFWL_Widget::SetThemeProvider(IFWL_ThemeProvider* pThemeProvider) {
  m_pProperties->m_pThemeProvider = pThemeProvider;
}

bool CFWL_Widget::IsEnabled() const {
  return (m_pProperties->m_dwStates & FWL_WGTSTATE_Disabled) == 0;
}

bool CFWL_Widget::HasBorder() const {
  return !!(m_pProperties->m_dwStyles & FWL_WGTSTYLE_Border);
}

bool CFWL_Widget::IsVisible() const {
  return (m_pProperties->m_dwStates & FWL_WGTSTATE_Invisible) == 0;
}

bool CFWL_Widget::IsOverLapper() const {
  return (m_pProperties->m_dwStyles & FWL_WGTSTYLE_WindowTypeMask) ==
         FWL_WGTSTYLE_OverLapper;
}

bool CFWL_Widget::IsPopup() const {
  return !!(m_pProperties->m_dwStyles & FWL_WGTSTYLE_Popup);
}

bool CFWL_Widget::IsChild() const {
  return !!(m_pProperties->m_dwStyles & FWL_WGTSTYLE_Child);
}

CFX_RectF CFWL_Widget::GetEdgeRect() {
  CFX_RectF rtEdge(0, 0, m_pProperties->m_rtWidget.width,
                   m_pProperties->m_rtWidget.height);
  if (HasBorder()) {
    float fCX = GetBorderSize(true);
    float fCY = GetBorderSize(false);
    rtEdge.Deflate(fCX, fCY);
  }
  return rtEdge;
}

float CFWL_Widget::GetBorderSize(bool bCX) {
  IFWL_ThemeProvider* theme = GetAvailableTheme();
  if (!theme)
    return 0.0f;
  return bCX ? theme->GetCXBorderSize() : theme->GetCYBorderSize();
}

CFX_RectF CFWL_Widget::GetRelativeRect() {
  return CFX_RectF(0, 0, m_pProperties->m_rtWidget.width,
                   m_pProperties->m_rtWidget.height);
}

IFWL_ThemeProvider* CFWL_Widget::GetAvailableTheme() {
  if (m_pProperties->m_pThemeProvider)
    return m_pProperties->m_pThemeProvider;

  CFWL_Widget* pUp = this;
  do {
    pUp = (pUp->GetStyles() & FWL_WGTSTYLE_Popup)
              ? m_pWidgetMgr->GetOwnerWidget(pUp)
              : m_pWidgetMgr->GetParentWidget(pUp);
    if (pUp) {
      IFWL_ThemeProvider* pRet = pUp->GetThemeProvider();
      if (pRet)
        return pRet;
    }
  } while (pUp);
  return nullptr;
}

CFWL_Widget* CFWL_Widget::GetRootOuter() {
  CFWL_Widget* pRet = m_pOuter;
  if (!pRet)
    return nullptr;

  while (CFWL_Widget* pOuter = pRet->GetOuter())
    pRet = pOuter;
  return pRet;
}

CFX_SizeF CFWL_Widget::CalcTextSize(const WideString& wsText,
                                    IFWL_ThemeProvider* pTheme,
                                    bool bMultiLine) {
  if (!pTheme)
    return CFX_SizeF();

  CFWL_ThemeText calPart;
  calPart.m_pWidget = this;
  calPart.m_wsText = wsText;
  if (bMultiLine)
    calPart.m_dwTTOStyles.line_wrap_ = true;
  else
    calPart.m_dwTTOStyles.single_line_ = true;

  calPart.m_iTTOAlign = FDE_TextAlignment::kTopLeft;
  float fWidth = bMultiLine ? FWL_WGT_CalcMultiLineDefWidth : FWL_WGT_CalcWidth;
  CFX_RectF rect(0, 0, fWidth, FWL_WGT_CalcHeight);
  pTheme->CalcTextRect(&calPart, &rect);
  return CFX_SizeF(rect.width, rect.height);
}

void CFWL_Widget::CalcTextRect(const WideString& wsText,
                               IFWL_ThemeProvider* pTheme,
                               const FDE_TextStyle& dwTTOStyles,
                               FDE_TextAlignment iTTOAlign,
                               CFX_RectF* pRect) {
  CFWL_ThemeText calPart;
  calPart.m_pWidget = this;
  calPart.m_wsText = wsText;
  calPart.m_dwTTOStyles = dwTTOStyles;
  calPart.m_iTTOAlign = iTTOAlign;
  pTheme->CalcTextRect(&calPart, pRect);
}

void CFWL_Widget::SetGrab(bool bSet) {
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pDriver =
      static_cast<CFWL_NoteDriver*>(pApp->GetNoteDriver());
  pDriver->SetGrab(this, bSet);
}

void CFWL_Widget::RegisterEventTarget(CFWL_Widget* pEventSource) {
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pNoteDriver = pApp->GetNoteDriver();
  if (!pNoteDriver)
    return;

  pNoteDriver->RegisterEventTarget(this, pEventSource);
}

void CFWL_Widget::UnregisterEventTarget() {
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pNoteDriver = pApp->GetNoteDriver();
  if (!pNoteDriver)
    return;

  pNoteDriver->UnregisterEventTarget(this);
}

void CFWL_Widget::DispatchEvent(CFWL_Event* pEvent) {
  if (m_pOuter) {
    m_pOuter->GetDelegate()->OnProcessEvent(pEvent);
    return;
  }
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pNoteDriver = pApp->GetNoteDriver();
  if (!pNoteDriver)
    return;
  pNoteDriver->SendEvent(pEvent);
}

void CFWL_Widget::Repaint() {
  RepaintRect(CFX_RectF(0, 0, m_pProperties->m_rtWidget.width,
                        m_pProperties->m_rtWidget.height));
}

void CFWL_Widget::RepaintRect(const CFX_RectF& pRect) {
  m_pWidgetMgr->RepaintWidget(this, pRect);
}

void CFWL_Widget::DrawBackground(CXFA_Graphics* pGraphics,
                                 CFWL_Part iPartBk,
                                 IFWL_ThemeProvider* pTheme,
                                 const CFX_Matrix* pMatrix) {
  CFWL_ThemeBackground param;
  param.m_pWidget = this;
  param.m_iPart = iPartBk;
  param.m_pGraphics = pGraphics;
  if (pMatrix)
    param.m_matrix.Concat(*pMatrix, true);
  param.m_rtPart = GetRelativeRect();
  pTheme->DrawBackground(&param);
}

void CFWL_Widget::DrawBorder(CXFA_Graphics* pGraphics,
                             CFWL_Part iPartBorder,
                             IFWL_ThemeProvider* pTheme,
                             const CFX_Matrix& matrix) {
  CFWL_ThemeBackground param;
  param.m_pWidget = this;
  param.m_iPart = iPartBorder;
  param.m_pGraphics = pGraphics;
  param.m_matrix.Concat(matrix, true);
  param.m_rtPart = GetRelativeRect();
  pTheme->DrawBackground(&param);
}

void CFWL_Widget::NotifyDriver() {
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pDriver =
      static_cast<CFWL_NoteDriver*>(pApp->GetNoteDriver());
  if (!pDriver)
    return;

  pDriver->NotifyTargetDestroy(this);
}

CFX_SizeF CFWL_Widget::GetOffsetFromParent(CFWL_Widget* pParent) {
  if (pParent == this)
    return CFX_SizeF();

  CFWL_WidgetMgr* pWidgetMgr = GetOwnerApp()->GetWidgetMgr();
  if (!pWidgetMgr)
    return CFX_SizeF();

  CFX_SizeF szRet(m_pProperties->m_rtWidget.left,
                  m_pProperties->m_rtWidget.top);

  CFWL_Widget* pDstWidget = GetParent();
  while (pDstWidget && pDstWidget != pParent) {
    CFX_RectF rtDst = pDstWidget->GetWidgetRect();
    szRet += CFX_SizeF(rtDst.left, rtDst.top);
    pDstWidget = pWidgetMgr->GetParentWidget(pDstWidget);
  }
  return szRet;
}

bool CFWL_Widget::IsParent(CFWL_Widget* pParent) {
  CFWL_Widget* pUpWidget = GetParent();
  while (pUpWidget) {
    if (pUpWidget == pParent)
      return true;
    pUpWidget = pUpWidget->GetParent();
  }
  return false;
}

void CFWL_Widget::OnProcessMessage(CFWL_Message* pMessage) {
  if (!pMessage->m_pDstTarget)
    return;

  CFWL_Widget* pWidget = pMessage->m_pDstTarget;
  switch (pMessage->GetType()) {
    case CFWL_Message::Type::Mouse: {
      CFWL_MessageMouse* pMsgMouse = static_cast<CFWL_MessageMouse*>(pMessage);

      CFWL_EventMouse evt(pWidget, pWidget);
      evt.m_dwCmd = pMsgMouse->m_dwCmd;
      pWidget->DispatchEvent(&evt);
      break;
    }
    default:
      break;
  }
}

void CFWL_Widget::OnProcessEvent(CFWL_Event* pEvent) {}

void CFWL_Widget::OnDrawWidget(CXFA_Graphics* pGraphics,
                               const CFX_Matrix& matrix) {}
