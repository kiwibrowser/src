// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/cfwl_form.h"

#include <utility>

#include "third_party/base/ptr_util.h"
#include "xfa/fde/cfde_textout.h"
#include "xfa/fwl/cfwl_app.h"
#include "xfa/fwl/cfwl_event.h"
#include "xfa/fwl/cfwl_messagemouse.h"
#include "xfa/fwl/cfwl_notedriver.h"
#include "xfa/fwl/cfwl_noteloop.h"
#include "xfa/fwl/cfwl_themebackground.h"
#include "xfa/fwl/cfwl_themepart.h"
#include "xfa/fwl/cfwl_themetext.h"
#include "xfa/fwl/cfwl_widgetmgr.h"
#include "xfa/fwl/ifwl_themeprovider.h"

CFWL_Form::CFWL_Form(const CFWL_App* app,
                     std::unique_ptr<CFWL_WidgetProperties> properties,
                     CFWL_Widget* pOuter)
    : CFWL_Widget(app, std::move(properties), pOuter),
      m_pSubFocus(nullptr),
      m_fCXBorder(0),
      m_fCYBorder(0) {
  m_rtRelative.Reset();
  m_rtRestore.Reset();

  RegisterForm();
  RegisterEventTarget(nullptr);
}

CFWL_Form::~CFWL_Form() {
  UnregisterEventTarget();
  UnRegisterForm();
}

FWL_Type CFWL_Form::GetClassID() const {
  return FWL_Type::Form;
}

bool CFWL_Form::IsInstance(const WideStringView& wsClass) const {
  if (wsClass == WideStringView(FWL_CLASS_Form))
    return true;
  return CFWL_Widget::IsInstance(wsClass);
}

CFX_RectF CFWL_Form::GetClientRect() {
  CFX_RectF rect = m_pProperties->m_rtWidget;
  rect.Offset(-rect.left, -rect.top);
  return rect;
}

void CFWL_Form::Update() {
  if (m_iLock > 0)
    return;
  if (!m_pProperties->m_pThemeProvider)
    m_pProperties->m_pThemeProvider = GetAvailableTheme();

  Layout();
}

FWL_WidgetHit CFWL_Form::HitTest(const CFX_PointF& point) {
  GetAvailableTheme();

  CFX_RectF rtCap(m_fCYBorder, m_fCXBorder, -2 * m_fCYBorder, 0 - m_fCXBorder);
  return rtCap.Contains(point) ? FWL_WidgetHit::Titlebar
                               : FWL_WidgetHit::Client;
}

void CFWL_Form::DrawWidget(CXFA_Graphics* pGraphics, const CFX_Matrix& matrix) {
  if (!pGraphics)
    return;
  if (!m_pProperties->m_pThemeProvider)
    return;

  IFWL_ThemeProvider* pTheme = m_pProperties->m_pThemeProvider;
  DrawBackground(pGraphics, pTheme);

#if _FX_OS_ == _FX_OS_MACOSX_
  return;
#endif
  CFWL_ThemeBackground param;
  param.m_pWidget = this;
  param.m_dwStates = CFWL_PartState_Normal;
  param.m_pGraphics = pGraphics;
  param.m_rtPart = m_rtRelative;
  param.m_matrix.Concat(matrix);
  if (m_pProperties->m_dwStyles & FWL_WGTSTYLE_Border) {
    param.m_iPart = CFWL_Part::Border;
    pTheme->DrawBackground(&param);
  }
}

CFWL_Widget* CFWL_Form::DoModal() {
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return nullptr;

  CFWL_NoteDriver* pDriver = pApp->GetNoteDriver();
  if (!pDriver)
    return nullptr;

  m_pNoteLoop = pdfium::MakeUnique<CFWL_NoteLoop>();
  m_pNoteLoop->SetMainForm(this);

  pDriver->PushNoteLoop(m_pNoteLoop.get());
  RemoveStates(FWL_WGTSTATE_Invisible);
  pDriver->Run();

#if _FX_OS_ != _FX_OS_MACOSX_
  pDriver->PopNoteLoop();
#endif

  m_pNoteLoop.reset();
  return nullptr;
}

void CFWL_Form::EndDoModal() {
  if (!m_pNoteLoop)
    return;

#if (_FX_OS_ == _FX_OS_MACOSX_)
  m_pNoteLoop->EndModalLoop();
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pDriver =
      static_cast<CFWL_NoteDriver*>(pApp->GetNoteDriver());
  if (!pDriver)
    return;

  pDriver->PopNoteLoop();
  SetStates(FWL_WGTSTATE_Invisible);
#else
  SetStates(FWL_WGTSTATE_Invisible);
  m_pNoteLoop->EndModalLoop();
#endif
}

void CFWL_Form::DrawBackground(CXFA_Graphics* pGraphics,
                               IFWL_ThemeProvider* pTheme) {
  CFWL_ThemeBackground param;
  param.m_pWidget = this;
  param.m_iPart = CFWL_Part::Background;
  param.m_pGraphics = pGraphics;
  param.m_rtPart = m_rtRelative;
  param.m_rtPart.Deflate(m_fCYBorder, m_fCXBorder, m_fCYBorder, m_fCXBorder);
  pTheme->DrawBackground(&param);
}

CFX_RectF CFWL_Form::GetEdgeRect() {
  CFX_RectF rtEdge = m_rtRelative;
  if (m_pProperties->m_dwStyles & FWL_WGTSTYLE_Border) {
    float fCX = GetBorderSize(true);
    float fCY = GetBorderSize(false);
    rtEdge.Deflate(fCX, fCY, fCX, fCY);
  }
  return rtEdge;
}

void CFWL_Form::SetWorkAreaRect() {
  m_rtRestore = m_pProperties->m_rtWidget;
  CFWL_WidgetMgr* pWidgetMgr = GetOwnerApp()->GetWidgetMgr();
  if (!pWidgetMgr)
    return;
  RepaintRect(m_rtRelative);
}

void CFWL_Form::Layout() {
  m_rtRelative = GetRelativeRect();

#if _FX_OS_ == _FX_OS_MACOSX_
  IFWL_ThemeProvider* theme = GetAvailableTheme();
  m_fCXBorder = theme ? theme->GetCXBorderSize() : 0.0f;
  m_fCYBorder = theme ? theme->GetCYBorderSize() : 0.0f;
#endif
}

void CFWL_Form::RegisterForm() {
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pDriver =
      static_cast<CFWL_NoteDriver*>(pApp->GetNoteDriver());
  if (!pDriver)
    return;

  pDriver->RegisterForm(this);
}

void CFWL_Form::UnRegisterForm() {
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pDriver =
      static_cast<CFWL_NoteDriver*>(pApp->GetNoteDriver());
  if (!pDriver)
    return;

  pDriver->UnRegisterForm(this);
}

void CFWL_Form::OnProcessMessage(CFWL_Message* pMessage) {
#if _FX_OS_ == _FX_OS_MACOSX_
  if (!pMessage)
    return;

  switch (pMessage->GetType()) {
    case CFWL_Message::Type::Mouse: {
      CFWL_MessageMouse* pMsg = static_cast<CFWL_MessageMouse*>(pMessage);
      switch (pMsg->m_dwCmd) {
        case FWL_MouseCommand::LeftButtonDown:
          OnLButtonDown(pMsg);
          break;
        case FWL_MouseCommand::LeftButtonUp:
          OnLButtonUp(pMsg);
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
#endif  // _FX_OS_ == _FX_OS_MACOSX_
}

void CFWL_Form::OnDrawWidget(CXFA_Graphics* pGraphics,
                             const CFX_Matrix& matrix) {
  DrawWidget(pGraphics, matrix);
}

void CFWL_Form::OnLButtonDown(CFWL_MessageMouse* pMsg) {
  SetGrab(true);
}

void CFWL_Form::OnLButtonUp(CFWL_MessageMouse* pMsg) {
  SetGrab(false);
}
